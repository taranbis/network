#ifndef _TCP_CONNECTION_HEADER_HPP_
#define _TCP_CONNECTION_HEADER_HPP_ 1
#pragma once

#include <thread>

#include <winsock2.h>

#include "connection.hpp"

class TCPConnectionManager;

//will use later
struct TCPKeepAliveInfo {};

struct TCPConnInfo 
{
    SOCKET sockfd {};
    std::string peerIP;
    uint16_t peerPort;

    //for std::set in tcp_server
    auto operator<=>(const TCPConnInfo& other) const = default;
};

class TCPConnection : public Connection
{
public:
    TCPConnection(TCPConnectionManager& tcpMgr, TCPConnInfo data);
    TCPConnection(const TCPConnection& other) = delete;

    virtual ~TCPConnection();
    virtual void stop() override;
    virtual bool write(const rmg::ByteArray& msg) override;
    virtual void startReadingData() override;

    TCPConnInfo& connData();

protected:
    TCPConnInfo connData_{};

private:
    TCPConnectionManager& m_tcpMgr;
    friend class TCPConnectionManager;
    std::jthread m_readingThread{};
    std::weak_ptr<TCPConnection> m_weakSelf;
};

#endif //!_TCP_CONNECTION_HEADER_HPP_