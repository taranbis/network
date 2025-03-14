#ifndef _TCP_UTIL_HEADER_HPP_
#define _TCP_UTIL_HEADER_HPP_ 1
#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

inline int makeSockAddr(const std::string& ipAddr, uint16_t port, sockaddr& addr)
{
    memset(&addr, 0, sizeof(addr)); // Clear memory

    if (ipAddr.find(".") == -1) {
        sockaddr_in6* ipv6 = (sockaddr_in6*)&addr;
        ipv6->sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, ipAddr.c_str(), &ipv6->sin6_addr) == 0) { return -1; }
        ipv6->sin6_port = htons(port);

        return 0;
    }
    sockaddr_in* ipv4 = (sockaddr_in*)&addr;
    memset(&addr, 0, sizeof(addr));
    ipv4->sin_family = AF_INET;
    if (inet_pton(AF_INET, ipAddr.c_str(), &ipv4->sin_addr) == 0) { return -1; }
    ipv4->sin_port = htons(port);

    return 0;
}

#endif