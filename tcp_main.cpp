#include <iostream>
#include <format>
#include <thread>

#include "tcp_server.hpp"
/**
 * Connect to server to write messages using
 * telnet localhost 12301 or nc localhost 12301
 */

 void printingFunction(std::vector<char> buffer) {
    std::cout << "[main] ----- Number of bytes read: " << buffer.size() << std::endl;
    std::cout << "[main] ----- Message: ";

    for (const auto& b : buffer) { std::cout << b; }
    std::cout << std::endl;
}

int main()
{
    TCPConnectionManager handler;
    TCPServer server1(handler);
    server1.start("127.0.0.1", 12301);
    TCPServer server2(handler);
    server2.start("127.0.0.1", 12301);

    std::unordered_map<SOCKET, std::shared_ptr<TCPConnection>> connections;
    handler.newConnection.connect([&](const TCPConnInfo& conn) {
        std::weak_ptr connWPtr = handler.getConnection(conn);
        if (std::shared_ptr<TCPConnection> pTcpConn = connWPtr.lock()) {
            //std::cout << "*spt == " << *spt << '\n';
            connections.emplace(conn.sockfd, pTcpConn);
            pTcpConn->newDataArrived.connect(printingFunction);
        }
    });

    handler.connectionClosed.connect([&](const TCPConnInfo& conn) {
        connections.erase(conn.sockfd);
    });

    std::jthread server1ProducerThread([&](std::stop_token st) {
        static int i = 0;
        while (!st.stop_requested()) {
            std::string msg = "Message from Server1 no.: " + std::to_string(i++);
            server1.broadcast(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
        std::jthread server2ProducerThread([&](std::stop_token st) {
        static int i = 0;
        while (!st.stop_requested()) {
            std::string msg = "Message from Server2 no.: " + std::to_string(i++);
            server2.broadcast(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    //----------------------------- Client Code ---------------------------------------------
    TCPConnInfo client = handler.openConnection("127.0.0.1", 12301);
    if (client == TCPConnInfo{}) {
        std::cerr << "socket wasn't open" << std::endl;
        return -1;
    }

    //std::jthread clientProducerThread([&](std::stop_token st) {
    //    static int i = 0;
    //    while (!st.stop_requested()) {
    //        std::string msg = "msg no. " + std::to_string(i++);
    //        client->write(msg);
    //        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //    }
    //});
    //----------------------------------------------------------------------------------------

    server1.broadcast(std::format("Message broadcasted from server1 no"));

    while (std::cin.get() != 'q') {
    }
    server1.broadcast(std::format("Closing TCP Server1! Goodbye!"));
    server2.broadcast(std::format("Closing TCP Server2! Goodbye!"));
    server1ProducerThread.request_stop();
    server2ProducerThread.request_stop();
    // clientProducerThread.request_stop();
    connections.clear();
    handler.stop();

    return 0;
}