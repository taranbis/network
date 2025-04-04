#ifndef _TCP_CONNECTION_MANAGER_HEADER_HPP_
#define _TCP_CONNECTION_MANAGER_HEADER_HPP_ 1
#pragma once

#include <vector>
#include <condition_variable>
#include <mutex>

#include <boost/signals2.hpp>

#include "tcp_util.hpp"
#include "tcp_connection.hpp"

class TCPConnectionManager
{
public:
    boost::signals2::signal<void(TCPConnInfo)> newConnection;
    boost::signals2::signal<void(TCPConnInfo)> connectionClosed;

public:
    TCPConnectionManager();
    ~TCPConnectionManager();
    void stop();

    // helper. can be even static 
    std::string dnsLookup(const std::string& host, uint16_t ipVersion = 0); 

    //can be left outside, but the class calling this need to be aware that it has to close the connection
    TCPConnInfo openConnection(const std::string& destAddress, uint16_t destPort); 
    TCPConnInfo openConnection(const std::string& destAddress, uint16_t destPort,
                                                  const std::string& sourceAddress, uint16_t sourcePort);

    bool write(TCPConnInfo connData, const rmg::ByteArray& msg);
    TCPConnInfo openListenSocket(const std::string& ipAddr, uint16_t port);

    std::weak_ptr<TCPConnection> getConnection(const TCPConnInfo& connInfo)
    {
        return m_connections.at(connInfo.sockfd);
    }
    //const std::vector<std::weak_ptr<TCPConnection>>& getConnections() const {
    //    return m_connections;
    //}

    void startReadingData(const TCPConnInfo& connInfo);
    void readDataFromSocket(TCPConnInfo connInfo, std::stop_token token);
    void closeConn(const TCPConnInfo& connData);

private:
    std::unordered_map<SOCKET, std::shared_ptr<TCPConnection>> m_connections;
    bool m_finish{false};
    bool m_printReceivedData{true};
    std::unordered_map<SOCKET, std::jthread> m_readingThreads;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::jthread m_readingThreadsCleaner;
    std::pair<bool, SOCKET> m_threadFinished = {false, -1};

    std::thread m_checkForConnectionsThread;
private:
    void checkForConnections(const TCPConnInfo& connInfo);
};

#endif //!_TCP_HANDLER_HEADER_HPP_