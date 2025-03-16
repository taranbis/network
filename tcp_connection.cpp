#include "tcp_connection.hpp"

#include <string>
#include <thread>
#include <functional>
#include <iostream>
#include <atomic>
#include <memory>
#include <format>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <boost/signals2.hpp>

#include "tcp_util.hpp"

TCPConnection::TCPConnection(TCPConnInfo data) : connData_(data) {}

TCPConnection::~TCPConnection()
{
    stop();
};

void TCPConnection::stop()
{
    readingThread_.request_stop();
    this->newBytesIncomed.disconnect_all_slots();
    closesocket(connData_.sockfd);
}

bool TCPConnection::write(const rmg::ByteArray& msg)
{
    // send returns the total number of bytes sent. Otherwise, a value of SOCKET_ERROR is returned
    int res = send(connData_.sockfd, msg.constData(), (int)msg.size(), 0);
    if (res == SOCKET_ERROR) {
        std::cerr << "send failed; msg: " << msg << ", error: " << WSAGetLastError() << "\n";
        printErrorMessage();
        return false;
    }
    return true;
}

void TCPConnection::startReadingData()
{
    //! had enormous bug because i detached thread!! Always jthread!!
    if (readingThread_.joinable()) return;
    readingThread_ = std::jthread([this](std::stop_token token) { this->readDataFromSocket(token); });
}

TCPConnInfo& TCPConnection::connData()
{
    return connData_;
}

// read data from socket and to socket should be here not in TCPConnectionManager
void TCPConnection::readDataFromSocket(std::stop_token st)
{
    std::unique_ptr<char> buffer(new char[1024]);
    while (!st.stop_requested()) {
        sockaddr_in addr;
        int addrLen = sizeof(addr);
        if (getpeername(connData_.sockfd, (sockaddr*)&addr, &addrLen) == SOCKET_ERROR) {
            std::cerr << "Socket is not connected! Error: " << WSAGetLastError() << std::endl;
        }

        WSAPOLLFD fd{};
        fd.fd = connData_.sockfd;
        fd.events = POLLIN; // Wait for incoming data

        int wsaPollRes = WSAPoll(&fd, 1, 2000); // 2 seconds timeout
        if (wsaPollRes > 0) {
            // If no error occurs, recv returns the number of bytes received and the buffer pointed to by the buf
            // parameter will
            // If the connection has been gracefully closed, the return value is zero.
            // Otherwise, a value of SOCKET_ERROR is returned
            const int recvRes = recv(connData_.sockfd, buffer.get(), 1024, 0);
            if (recvRes == SOCKET_ERROR) {
                std::cerr << std::format("receive error on socket {} \n", connData_.sockfd);
                printErrorMessage();
                stop();
                return;
            }

            if (recvRes == 0) {
                std::clog << std::format("connection on socket {} was closed by peer\n", connData_.sockfd);
                stop();
                //connClosed();
                return;
            }

            if (printReceivedData) {
                std::cout << "Number of bytes read: " << recvRes << std::endl;
                std::cout << "Message: " << std::endl;

                for (int i = 0; i < recvRes; ++i) std::cout << *(buffer.get() + i);
                std::cout << std::endl;
            }

            std::vector<char> rv;
            for (int i = 0; i < recvRes; ++i) rv.emplace_back(*(buffer.get() + i));
            newBytesIncomed(rv);
        } else if (wsaPollRes == 0) {
            std::cout << "Timeout: No data received.\n";
        } else {
            std::cerr << "WSAPoll() failed with error: " << WSAGetLastError() << std::endl;
        }
    }
}