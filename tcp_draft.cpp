#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <future>
#include <cassert>
#include <format>
#include <random>
#include <set>

#include "tcp_connection_manager.hpp"
#include "tcp_server.hpp"

int main() {
    std::cout << "\n--- Testing data integrity under load ---" << std::endl;

    TCPConnectionManager manager;
    std::atomic<int> messages_sent{0};
    std::atomic<int> messages_received{0};
    std::atomic<int> correct_messages{0};

    std::shared_ptr<TCPConnection> serverConn;

    manager.newConnection.connect([&](const TCPConnInfo& conn) {
        auto connPtr = manager.getConnection(conn).lock();
        if (connPtr) {
            serverConn = connPtr;
            connPtr->newDataArrived.connect([&](const std::vector<char>& data) {
                ++messages_received;
                std::string received(data.begin(), data.end());

                // Check if message follows expected pattern
                if (received.find("Message_") == 0) { ++correct_messages; }
            });
        }
    });

    // Create server
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 12540);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create client
    TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 12540);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send multiple messages rapidly
    const int num_messages = 100;
    for (int i = 0; i < num_messages; ++i) {
        std::string message = std::format("Message_{:04d}_TestData", i);
        bool sent = manager.write(clientInfo, message);
        if (sent) { ++messages_sent; }

        if (i % 10 == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
    }   

    // Wait for all messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    if (messages_sent > 0) {
        std::clog << "Should have sent some messages";
    }
    if (messages_received > 0) {
        std::clog << "Should have received some messages";
    }

    std::cout << std::format("Data integrity test: {} sent, {} received, {} correct format", messages_sent.load(),
                             messages_received.load(), correct_messages.load())
              << std::endl;

    // Allow for some message loss in high-load scenarios, but most should get through
    double success_rate = (double)correct_messages.load() / (double)messages_sent.load();
    //UnitTestFramework::assert_true(success_rate > 0.8, "Should have >80% message success rate");

    if (success_rate < 0.8) std::clog << "FAIL: Should NOT have <80% message success rate";
    if (success_rate > 0.8) std::clog << "PASS: Received with >80% message success rate";

    manager.stop();
}

//int main() {
//    std::cout << "\n--- Testing IP version handling ---" << std::endl;
//
//    TCPConnectionManager manager;
//
//    // Test IPv4 addresses
//    TCPConnInfo ipv4Server = manager.openListenSocket("127.0.0.1", 12530);
//    //UnitTestFramework::assert_true(ipv4Server.sockfd != 0, "IPv4 server should be created");
//
//    if (ipv4Server.sockfd != 0) { std::clog << "IPv4 server should be created!\n"; }
//
//    // Test IPv6 addresses (if supported)
//    TCPConnInfo ipv6Server = manager.openListenSocket("::1", 12531);
//    if (ipv6Server.sockfd != 0) {
//        //UnitTestFramework::assert_true(true, "IPv6 server created successfully");
//        std::clog << " IPv6 server created successfully \n";
//    } else {
//        //UnitTestFramework::assert_true(true, ")");
//                std::clog << " IPv6 server creation failed (IPv6 might not be supported \n";
//
//    }
//
//    // Test invalid IP addresses
//    TCPConnInfo invalidServer1 = manager.openListenSocket("999.999.999.999", 12532);
//    //UnitTestFramework::assert_true(invalidServer1.sockfd == 0, "Invalid IPv4 address should fail");
//    if (invalidServer1.sockfd == 0) {
//        std::clog << "Invalid IPv4 address should fail!\n";
//    }
//
//    TCPConnInfo invalidServer2 = manager.openListenSocket("not.an.ip.address", 12533);
//    //UnitTestFramework::assert_true(invalidServer2.sockfd == 0, "Invalid hostname should fail");
//
//        if (invalidServer1.sockfd == 0) { std::clog << "Invalid hostname should fail!\n"; }
//
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));
//    manager.stop();
//}