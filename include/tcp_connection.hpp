#ifndef _TCP_CONNECTION_HEADER_HPP_
#define _TCP_CONNECTION_HEADER_HPP_ 1
#pragma once

#include <thread>

#include <winsock2.h>

#include "connection.hpp"

class TCPConnectionManager;

struct TCPKeepAliveInfo {};

struct TCPConnInfo 
{
    SOCKET sockfd {};
    std::string peerIP;
    uint16_t peerPort;
};

class TCPConnection : public Connection
{
public:

    TCPConnection(TCPConnInfo data);
    TCPConnection(const TCPConnection& other) = delete;

    virtual ~TCPConnection();
    virtual void stop() override;
    virtual bool write(const rmg::ByteArray& msg) override;
    virtual void startReadingData() override;

    TCPConnInfo& connData();

private:
    // read data from socket and to socket should be here not in TCPConnectionManager
    void readDataFromSocket(std::stop_token st);

    // TODO: bug if socket is closed from the other side reading continues with nothing
protected:
    TCPConnInfo connData_{};
    bool printReceivedData{true};

private:
    std::jthread readingThread_{};
    friend class TCPConnectionManager;
};

#endif //!_TCP_CONNECTION_HEADER_HPP_