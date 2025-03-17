#ifndef _TCP_SERVER_HEADER_HPP_
#define _TCP_SERVER_HEADER_HPP_ 1
#pragma once

#include <format>
#include <set>

#include "tcp_connection_manager.hpp"

class TCPServer
{
public:
    TCPServer(TCPConnectionManager& tcpHandler) : m_tcpConnMgr(tcpHandler) {}

    void start(const std::string& sourceAddress, uint16_t sourcePort)
    {
        m_listenSockData = m_tcpConnMgr.openListenSocket(sourceAddress, sourcePort);
        m_tcpConnMgr.newConnection.connect([&](TCPConnInfo conn) { 
                m_connections.emplace(conn);
            });
    }

    void broadcast(const std::string& message) {
        std::vector<TCPConnInfo> tobeRemoved;
        for (const auto& conn : m_connections) { 
            if (conn.sockfd == m_listenSockData.sockfd) continue;
            int res = m_tcpConnMgr.write(conn, message);
            if (res) { 
                //tobeRemoved.emplace_back(conn);
            }
        }

        for (const auto& toRemove : tobeRemoved) { 
            m_connections.erase(toRemove);
        }
    }

    // TODO: add function that does sth when new connection appears;

private:
    TCPConnectionManager& m_tcpConnMgr;
    TCPConnInfo m_listenSockData;
    std::set<TCPConnInfo> m_connections;
};

#endif