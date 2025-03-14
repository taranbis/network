#include "tcp_connection_manager.hpp"

#include <string>
#include <thread>
#include <cstring>
#include <functional>
#include <iostream>
#include <atomic>
#include <memory>

#include <winsock2.h>
#include <ws2tcpip.h>

TCPConnectionManager::~TCPConnectionManager()
{
    //! this could be DEAD long ago; this is why i use weak_ptr. to look for forgotten connections
    for (std::weak_ptr<TCPConnection> conn : connections_) {
        if (std::shared_ptr<TCPConnection> connSpt = conn.lock()) connSpt->stop();
    }
}

void TCPConnectionManager::stop()
{
    finish = true;
}

std::string TCPConnectionManager::dnsLookup(const std::string& host, uint16_t ipVersion)
{
    char ipAddress[INET6_ADDRSTRLEN]; // choose directly the maximum length which is for ipv6;

    struct addrinfo hints {
    };
    memset(&hints, 0, sizeof hints);
    hints.ai_family = (ipVersion == 6 ? AF_INET6 : AF_INET); // if it's 6 we choose IPv6, else we go with IPv4
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res;
    int status = 0;
    if ((status = getaddrinfo(host.c_str(), NULL, &hints, &res)) != 0) {
        std::cerr << "getaddrinfo failed: " << gai_strerror(status) << std::endl;
        return {};
    }

    // for (auto p = res; p != NULL; p = p->ai_next); this is only one. we looked for either IPv4 or 6. if we
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

std::shared_ptr<TCPConnection> TCPConnectionManager::openConnection(const std::string& destAddress, uint16_t destPort)
{
    return openConnection(destAddress, destPort, std::string(), 0);
}

std::shared_ptr<TCPConnection> TCPConnectionManager::openConnection(const std::string& destAddress,
                                                                uint16_t destPort,
                                                const std::string& sourceAddress, uint16_t sourcePort)
{
    if (destAddress.empty()) {
        std::cerr << "Destination address not provided" << std::endl;
        return {};
    }

    // SOCK_STREAM for TCP, SOCK_DGRAM for UDP
    SOCKET sockfd = socket(destAddress.find(".") == -1 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "couldn't create socket" << std::endl;
        return {};
    }

    //! Don't do non-blocking. I ran into "Resource Temporarily Unavailable" when trying to send an HTTP Request  to google
    // if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
    //     std::cerr << "cannot set fd non blocking " << std::endl;
    //     return {};
    // }

    int on = !sourceAddress.empty() ? 1 : 0;
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
    std::shared_ptr<TCPConnection> conn{new TCPConnection(connInfo)};
    connections_.push_back(conn);
    return conn;
}

//std::shared_ptr<TCPConnection> TCPConnectionManager::start(const std::string& ipAddr, uint16_t port)
//{
//    return openListenSocket(ipAddr, port);
//}
//
//bool TCPConnectionManager::start(int listenSocket)
//{
//    // if ((new_socket = accept(listenSocket, (struct sockaddr *)&address,
//    //                          (socklen_t *)&addrlen)) < 0) {
//    //     perror("accept");
//    //     exit(EXIT_FAILURE);
//    // }
//    return false;
//}

std::shared_ptr<TCPConnection> TCPConnectionManager::openListenSocket(const std::string& ipAddr, uint16_t port)
{
    if (ipAddr.empty()) {
        std::cerr << "No peer address provided" << std::endl;
        return {};
    }

    // SOCK_STREAM for TCP, SOCK_DGRAM for UDP
    SOCKET sockfd = socket(ipAddr.find(".") == -1 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "couldn't create socket" << std::endl;
        return {};
    }

    int err;
    u_long mode = 1; // 1 = non-blocking, 0 = blocking
    //if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)  // on UNIX systems
    if ( ioctlsocket(sockfd, FIONBIO, &mode)) {
        std::cerr << "cannot set fd non blocking " << std::endl;
        return {};
    }

    int on = 1;
    err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    if (err < 0) {
        std::cerr << "couldn't set option" << std::endl;
        closesocket(sockfd);
        return {};
    }

    struct sockaddr addr;
    err = makeSockAddr(ipAddr, port, addr);
    if (err) {
        std::cerr << "couldn't create sockAddr" << std::endl;
        return {};
    }

    err = bind(sockfd, &addr, sizeof(addr));
    if (err < 0) {
        // couldn't bind source address and port;
        std::cerr << "cannot bind to " << ipAddr << ":" << port << " (" << errno << ", " << strerror(errno)
                    << ")" << std::endl;
        closesocket(sockfd);
        return {};
    }

    // the second number represents the maximum number of accepted connections, before they start being refused
    if (listen(sockfd, 1024) < 0) {
        closesocket(sockfd);
        return {};
    }

    const TCPConnInfo connInfo{.sockfd = sockfd, .peerIP = ipAddr, .peerPort = port};
    std::shared_ptr<TCPConnection> conn{new TCPConnection(connInfo)};
    std::thread th(&TCPConnectionManager::checkForConnections, this, conn.get());
    th.detach();
    connections_.push_back(conn);
    return conn;
}

void TCPConnectionManager::checkForConnections(TCPConnection* conn)
{
    const TCPConnInfo connInfo = conn->connData_;
    fd_set set;
    SOCKET listenSockFD = conn->connData_.sockfd;

    while (!finish) {
        FD_ZERO(&set);        /* clear the set */
        FD_SET(listenSockFD, &set); /* add the socket file descriptor to the set */
        int activity = select(listenSockFD + 1, &set, NULL, NULL, NULL);

        if (activity == SOCKET_ERROR) { 
            std::cerr << "select error " << std::endl; 
            continue;
        }

        if (activity == 0) continue; // timeout

        // If something happened on the socket, then its an incoming connection
        if (FD_ISSET(listenSockFD, &set)) {

            struct sockaddr addr;
            int err = makeSockAddr(connInfo.peerIP, connInfo.peerPort, addr);
            if (err) {
                std::cerr << "couldn't create sockAddr" << std::endl;
                continue;
            }

            int size = sizeof(addr);
            SOCKET newSockFd = accept(listenSockFD, &addr, &size);
            if (newSockFd < 0) {
                std::cerr << "accept error" << std::endl;
                continue;
            }
            std::cerr << "New Connection, socket fd is " << newSockFd << ", destIP is " << connInfo.peerIP
                        << ", port : " << connInfo.peerPort << std::endl;

            //! should not send anything on the socket. e.g. failure for HTTP expects and HTTP message
            // const char* message = "Welcome message \r\n";
            // // send new connection greeting message
            // if (send(newSockFd, message, strlen(message), 0) != strlen(message)) { perror("send"); }

            // std::cerr << "Welcome message sent successfully " << std::endl;

            std::shared_ptr<TCPConnection> newConn{new TCPConnection(connInfo)};
            newConn->connData_.sockfd = newSockFd;
            connections_.push_back(newConn);
            newConnection(newConn);
        }
    }
}