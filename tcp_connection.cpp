#include "tcp_connection.hpp"

#include <iostream>
#include <format>

#include <winsock2.h>

#include <boost/signals2.hpp>

#include "tcp_connection_manager.hpp"

TCPConnection::TCPConnection(TCPConnectionManager& tcpMgr, TCPConnInfo data) : m_tcpMgr(tcpMgr), connInfo_(data) {
}

TCPConnection::~TCPConnection()
{
    std::clog << std::format("TCP connection closing for socket {}\n", connInfo_.sockfd);
    stop();
};

void TCPConnection::stop()
{
    this->newDataArrived.disconnect_all_slots();
    closesocket(connInfo_.sockfd);
}

bool TCPConnection::write(const std::string& msg)
{
    // send returns the total number of bytes sent. Otherwise, a value of SOCKET_ERROR is returned
    return m_tcpMgr.write(connInfo_, msg);
}

void TCPConnection::startReadingData()
{
    m_tcpMgr.startReadingData(connInfo_);
}

TCPConnInfo& TCPConnection::connInfo()
{
    return connInfo_;
}