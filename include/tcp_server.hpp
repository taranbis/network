#ifndef _TCP_SERVER_HEADER_HPP_
#define _TCP_SERVER_HEADER_HPP_ 1
#pragma once

#include <set>

#include "tcp_connection_manager.hpp"
class TCPServer
{
public:
    TCPServer(TCPConnectionManager& tcpConnMgr) : m_tcpConnMgr(tcpConnMgr) {}

    void start(const std::string& sourceAddress, uint16_t sourcePort)
    {
        m_listenSockInfo = m_tcpConnMgr.openListenSocket(sourceAddress, sourcePort);
        m_tcpConnMgr.newConnectionOnListeningSocket.connect(
                    m_listenSockInfo.sockfd,
                    [&](TCPConnInfo conn) { 
                m_connections.emplace(conn);
            });
        m_tcpConnMgr.connectionClosed.connect([&](TCPConnInfo conn) { 
                m_connections.erase(conn);
            });
    }

    void broadcast(const std::string& message) {
        for (const auto& conn : m_connections) { 
            m_tcpConnMgr.write(conn, message);
        }
    }

private:
    TCPConnectionManager&   m_tcpConnMgr;
    TCPConnInfo             m_listenSockInfo;
    std::set<TCPConnInfo>   m_connections;
};

#endif