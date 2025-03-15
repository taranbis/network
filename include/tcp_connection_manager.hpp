#ifndef _TCP_CONNECTION_MANAGER_HEADER_HPP_
#define _TCP_CONNECTION_MANAGER_HEADER_HPP_ 1
#pragma once

#include <vector>

#include <boost/signals2.hpp>

#include "tcp_util.hpp"
#include "tcp_connection.hpp"

class TCPConnectionManager
{
public:
    boost::signals2::signal<void(std::shared_ptr<TCPConnection>)> newConnection;

public:
    ~TCPConnectionManager(); //server
    void stop(); //server

    std::string dnsLookup(const std::string& host, uint16_t ipVersion = 0); // helper. can be even static 

    //! it really sems that i don';t need the manager. everything can be taken by the TCPServer, some static and outside

    //can be left outside, but the class calling this need to be aware that it has to close the connection
    std::shared_ptr<TCPConnection> openConnection(const std::string& destAddress, uint16_t destPort); 
    std::shared_ptr<TCPConnection> openConnection(const std::string& destAddress, uint16_t destPort,
                                                  const std::string& sourceAddress, uint16_t sourcePort);

    //bool start(int listenSocket);
    //std::shared_ptr<TCPConnection> start(const std::string& ipAddr, uint16_t port);

    std::shared_ptr<TCPConnection> openListenSocket(const std::string& ipAddr, uint16_t port); // server

    const std::vector<std::weak_ptr<TCPConnection>>& getConnections() const {
        return m_connections;
    }

private:
    std::vector<std::weak_ptr<TCPConnection>> m_connections; // server
    bool m_finish{false};                                    // server

private:
    void checkForConnections(TCPConnection* conn); // server
};

#endif //!_TCP_HANDLER_HEADER_HPP_