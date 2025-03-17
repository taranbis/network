#include "tcp_server.hpp"

#include <iostream>
#include <thread>

/**
 * Since I am starting a server one can connect to write messages using
 * telnet localhost 12301
 * or
 * nc localhost 12301
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
    TCPServer server(handler);
    server.start("127.0.0.1", 12301);

    //std::vector<std::shared_ptr<TCPConnection>> connections;
    //handler.newConnection.connect([/*&connections*/](const TCPConnInfo& conn) {
    //    conn->newBytesIncomed.connect(printingFunction);
    //    conn->startReadingData();
    //    //connections.emplace_back(conn);
    //});

    std::jthread serverProducerThread([&](std::stop_token st) {
        static int i = 0;
        while (!st.stop_requested()) {
            std::string msg = "Message from Server no.: " + std::to_string(i++);
            server.broadcast(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    //----------------------------- Client Code ------------------ ---------------------------
    TCPConnInfo client = handler.openConnection("127.0.0.1", 12301);
    //if (!client) {
    //    std::cerr << "socket wasn't open" << std::endl;
    //    return -1;
    //}

    //std::jthread clientProducerThread([&](std::stop_token st) {
    //    static int i = 0;
    //    while (!st.stop_requested()) {
    //        std::string msg = "msg no. " + std::to_string(i++);
    //        client->write(msg);
    //        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //    }
    //});
    //----------------------------------------------------------------------------------------

    server.broadcast(std::format("Message broadcasted from server no"));

    if (std::cin.get() == 'n') {
        server.broadcast(std::format("Closing TCP Server! Goodbye!"));

        serverProducerThread.request_stop();
        //clientProducerThread.request_stop();
        handler.stop();
    }

    return 0;
}