#include "tcp_connection.hpp"

#include <string>
#include <thread>
#include <functional>
#include <iostream>
#include <atomic>
#include <memory>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <boost/signals2.hpp>

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
    if (send(connData_.sockfd, msg.constData(), (int)msg.size(), 0) < 0) {
        std::perror("send failed");
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
        int valread = recv(connData_.sockfd, buffer.get(), 1024, 0);
        if (valread != -1) {
            if (valread == 0) {
                stop();
                return;
            }
            if (printReceivedData) {
                std::cout << "Number of bytes read: " << valread << std::endl;
                std::cout << "Message: " << std::endl;

                for (int i = 0; i < valread; ++i) std::cout << *(buffer.get() + i);
                std::cout << std::endl;
            }

            std::vector<char> rv;
            for (int i = 0; i < valread; ++i) rv.emplace_back(*(buffer.get() + i));
            newBytesIncomed(rv);
        }
    }
}