#ifndef _TCP_SERVER_HEADER_HPP_
#define _TCP_SERVER_HEADER_HPP_ 1
#pragma once

#include <format>

#include "tcp_connection_manager.hpp"

class TCPServer
{
public:
    TCPServer(TCPConnectionManager& tcpHandler) : m_tcpConnMgr(tcpHandler) {}

    void start(const std::string& sourceAddress, uint16_t sourcePort)
    {
        m_tcpConn = m_tcpConnMgr.openListenSocket(sourceAddress, sourcePort);
    }

    void broadcast(const std::string& message) {
        for (const std::weak_ptr<TCPConnection>& conn : m_tcpConnMgr.getConnections()) {
            if (std::shared_ptr<TCPConnection> connSpt = conn.lock()) connSpt->write(message);
        }
    }

    // TODO: add function that does sth when new connection appears;

private:
    TCPConnectionManager& m_tcpConnMgr;
    std::shared_ptr<TCPConnection> m_tcpConn;
};

#endif