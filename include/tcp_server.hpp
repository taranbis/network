#ifndef _TCP_SERVER_HEADER_HPP_
#define _TCP_SERVER_HEADER_HPP_ 1
#pragma once

#include <set>

#include "tcp_connection_manager.hpp"

class TCPServer
{
public:
    TCPServer(TCPConnectionManager& tcpHandler) : m_tcpConnMgr(tcpHandler) {}

    void start(const std::string& sourceAddress, uint16_t sourcePort)
    {
        m_listenSockInfo = m_tcpConnMgr.openListenSocket(sourceAddress, sourcePort);
        m_tcpConnMgr.newConnection.connect([&](TCPConnInfo conn) { 
                m_connections.emplace(conn);
            });
        m_tcpConnMgr.connectionClosed.connect([&](TCPConnInfo conn) { 
                m_connections.erase(conn);
            });
    }

    void broadcast(const std::string& message) {
        std::vector<TCPConnInfo> tobeRemoved;
        for (const auto& conn : m_connections) { 
            //should not be the case
            //if (conn.sockfd == m_listenSockInfo.sockfd) continue;
            int res = m_tcpConnMgr.write(conn, message);
            if (res) { 
                //tobeRemoved.emplace_back(conn);
                //std::clog << "coulnd't write to sock: " << conn.sockfd << std::endl;
            }
        }
    }

private:
    TCPConnectionManager&   m_tcpConnMgr;
    TCPConnInfo             m_listenSockInfo;
    std::set<TCPConnInfo>   m_connections;
};

#endif