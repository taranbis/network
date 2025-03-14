#ifndef _TCP_SERVER_HEADER_HPP_
#define _TCP_SERVER_HEADER_HPP_ 1
#pragma once

#include "tcp_connection_manager.hpp"

class TCPServer
{
private:
    TCPConnectionManager& tcpHandler_;
    std::shared_ptr<TCPConnection> tcpConn_;

public:
    TCPServer(TCPConnectionManager& tcpHandler) : tcpHandler_(tcpHandler) {}

    void start(const std::string& sourceAddress, uint16_t sourcePort)
    {
        tcpConn_ = tcpHandler_.openListenSocket(sourceAddress, sourcePort);
    }

    // TODO: add function that does sth when new connection appears;
};

#endif