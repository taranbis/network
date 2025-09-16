#ifndef _TCP_CONNECTION_MANAGER_HEADER_HPP_
#define _TCP_CONNECTION_MANAGER_HEADER_HPP_ 1
#pragma once

#include <mutex>
#include <string>

#include <boost/signals2.hpp>

#include "tcp_connection.hpp"

class TargetedSignal
{
public:
    void connect(const SOCKET socket, std::function<void(TCPConnInfo)> slotFunc)
    {
        slots.push_back({socket, slotFunc});
    }

    void sendTo(const SOCKET targetSocket, TCPConnInfo tcpConnInfo)
    {
        for (const auto& ts : slots) {
            if (ts.socket == targetSocket) { ts.slot(tcpConnInfo); }
        }
    }

private:
    struct SocketSlot {
        SOCKET socket;
        std::function<void(TCPConnInfo)> slot;
    };

    std::vector<SocketSlot> slots;
};

class TCPConnectionManager
{
public:
    boost::signals2::signal<void(TCPConnInfo)> newConnection;
    boost::signals2::signal<void(TCPConnInfo)> connectionClosed;
    TargetedSignal newConnectionOnListeningSocket;

public:
    TCPConnectionManager();
    ~TCPConnectionManager();
    void stop();

    static std::string dnsLookup(const std::string& host, uint16_t ipVersion = 0); 

    TCPConnInfo openConnection(const std::string& destAddress, uint16_t destPort); 
    TCPConnInfo openConnection(const std::string& destAddress, uint16_t destPort,
                                                  const std::string& sourceAddress, uint16_t sourcePort);

    bool write(TCPConnInfo connData, const std::string& msg);
    TCPConnInfo openListenSocket(const std::string& ipAddr, uint16_t port);

    std::weak_ptr<TCPConnection> getConnection(const TCPConnInfo& connInfo) const
    {
        return m_connections.at(connInfo.sockfd);
    }

    void startReadingData(const TCPConnInfo& connInfo);
    void closeConn(const TCPConnInfo& connData);

private:
    void checkForConnections(const TCPConnInfo& connInfo);
    void readDataFromSocket(TCPConnInfo connInfo, std::stop_token token) const;

private:
    bool m_finish{false};
    bool m_printReceivedData{true};
    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::jthread m_readingThreadsCleaner;
    std::pair<bool, SOCKET> m_threadFinished = {false, -1};

    std::unordered_map<SOCKET, std::shared_ptr<TCPConnection>> m_connections;
    std::unordered_map<SOCKET, std::jthread> m_readingThreads;
    std::unordered_map<SOCKET, std::jthread> m_checkForConnectionsThreads;
};

#endif //!_TCP_HANDLER_HEADER_HPP_