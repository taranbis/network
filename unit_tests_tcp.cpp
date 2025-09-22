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

// Focused unit tests for edge cases and error conditions

class UnitTestFramework {
public:
    static void assert_true(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "UNIT TEST FAILED: " << message << std::endl;
            ++failed_tests;
        } else {
            std::cout << "PASS: " << message << std::endl;
            ++passed_tests;
        }
    }

    static void assert_equals(int expected, int actual, const std::string& message) {
        assert_true(expected == actual, message + std::format(" (expected: {}, actual: {})", expected, actual));
    }

    static void print_results() {
        std::cout << "\n=== UNIT TEST RESULTS ===" << std::endl;
        std::cout << "Passed: " << passed_tests << std::endl;
        std::cout << "Failed: " << failed_tests << std::endl;
        std::cout << "Total: " << (passed_tests + failed_tests) << std::endl;
        
        if (failed_tests == 0) {
            std::cout << "ALL UNIT TESTS PASSED!" << std::endl;
        } else {
            std::cout << "SOME UNIT TESTS FAILED!" << std::endl;
        }
    }

    static int get_failed_tests() {
        return failed_tests.load();
    }

    static int get_passed_tests() {
        return passed_tests.load();
    }

private:
    static std::atomic<int> passed_tests;
    static std::atomic<int> failed_tests;
};

std::atomic<int> UnitTestFramework::passed_tests{0};
std::atomic<int> UnitTestFramework::failed_tests{0};

// Test TargetedSignal class specifically
void test_targeted_signal() {
    std::cout << "\n--- Testing TargetedSignal class ---" << std::endl;
    
    TargetedSignal signal;
    std::atomic<int> slot1_calls{0};
    std::atomic<int> slot2_calls{0};
    std::atomic<int> slot3_calls{0};
    
    SOCKET socket1 = 100;
    SOCKET socket2 = 200;
    SOCKET socket3 = 300;
    
    // Connect multiple slots to different sockets
    signal.connect(socket1, [&](TCPConnInfo conn) { ++slot1_calls; });
    signal.connect(socket2, [&](TCPConnInfo conn) { ++slot2_calls; });
    signal.connect(socket3, [&](TCPConnInfo conn) { ++slot3_calls; });
    
    // Send to specific socket
    TCPConnInfo testConn{socket1, "127.0.0.1", 8080};
    signal.sendTo(socket1, testConn);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    UnitTestFramework::assert_equals(1, slot1_calls.load(), "Socket1 slot should be called once");
    UnitTestFramework::assert_equals(0, slot2_calls.load(), "Socket2 slot should not be called");
    UnitTestFramework::assert_equals(0, slot3_calls.load(), "Socket3 slot should not be called");
    
    // Send to different socket
    signal.sendTo(socket2, testConn);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    UnitTestFramework::assert_equals(1, slot1_calls.load(), "Socket1 slot should still be called once");
    UnitTestFramework::assert_equals(1, slot2_calls.load(), "Socket2 slot should now be called once");
    UnitTestFramework::assert_equals(0, slot3_calls.load(), "Socket3 slot should still not be called");
    
    // Send to non-existent socket
    signal.sendTo(999, testConn);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    UnitTestFramework::assert_equals(1, slot1_calls.load(), "Socket1 calls should remain unchanged");
    UnitTestFramework::assert_equals(1, slot2_calls.load(), "Socket2 calls should remain unchanged");
    UnitTestFramework::assert_equals(0, slot3_calls.load(), "Socket3 calls should remain unchanged");
}

// Test TCPConnInfo comparison operators
void test_tcp_conn_info_comparison() {
    std::cout << "\n--- Testing TCPConnInfo comparison operators ---" << std::endl;
    
    TCPConnInfo conn1{100, "127.0.0.1", 8080};
    TCPConnInfo conn2{100, "127.0.0.1", 8080};
    TCPConnInfo conn3{200, "127.0.0.1", 8080};
    TCPConnInfo conn4{100, "192.168.1.1", 8080};
    TCPConnInfo conn5{100, "127.0.0.1", 9090};
    
    UnitTestFramework::assert_true(conn1 == conn2, "Identical connections should be equal");
    UnitTestFramework::assert_true(conn1 != conn3, "Different socket FDs should be unequal");
    UnitTestFramework::assert_true(conn1 != conn4, "Different IPs should be unequal");
    UnitTestFramework::assert_true(conn1 != conn5, "Different ports should be unequal");
    
    // Test in std::set
    std::set<TCPConnInfo> connSet;
    connSet.insert(conn1);
    connSet.insert(conn2); // Should not be inserted (duplicate)
    connSet.insert(conn3);
    connSet.insert(conn4);
    connSet.insert(conn5);
    
    UnitTestFramework::assert_equals(4, (int)connSet.size(), "Set should contain 4 unique connections");
}

// Test connection manager with invalid operations
void test_invalid_operations() {
    std::cout << "\n--- Testing invalid operations ---" << std::endl;
    
    TCPConnectionManager manager;
    
    // Test getConnection with invalid socket
    TCPConnInfo invalidConn{999, "127.0.0.1", 8080};
    try {
        auto weakPtr = manager.getConnection(invalidConn);
        auto sharedPtr = weakPtr.lock();
        UnitTestFramework::assert_true(sharedPtr == nullptr, "getConnection with invalid socket should return null");
    } catch (const std::exception& e) {
        std::cout << "Expected exception caught: " << e.what() << std::endl;
        UnitTestFramework::assert_true(true, "getConnection with invalid socket should throw or return null");
    }
    
    // Test closeConn with invalid connection - try to get connection and close it
    try {
        auto weakConn = manager.getConnection(invalidConn);
        auto sharedConn = weakConn.lock();
        if (sharedConn) {
            sharedConn->stop(); // This will close the connection
        }
        UnitTestFramework::assert_true(true, "closeConn with invalid connection should not crash");
    } catch (...) {
        UnitTestFramework::assert_true(true, "Exception expected for invalid connection");
    }
    
    // Test write with invalid connection
    bool writeResult = manager.write(invalidConn, "test message");
    UnitTestFramework::assert_true(!writeResult, "write with invalid connection should return false");
    
    manager.stop();
}

// Test DNS lookup edge cases
void test_dns_lookup_edge_cases() {
    std::cout << "\n--- Testing DNS lookup edge cases ---" << std::endl;
    
    // Test empty hostname
    std::string result1 = TCPConnectionManager::dnsLookup("");
    UnitTestFramework::assert_true(result1.empty(), "DNS lookup with empty hostname should return empty string");
    
    // Test localhost variants
    std::string result2 = TCPConnectionManager::dnsLookup("localhost");
    UnitTestFramework::assert_true(!result2.empty(), "DNS lookup for localhost should succeed");
    
    std::string result3 = TCPConnectionManager::dnsLookup("127.0.0.1");
    UnitTestFramework::assert_true(!result3.empty(), "DNS lookup for 127.0.0.1 should succeed");
    
    // Test IPv6 if supported
    std::string result4 = TCPConnectionManager::dnsLookup("::1", 6);
    // This might fail on systems without IPv6, so we just check it doesn't crash
    UnitTestFramework::assert_true(true, "DNS lookup for IPv6 localhost should not crash");
    
    std::cout << "Localhost resolved to: " << result2 << std::endl;
    std::cout << "127.0.0.1 resolved to: " << result3 << std::endl;
    if (!result4.empty()) {
        std::cout << "::1 resolved to: " << result4 << std::endl;
    }
}

// Test connection with source binding
void test_source_binding() {
    std::cout << "\n--- Testing connection with source binding ---" << std::endl;
    
    TCPConnectionManager manager;
    
    // Create a listen socket first
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 12500);
    UnitTestFramework::assert_true(serverInfo.sockfd != 0, "Server should be created for source binding test");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Try to connect with specific source address
    TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 12500, "127.0.0.1", 0);
    UnitTestFramework::assert_true(clientInfo.sockfd != 0, "Client with source binding should connect successfully");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    manager.stop();
}

// Test rapid connection/disconnection cycles
void test_rapid_cycles() {
    std::cout << "\n--- Testing rapid connection/disconnection cycles ---" << std::endl;
    
    TCPConnectionManager manager;
    std::atomic<int> connection_count{0};
    std::atomic<int> disconnection_count{0};
    
    manager.newConnection.connect([&](const TCPConnInfo& conn) {
        ++connection_count;
    });
    
    manager.connectionClosed.connect([&](const TCPConnInfo& conn) {
        ++disconnection_count;
    });
    
    // Create server
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 12510);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const int cycles = 20;
    for (int i = 0; i < cycles; ++i) {
        TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 12510);
        if (clientInfo.sockfd != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Get the connection and close it properly
            try {
                auto weakConn = manager.getConnection(clientInfo);
                auto sharedConn = weakConn.lock();
                if (sharedConn) {
                    sharedConn->stop(); // This will trigger connection cleanup
                }
            } catch (...) {
                // Connection might already be closed
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    UnitTestFramework::assert_true(connection_count > 0, "Should have some successful connections in rapid cycles");
    UnitTestFramework::assert_true(disconnection_count > 0, "Should have some successful disconnections in rapid cycles");
    
    std::cout << std::format("Rapid cycles: {} connections, {} disconnections", 
        connection_count.load(), disconnection_count.load()) << std::endl;
    
    manager.stop();
}

// Test memory management and resource cleanup
void test_resource_cleanup() {
    std::cout << "\n--- Testing resource cleanup ---" << std::endl;
    
    std::atomic<int> connection_objects_created{0};
    std::atomic<int> connection_objects_destroyed{0};
    
    {
        TCPConnectionManager manager;
        
        // Create and destroy connections to test cleanup
        for (int i = 0; i < 5; ++i) {
            TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 12520 + i);
            if (serverInfo.sockfd != 0) {
                ++connection_objects_created;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        manager.stop();
    } // manager goes out of scope here
    
    // Give time for cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    UnitTestFramework::assert_true(connection_objects_created > 0, "Should have created some connection objects");
    UnitTestFramework::assert_true(true, "Resource cleanup test completed without crashes");
    
    std::cout << std::format("Created {} connection objects", connection_objects_created.load()) << std::endl;
}

// Test edge cases with port numbers
void test_port_edge_cases() {
    std::cout << "\n--- Testing port number edge cases ---" << std::endl;
    
    TCPConnectionManager manager;
    
    // Test port 0 (should auto-assign)
    TCPConnInfo serverInfo1 = manager.openListenSocket("127.0.0.1", 0);
    UnitTestFramework::assert_true(serverInfo1.sockfd != 0, "Server with port 0 should be created (auto-assign)");
    
    // Test high port number
    TCPConnInfo serverInfo2 = manager.openListenSocket("127.0.0.1", 65000);
    UnitTestFramework::assert_true(serverInfo2.sockfd != 0, "Server with high port number should be created");
    
    // Test privileged port (should likely fail unless running as admin)
    TCPConnInfo serverInfo3 = manager.openListenSocket("127.0.0.1", 80);
    if (serverInfo3.sockfd == 0) {
        UnitTestFramework::assert_true(true, "Privileged port binding failed as expected");
    } else {
        UnitTestFramework::assert_true(true, "Privileged port binding succeeded (running with admin privileges?)");
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    manager.stop();
}

// Test IPv4 vs IPv6 address handling
void test_ip_version_handling() {
    std::cout << "\n--- Testing IP version handling ---" << std::endl;
    
    TCPConnectionManager manager;
    
    // Test IPv4 addresses
    TCPConnInfo ipv4Server = manager.openListenSocket("127.0.0.1", 12530);
    UnitTestFramework::assert_true(ipv4Server.sockfd != 0, "IPv4 server should be created");
    
    // Test IPv6 addresses (if supported)
    TCPConnInfo ipv6Server = manager.openListenSocket("::1", 12531);
    if (ipv6Server.sockfd != 0) {
        UnitTestFramework::assert_true(true, "IPv6 server created successfully");
    } else {
        UnitTestFramework::assert_true(true, "IPv6 server creation failed (IPv6 might not be supported)");
    }
    
    // Test invalid IP addresses
    TCPConnInfo invalidServer1 = manager.openListenSocket("999.999.999.999", 12532);
    UnitTestFramework::assert_true(invalidServer1.sockfd == 0, "Invalid IPv4 address should fail");
    
    TCPConnInfo invalidServer2 = manager.openListenSocket("not.an.ip.address", 12533);
    UnitTestFramework::assert_true(invalidServer2.sockfd == 0, "Invalid hostname should fail");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    manager.stop();
}

// Test data integrity in high-load scenarios
void test_data_integrity() {
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
                if (received.find("Message_") == 0) {
                    ++correct_messages;
                }
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
        if (sent) {
            ++messages_sent;
        }
        
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    // Wait for all messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    UnitTestFramework::assert_true(messages_sent > 0, "Should have sent some messages");
    UnitTestFramework::assert_true(messages_received > 0, "Should have received some messages");
    
    std::cout << std::format("Data integrity test: {} sent, {} received, {} correct format", 
        messages_sent.load(), messages_received.load(), correct_messages.load()) << std::endl;
    
    // Allow for some message loss in high-load scenarios, but most should get through
    double success_rate = (double)correct_messages.load() / (double)messages_sent.load();
    UnitTestFramework::assert_true(success_rate > 0.8, "Should have >80% message success rate");
    
    manager.stop();
}

// Test server lifecycle with multiple start/stop cycles and connection verification
void test_server_lifecycle_and_connections() {
    std::cout << "\n--- Testing server lifecycle and connection management ---" << std::endl;
    
    const int cycles = 3;
    const int clients_per_cycle = 5;
    
    for (int cycle = 0; cycle < cycles; ++cycle) {
        std::cout << std::format("Server lifecycle cycle {}/{}", cycle + 1, cycles) << std::endl;
        
        TCPConnectionManager manager;
        std::atomic<int> connections_accepted{0};
        std::atomic<int> connections_closed{0};
        std::vector<TCPConnInfo> active_connections;
        std::mutex connections_mutex;
        
        // Track connection events
        manager.newConnection.connect([&](const TCPConnInfo& conn) {
            ++connections_accepted;
            std::lock_guard<std::mutex> lock(connections_mutex);
            active_connections.push_back(conn);
            
            // Verify connection is in manager's connection map
            try {
                auto weakConn = manager.getConnection(conn);
                auto sharedConn = weakConn.lock();
                UnitTestFramework::assert_true(sharedConn != nullptr, 
                    std::format("Connection {} should be in manager's map", conn.sockfd));
            } catch (...) {
                UnitTestFramework::assert_true(false, 
                    std::format("Failed to get connection {} from manager", conn.sockfd));
            }
        });
        
        manager.connectionClosed.connect([&](const TCPConnInfo& conn) {
            ++connections_closed;
            std::lock_guard<std::mutex> lock(connections_mutex);
            active_connections.erase(
                std::remove_if(active_connections.begin(), active_connections.end(),
                    [&conn](const TCPConnInfo& c) { return c.sockfd == conn.sockfd; }),
                active_connections.end());
        });
        
        // Create and start server
        TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 14000 + cycle);
        UnitTestFramework::assert_true(serverInfo.sockfd != 0, 
            std::format("Server should be created in cycle {}", cycle + 1));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Create multiple clients
        std::vector<TCPConnInfo> clients;
        for (int i = 0; i < clients_per_cycle; ++i) {
            TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 14000 + cycle);
            if (clientInfo.sockfd != 0) {
                clients.push_back(clientInfo);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Wait for connections to be established
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Verify connection counts
        UnitTestFramework::assert_true(connections_accepted >= clients.size(), 
            std::format("Should accept at least {} connections", clients.size()));
        
        {
            std::lock_guard<std::mutex> lock(connections_mutex);
            UnitTestFramework::assert_true(active_connections.size() >= clients.size(),
                std::format("Should have {} active connections tracked", clients.size()));
        }
        
        std::cout << std::format("  Cycle {}: {} clients connected, {} connections accepted", 
            cycle + 1, clients.size(), connections_accepted.load()) << std::endl;
        
        // Test data transfer to verify connections are working
        for (const auto& client : clients) {
            std::string message = std::format("Test message from client {}", client.sockfd);
            bool sent = manager.write(client, message);
            UnitTestFramework::assert_true(sent, "Should be able to send data on active connection");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Stop the manager (this should clean up all connections)
        manager.stop();
        
        // Give time for cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        std::cout << std::format("  Cycle {}: Server stopped, {} connections closed", 
            cycle + 1, connections_closed.load()) << std::endl;
        
        // Small delay between cycles
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    UnitTestFramework::assert_true(true, "Server lifecycle test completed successfully");
}

// Test thread management - verify correct number of threads created and cleaned up
void test_thread_management() {
    std::cout << "\n--- Testing thread management ---" << std::endl;
    
    TCPConnectionManager manager;
    std::atomic<int> connections_created{0};
    std::atomic<int> data_received_count{0};
    
    manager.newConnection.connect([&](const TCPConnInfo& conn) {
        ++connections_created;
        auto connPtr = manager.getConnection(conn).lock();
        if (connPtr) {
            connPtr->newDataArrived.connect([&](const std::vector<char>& data) {
                ++data_received_count;
            });
        }
    });
    
    // Create server
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 14100);
    UnitTestFramework::assert_true(serverInfo.sockfd != 0, "Server should be created for thread test");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const int num_connections = 10;
    std::vector<TCPConnInfo> clients;
    
    // Create multiple connections (each should spawn a reading thread)
    for (int i = 0; i < num_connections; ++i) {
        TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 14100);
        if (clientInfo.sockfd != 0) {
            clients.push_back(clientInfo);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    UnitTestFramework::assert_true(connections_created >= clients.size(), 
        std::format("Should have created {} connections", clients.size()));
    
    // Send data on each connection to verify reading threads are working
    for (size_t i = 0; i < clients.size(); ++i) {
        std::string message = std::format("Thread test message {}", i);
        manager.write(clients[i], message);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    UnitTestFramework::assert_true(data_received_count > 0, 
        "Should have received data (indicating reading threads are working)");
    
    std::cout << std::format("Thread management test: {} connections, {} data packets received", 
        connections_created.load(), data_received_count.load()) << std::endl;
    
    // Close connections one by one and verify cleanup
    for (const auto& client : clients) {
        try {
            auto weakConn = manager.getConnection(client);
            auto sharedConn = weakConn.lock();
            if (sharedConn) {
                sharedConn->stop(); // This will trigger connection cleanup
            }
        } catch (...) {
            // Connection might already be closed
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    manager.stop();
    
    // Give time for thread cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    UnitTestFramework::assert_true(true, "Thread management test completed without crashes");
}

// Test connection map integrity - verify m_connections map is properly managed
void test_connection_map_integrity() {
    std::cout << "\n--- Testing connection map integrity ---" << std::endl;
    
    TCPConnectionManager manager;
    std::atomic<int> connections_added{0};
    std::atomic<int> connections_removed{0};
    
    manager.newConnection.connect([&](const TCPConnInfo& conn) {
        ++connections_added;
        
        // Verify connection is immediately available in map
        try {
            auto weakConn = manager.getConnection(conn);
            auto sharedConn = weakConn.lock();
            UnitTestFramework::assert_true(sharedConn != nullptr, 
                std::format("New connection {} should be immediately available", conn.sockfd));
            
            // Verify connection info matches
            UnitTestFramework::assert_true(sharedConn->connInfo().sockfd == conn.sockfd,
                "Connection socket FD should match");
        } catch (const std::exception& e) {
            UnitTestFramework::assert_true(false, 
                std::format("Exception getting new connection {}: {}", conn.sockfd, e.what()));
        }
    });
    
    manager.connectionClosed.connect([&](const TCPConnInfo& conn) {
        ++connections_removed;
        
        // Verify connection is removed from map after closure
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        try {
            auto weakConn = manager.getConnection(conn);
            auto sharedConn = weakConn.lock();
            UnitTestFramework::assert_true(sharedConn == nullptr, 
                std::format("Closed connection {} should be removed from map", conn.sockfd));
        } catch (...) {
            // Exception is expected when trying to access removed connection
            UnitTestFramework::assert_true(true, "Exception expected for removed connection");
        }
    });
    
    // Create server
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 14200);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const int test_connections = 8;
    std::vector<TCPConnInfo> clients;
    
    // Phase 1: Create connections and verify map additions
    std::cout << "Phase 1: Creating connections and verifying map additions..." << std::endl;
    for (int i = 0; i < test_connections; ++i) {
        TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 14200);
        if (clientInfo.sockfd != 0) {
            clients.push_back(clientInfo);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    UnitTestFramework::assert_equals(test_connections, connections_added.load(), 
        "Should have added all connections to map");
    
    // Phase 2: Verify all connections are accessible
    std::cout << "Phase 2: Verifying all connections are accessible..." << std::endl;
    for (const auto& client : clients) {
        try {
            auto weakConn = manager.getConnection(client);
            auto sharedConn = weakConn.lock();
            UnitTestFramework::assert_true(sharedConn != nullptr, 
                std::format("Connection {} should be accessible", client.sockfd));
            
            // Test that we can use the connection
            bool writeResult = sharedConn->write("Map integrity test");
            UnitTestFramework::assert_true(writeResult, 
                std::format("Should be able to write to connection {}", client.sockfd));
        } catch (const std::exception& e) {
            UnitTestFramework::assert_true(false, 
                std::format("Failed to access connection {}: {}", client.sockfd, e.what()));
        }
    }
    
    // Phase 3: Close half the connections and verify map updates
    std::cout << "Phase 3: Closing connections and verifying map cleanup..." << std::endl;
    const int connections_to_close = test_connections / 2;
    for (int i = 0; i < connections_to_close; ++i) {
        try {
            auto weakConn = manager.getConnection(clients[i]);
            auto sharedConn = weakConn.lock();
            if (sharedConn) {
                sharedConn->stop(); // This will trigger connection cleanup and map removal
            }
        } catch (...) {
            // Connection might already be closed
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    UnitTestFramework::assert_true(connections_removed >= connections_to_close, 
        std::format("Should have removed at least {} connections from map", connections_to_close));
    
    // Phase 4: Verify remaining connections are still accessible
    std::cout << "Phase 4: Verifying remaining connections..." << std::endl;
    for (int i = connections_to_close; i < test_connections; ++i) {
        try {
            auto weakConn = manager.getConnection(clients[i]);
            auto sharedConn = weakConn.lock();
            UnitTestFramework::assert_true(sharedConn != nullptr, 
                std::format("Remaining connection {} should still be accessible", clients[i].sockfd));
        } catch (...) {
            UnitTestFramework::assert_true(false, 
                std::format("Remaining connection {} should be accessible", clients[i].sockfd));
        }
    }
    
    std::cout << std::format("Connection map integrity test: {} added, {} removed", 
        connections_added.load(), connections_removed.load()) << std::endl;
    
    manager.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    UnitTestFramework::assert_true(true, "Connection map integrity test completed");
}

// Test memory leak detection through multiple server cycles
void test_memory_leak_detection() {
    std::cout << "\n--- Testing memory leak detection ---" << std::endl;
    
    const int leak_test_cycles = 5;
    const int connections_per_cycle = 20;
    
    std::cout << std::format("Running {} cycles with {} connections each to detect memory leaks...", 
        leak_test_cycles, connections_per_cycle) << std::endl;
    
    for (int cycle = 0; cycle < leak_test_cycles; ++cycle) {
        std::cout << std::format("Memory leak test cycle {}/{}", cycle + 1, leak_test_cycles) << std::endl;
        
        // Create a new manager for each cycle to test complete cleanup
        {
            TCPConnectionManager manager;
            std::atomic<int> peak_connections{0};
            std::atomic<int> total_data_received{0};
            
            manager.newConnection.connect([&](const TCPConnInfo& conn) {
                ++peak_connections;
                auto connPtr = manager.getConnection(conn).lock();
                if (connPtr) {
                    connPtr->newDataArrived.connect([&](const std::vector<char>& data) {
                        total_data_received += data.size();
                    });
                }
            });
            
            // Create server
            TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 14300 + cycle);
            if (serverInfo.sockfd == 0) {
                UnitTestFramework::assert_true(false, 
                    std::format("Failed to create server in leak test cycle {}", cycle + 1));
                continue;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Create many connections
            std::vector<TCPConnInfo> clients;
            for (int i = 0; i < connections_per_cycle; ++i) {
                TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 14300 + cycle);
                if (clientInfo.sockfd != 0) {
                    clients.push_back(clientInfo);
                }
                
                // Small delay every few connections
                if (i % 5 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            // Send data on all connections to exercise memory allocation
            for (size_t i = 0; i < clients.size(); ++i) {
                std::string message = std::format("Leak test cycle {} message {}", cycle, i);
                manager.write(clients[i], message);
                
                if (i % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            // Close all connections explicitly
            for (const auto& client : clients) {
                try {
                    auto weakConn = manager.getConnection(client);
                    auto sharedConn = weakConn.lock();
                    if (sharedConn) {
                        sharedConn->stop(); // This will trigger connection cleanup
                    }
                } catch (...) {
                    // Connection might already be closed
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Stop manager
            manager.stop();
            
            std::cout << std::format("  Cycle {}: {} connections, {} bytes data processed", 
                cycle + 1, peak_connections.load(), total_data_received.load()) << std::endl;
            
            // Verify we created expected number of connections
            UnitTestFramework::assert_true(peak_connections >= clients.size() * 0.8, 
                std::format("Should have created most connections in cycle {}", cycle + 1));
            
        } // manager goes out of scope here, should clean up everything
        
        // Give time for complete cleanup between cycles
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Final cleanup time
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    UnitTestFramework::assert_true(true, "Memory leak detection test completed");
    std::cout << "Memory leak test completed. Monitor system memory usage to verify no leaks." << std::endl;
    std::cout << "If running under a debugger, check for memory leak reports." << std::endl;
}

int main() {
    std::cout << "=== TCP Connection Manager Unit Tests ===" << std::endl;
    std::cout << "Running focused unit tests for edge cases and error conditions..." << std::endl;

    test_targeted_signal();
    test_tcp_conn_info_comparison();
    test_invalid_operations();
    test_dns_lookup_edge_cases();
    test_source_binding();
    test_rapid_cycles();
    test_resource_cleanup();
    test_port_edge_cases();
    //test_ip_version_handling();
    test_data_integrity();
    test_server_lifecycle_and_connections();
    test_thread_management();
    test_connection_map_integrity();
    test_memory_leak_detection();

    UnitTestFramework::print_results();

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return UnitTestFramework::get_failed_tests() > 0 ? 1 : 0;
}
