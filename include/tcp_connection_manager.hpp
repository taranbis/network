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
    bool finish{false};

public:
    ~TCPConnectionManager();
    void stop();

    std::string dnsLookup(const std::string& host, uint16_t ipVersion = 0);

    std::shared_ptr<TCPConnection> openConnection(const std::string& destAddress, uint16_t destPort);
    std::shared_ptr<TCPConnection> openConnection(const std::string& destAddress, uint16_t destPort,
                                                  const std::string& sourceAddress, uint16_t sourcePort);

    //bool start(int listenSocket);
    //std::shared_ptr<TCPConnection> start(const std::string& ipAddr, uint16_t port);

    std::shared_ptr<TCPConnection> openListenSocket(const std::string& ipAddr, uint16_t port);

private:
    std::vector<std::weak_ptr<TCPConnection>> connections_;

private:
    void checkForConnections(TCPConnection* conn);
};

#endif //!_TCP_HANDLER_HEADER_HPP_