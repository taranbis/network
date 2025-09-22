#include "tcp_connection_manager.hpp"

#include <string>
#include <thread>
#include <cstring>
#include <iostream>
#include <memory>
#include <format>
#include <unordered_map>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "tcp_util.hpp"

TCPConnectionManager::TCPConnectionManager() 
{
    WSADATA wsaData;
    const int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        std::cerr << "WSAStartup failed: " << res << std::endl;
        return;
    }
    std::clog << "Winsock initialized.\n";

    m_connThreadsCleaner = std::jthread([this]() {
        while (!m_finish) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] {
                return m_threadFinished.first || m_finish;
            });

            //std::clog << "removing thread for connection " << m_threadFinished.second << std::endl;

            {
                std::lock_guard<std::mutex> connThreadsLock(m_connThreadsMutex);
                m_connThreads[m_threadFinished.second].request_stop();
                m_connThreads.erase(m_threadFinished.second);
            }

            m_threadFinished = {false, -1};
        }
    });
}

TCPConnectionManager::~TCPConnectionManager()
{
    //! this could be DEAD long ago; this is why i use weak_ptr. to look for forgotten connections
    //for (std::weak_ptr<TCPConnection> conn : m_connections) {
    //    if (std::shared_ptr<TCPConnection> connSpt = conn.lock()) connSpt->stop();
    //}
    stop();
    m_cv.notify_all();
    {
        std::lock_guard<std::mutex> lock(m_connThreadsMutex);
        for (auto& connThread : m_connThreads) connThread.second.join();
    }
    newConnection.disconnect_all_slots();
    connectionClosed.disconnect_all_slots();
    //m_connThreads.clear();
    WSACleanup();
}

void TCPConnectionManager::stop()
{
    m_finish = true; //this stops also the reading threads to clean up their connections

    {
        std::lock_guard<std::mutex> lock(m_connThreadsMutex);
        for (auto& connThread : m_connThreads) connThread.second.request_stop();
    }
    std::unique_lock lock(m_connectionsMutex);
    const auto copyConns = m_connections;
    lock.unlock();
    for (auto& conn : copyConns) { 
        closeConn(conn.second->connInfo());
    }
}

std::string TCPConnectionManager::dnsLookup(const std::string& host, uint16_t ipVersion)
{
    char ipAddress[INET6_ADDRSTRLEN]; // choose directly the maximum length which is for ipv6;

    struct addrinfo hints{};
    memset(&hints, 0, sizeof hints);
    hints.ai_family = (ipVersion == 6 ? AF_INET6 : AF_INET); // if it's 6 we choose IPv6, else we go with IPv4
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res;
    int status = 0;
    if ((status = getaddrinfo(host.c_str(), NULL, &hints, &res)) != 0) {
        std::cerr << "getaddrinfo failed: " << gai_strerror(status) << std::endl;
        return {};
    }

    // for (auto p = res; p != NULL; p = p->ai_next); this is only one. we looked for either IPv4 or IPv6. if we
    // looked for both it would have been a linked list
    void* addr;
    if (res->ai_family == AF_INET) {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;
        addr = &(ipv4->sin_addr);
        inet_ntop(res->ai_family, addr, ipAddress, sizeof ipAddress);
    } else {
        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)res->ai_addr;
        addr = &(ipv6->sin6_addr);
        inet_ntop(res->ai_family, addr, ipAddress, sizeof ipAddress);
    }

    freeaddrinfo(res); // free the linked list

    return ipAddress;
}

TCPConnInfo TCPConnectionManager::openConnection(const std::string& destAddress, uint16_t destPort)
{
    return openConnection(destAddress, destPort, std::string(), 0);
}

TCPConnInfo TCPConnectionManager::openConnection(const std::string& destAddress, uint16_t destPort,
                                                const std::string& sourceAddress, uint16_t sourcePort)
{
    if (destAddress.empty()) {
        std::cerr << "Destination address not provided" << std::endl;
        return {};
    }

    // SOCK_STREAM for TCP, SOCK_DGRAM for UDP
    const SOCKET sockfd = socket(destAddress.find(".") == -1 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "couldn't create socket" << std::endl;
        return {};
    }

    //! Don't do non-blocking. I ran into "Resource Temporarily Unavailable" when trying to send an HTTP Request  to google
    // if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
    //     std::cerr << "cannot set fd non blocking " << std::endl;
    //     return {};
    // }

    const int on = !sourceAddress.empty() ? 1 : 0;
    int err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    if (err) {
        std::cerr << "couldn't set SO_REUSEADDR option" << std::endl;
        return {};
    }

    if (!sourceAddress.empty()) {
        struct sockaddr addr;
        err = makeSockAddr(sourceAddress, sourcePort, addr);
        if (err) {
            std::cerr << "couldn't create sockAddr" << std::endl;
            return {};
        }

        err = bind(sockfd, &addr, sizeof(addr));
        if (err) {
            std::cerr << "couldn't bind source address and port" << std::endl;
            return {};
        }
    }

    struct sockaddr addr;
    err = makeSockAddr(destAddress, destPort, addr);
    if (err) {
        std::cerr << "couldn't create sockAddr" << std::endl;
        return {};
    }

    err = connect(sockfd, &addr, sizeof(addr));
    if (err || errno == EINPROGRESS) {
        std::cerr << "couldn't connect to destination address and port" << std::endl;
        return {};
    }

    const TCPConnInfo connInfo{.sockfd = sockfd, .peerIP = destAddress, .peerPort = destPort};
    std::shared_ptr<TCPConnection> conn{new TCPConnection(*this, connInfo)};
    conn->startReadingData();
    addConnection(sockfd, std::move(conn));

    std::clog << std::format("New Connection - socket fd: {}; destIp: {}, destPort: {}\n", sockfd,
                    connInfo.peerIP, connInfo.peerPort);

    return connInfo;
}

void TCPConnectionManager::startReadingData(const TCPConnInfo& connInfo)
{
    std::lock_guard<std::mutex> lock(m_connThreadsMutex);
    if (m_connThreads[connInfo.sockfd].joinable()) return;
    m_connThreads[connInfo.sockfd] = std::jthread([this, connInfo = connInfo](std::stop_token st) {
        readDataFromSocket(st, connInfo);
        // connection should be closed it is no longer valid.
        closeConn(connInfo);
    });
}

void TCPConnectionManager::readDataFromSocket(std::stop_token st, TCPConnInfo connData) const
{
        std::unique_ptr<char> buffer(new char[1024]);
        while (!m_finish && !st.stop_requested()) {
            WSAPOLLFD fd{};
            fd.fd = connData.sockfd;
            fd.events = POLLIN; // Wait for incoming data

            int wsaPollRes = WSAPoll(&fd, 1, 2000); // 2 seconds timeout
            if (wsaPollRes > 0) {
                // If no error occurs, recv returns the number of bytes received and the buffer pointed to by the
                // buf parameter will If the connection has been gracefully closed, the return value is zero.
                // Otherwise, a value of SOCKET_ERROR is returned
                const int recvRes = recv(connData.sockfd, buffer.get(), 1024, 0);
                if (recvRes == SOCKET_ERROR) {
                    std::cerr << std::format("receive failed on socket {}; closing connection!\n", connData.sockfd);
                    return;
                }

                if (recvRes == 0) {
                    std::clog << std::format("connection on socket {} was closed by peer\n", connData.sockfd);
                    return;
                }

                if (m_printReceivedData) {
                    std::cout << "Number of bytes read: " << recvRes << "; Message: " << std::endl;
                    for (int i = 0; i < recvRes; ++i) std::cout << *(buffer.get() + i);
                    std::cout << std::endl;
                }

                const auto conn = getConnectionDirect(connData.sockfd);
                if(!conn) {
                    std::cerr << std::format("socket {} is no longer an active connection", connData.sockfd);
                    return;
                }

                std::vector<char> bytes(buffer.get(), buffer.get() + recvRes);
                conn->newDataArrived(bytes);
                conn->newDataArrived(bytes);
            } else if (wsaPollRes == 0) {
                // Timeout: No data received
            } else {
                std::cerr << "WSAPoll() failed with error: " << WSAGetLastError() << std::endl;
                return;
            }
        }
    }

void TCPConnectionManager::closeConn(const TCPConnInfo connInfo) {
    //std::clog << "TCPConnectionManager::closeConn for connInfo.sockfd " << connInfo.sockfd << std::endl;
    if (!hasConnection(connInfo.sockfd)) return;

    std::lock_guard lock(m_mutex);
    removeConnection(connInfo.sockfd);
    connectionClosed(connInfo);
    m_threadFinished.first = true;
    m_threadFinished.second = connInfo.sockfd;
    m_cv.notify_all();
}

TCPConnInfo TCPConnectionManager::openListenSocket(const std::string& hostAddr, uint16_t port)
{
    if (hostAddr.empty()) {
        std::cerr << "No peer address provided" << std::endl;
        return {};
    }

    // SOCK_STREAM for TCP, SOCK_DGRAM for UDP
    const SOCKET listenSocket = socket(hostAddr.find(".") == -1 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "couldn't create socket" << std::endl;
        return {};
    }

    //if (fcntl(listenSocket, F_SETFL, O_NONBLOCK) == -1)  // on UNIX systems
    u_long mode = 1; // 1 = non-blocking, 0 = blocking
    if (ioctlsocket(listenSocket, FIONBIO, &mode)) {
        std::cerr << "cannot set fd non blocking " << std::endl;
        return {};
    }

    const int on = 1;
    int err = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    if (err < 0) {
        std::cerr << "couldn't set option" << std::endl;
        closesocket(listenSocket);
        return {};
    }

    struct sockaddr addr;
    err = makeSockAddr(hostAddr, port, addr);
    if (err) {
        std::cerr << "couldn't create sockAddr" << std::endl;
        return {};
    }

    err = bind(listenSocket, &addr, sizeof(addr));
    if (err < 0) {
        // couldn't bind source address and port;
        //std::cerr << "cannot bind to " << hostAddr << ":" << port << " (" << errno << ", " << strerror(errno)
        //            << ")" << std::endl;
        printErrorMessage();
        closesocket(listenSocket);
        return {};
    }

    // the second number represents the maximum number of accepted connections, before they start being refused
    if (listen(listenSocket, 1024) < 0) {
        printErrorMessage();
        closesocket(listenSocket);
        return {};
    }

    const TCPConnInfo connInfo{.sockfd = listenSocket, .peerIP = hostAddr, .peerPort = port};
    std::shared_ptr<TCPConnection> conn{new TCPConnection(*this, connInfo)};
    {
        std::lock_guard<std::mutex> lock(m_connThreadsMutex);
        m_connThreads[listenSocket] =
                    std::jthread([this, connInfo = connInfo](std::stop_token st) { 
             this->checkForConnections(st, connInfo); 
         });
    }

    //th.detach(); - no longer needed
    std::clog << std::format("New Listening Socket - socket fd: {}; on IP: {}, on Port: {}\n", listenSocket,
                            connInfo.peerIP, connInfo.peerPort);
    addConnection(listenSocket, std::move(conn));
    return connInfo;
}

void TCPConnectionManager::checkForConnections(std::stop_token st, const TCPConnInfo& connInfo)
{
    SOCKET listenSockFD = connInfo.sockfd;
    fd_set set{};
    
    while (!m_finish && !st.stop_requested()) {
        FD_ZERO(&set);              // reset memory 
        FD_SET(listenSockFD, &set); // add the socket file descriptor to the set
        timeval timeout = {2, 0};   // 2 seconds timeout
        const int activity = select(-1 /*ignored*/, &set, NULL, NULL, &timeout);

        if (activity == SOCKET_ERROR) { 
            std::cerr << "select error " << std::endl; 
            continue;
        }

        if (activity == 0) continue; // timeout

        // If something happened on the socket, then its an incoming connection
        if (FD_ISSET(listenSockFD, &set)) {
            struct sockaddr addr;
            const int err = makeSockAddr(connInfo.peerIP, connInfo.peerPort, addr);
            if (err) {
                std::cerr << "couldn't create sockAddr" << std::endl;
                continue;
            }

            int size = sizeof(addr);
            SOCKET newSockFd = accept(listenSockFD, &addr, &size);
            if (newSockFd == INVALID_SOCKET) {
                std::cerr << "accept error" << std::endl;
                continue;
            }
            std::clog << std::format("New Connection - socket fd: {}; peerIp: {}, peerPort: {}", newSockFd,
                                     connInfo.peerIP, connInfo.peerPort) << std::endl;
            //! used to send sth on to the client but SHOULD NOT send anything on the socket. e.g. failure for HTTP expects and HTTP message; 
            // this is the job of the client; 

            std::shared_ptr<TCPConnection> newConn{new TCPConnection(*this, connInfo)};
            newConn->connInfo().sockfd = newSockFd;
            newConn->startReadingData();

            const TCPConnInfo connInfo = newConn->connInfo();
            addConnection(newSockFd, std::move(newConn));
            newConnection(connInfo);
            newConnectionOnListeningSocket.sendTo(listenSockFD,connInfo);
        }
    }
}

bool TCPConnectionManager::write(TCPConnInfo connData, const std::string& msg)
{
    if (!hasConnection(connData.sockfd)) return false;

    // send returns the total number of bytes sent. Otherwise, a value of SOCKET_ERROR is returned
    const int res = send(connData.sockfd, msg.data(), (int)msg.size(), 0);
    if (res == SOCKET_ERROR) {
        std::cerr << "send failed; msg: " << msg << ", error: " << WSAGetLastError() << "\n";
        printErrorMessage();
        return false;
    }
    return true;
}


void TCPConnectionManager::addConnection(SOCKET sockfd, std::shared_ptr<TCPConnection> conn)
{
    std::lock_guard lock(m_connectionsMutex);
    m_connections.emplace(sockfd, std::move(conn));
}

void TCPConnectionManager::removeConnection(SOCKET sockfd)
{
    std::lock_guard lock(m_connectionsMutex);
    m_connections.erase(sockfd);
}

bool TCPConnectionManager::hasConnection(SOCKET sockfd) const
{
    std::lock_guard lock(m_connectionsMutex);
    return m_connections.contains(sockfd);
}

std::shared_ptr<TCPConnection> TCPConnectionManager::getConnectionDirect(SOCKET sockfd) const
{
    std::lock_guard lock(m_connectionsMutex);
    const auto it = m_connections.find(sockfd);
    return (it != m_connections.end()) ? it->second : nullptr;
}

std::weak_ptr<TCPConnection> TCPConnectionManager::getConnection(const TCPConnInfo& connInfo) const
{
    std::lock_guard lock(m_connectionsMutex);

    return m_connections.at(connInfo.sockfd);
}