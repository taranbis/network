#include "tcp_connection.hpp"

#include <string>
#include <thread>
#include <functional>
#include <iostream>
#include <format>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <boost/signals2.hpp>

#include "tcp_util.hpp"
#include "tcp_connection_manager.hpp"

TCPConnection::TCPConnection(TCPConnectionManager& tcpMgr, TCPConnInfo data) : m_tcpMgr(tcpMgr), connData_(data) {
}

TCPConnection::~TCPConnection()
{
    std::clog << std::format("TCP connection closing for socket {}\n", connData_.sockfd);
    stop();
};

void TCPConnection::stop()
{
    m_readingThread.request_stop();
    this->newBytesIncomed.disconnect_all_slots();
    closesocket(connData_.sockfd);
}

bool TCPConnection::write(const rmg::ByteArray& msg)
{
    // send returns the total number of bytes sent. Otherwise, a value of SOCKET_ERROR is returned
    return m_tcpMgr.write(connData_, msg);
}

void TCPConnection::startReadingData()
{
    m_tcpMgr.startReadingData(connData_);
}

TCPConnInfo& TCPConnection::connData()
{
    return connData_;
}