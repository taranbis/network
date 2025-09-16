#include <chrono>
#include <format>
#include <future>
#include <iostream>
#include <thread>
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <conio.h>

const std::string hostAddr = "127.0.0.1";
const uint16_t hostPort = 12301;

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

inline void printErrorMessage()
{
    int errCode = WSAGetLastError();
    char* msgBuffer = nullptr;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errCode, 0, (LPSTR)&msgBuffer,
                  0, NULL);

    std::cerr << "Error " << errCode << ": " << (msgBuffer ? msgBuffer : "Unknown error") << std::endl;
    LocalFree(msgBuffer);
}

int main()
{
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err) {
        std::cerr << "WSAStartup failed: " << err << std::endl;
        return -1;
    }
    std::cout << "Winsock initialized.\n";

    const SOCKET serverSock = socket(hostAddr.find(".") == -1 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "couldn't create socket" << std::endl;
        return -1;
    }

    const int on = 1;
    err = setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    if (err) {
        std::cerr << "couldn't set option" << std::endl;
        closesocket(serverSock);
        return -1;
    }

    struct sockaddr addr;
    err = makeSockAddr(hostAddr, hostPort, addr);
    if (err) {
        std::cerr << "couldn't create sockAddr" << std::endl;
        return -1;
    }

    err = bind(serverSock, &addr, sizeof(addr));
    if (err) { // couldn't bind source address and port;
        printErrorMessage();
        closesocket(serverSock);
        return -1;
    }

    err = listen(serverSock, 10);
    if (err) {
        printErrorMessage();
        closesocket(serverSock);
        return -1;
    }

    std::cout << "Waiting for connection on " << serverSock << std::endl;
    const int addrlen = sizeof(addr);
    const SOCKET newSock = accept(serverSock, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
    if (newSock < 0) {
        printErrorMessage();
        return -1;
    }

    std::promise<int> p;
    std::cout << "Starting reading from " << newSock << std::endl;
    std::jthread th([&](std::stop_token st) {
        char buffer[1024] = {0};
        while (!st.stop_requested()) {
            const int recvRes = recv(newSock, buffer, 1024, 0);
            const std::string messageFromClient= std::format("Message from client: {}", buffer);
            std::cout << messageFromClient << std::endl;
            if (recvRes == SOCKET_ERROR) {
                std::cerr << std::format("receive failed on socket {}; closing connection!\n", newSock);
                p.set_value(1);
                break;
            }

            if (recvRes == 0) {
                std::clog << std::format("connection on socket {} was closed by peer\n", newSock);
                p.set_value(2);
                break;
            }

            const std::string messageToClient = std::format("TCP Server received message:{}", buffer);
            const int sendRes = send(newSock, messageToClient.data(), messageToClient.size(), 0);
        }
    });

    std::future<int> f = p.get_future();
    using namespace std::chrono_literals;
    while (f.wait_for(100ms) != std::future_status::ready) {
        if (_kbhit()) {
            char ch = _getch(); // Waits for keypress (non-blocking check)
            if (ch == 'q' || ch == 'Q') {
                std::cout << "Exiting...\n";
                break;
            }
        }
    }

    th.request_stop();

    closesocket(newSock);
    closesocket(serverSock);

    WSACleanup();
    return 0;
}