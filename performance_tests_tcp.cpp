#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <future>
#include <numeric>
#include <algorithm>
#include <format>

#include "tcp_connection_manager.hpp"
#include "tcp_server.hpp"

// Performance testing framework
class PerformanceTest {
public:
    static void measure_time(const std::string& test_name, std::function<void()> test_func) {
        std::cout << "\n--- Performance Test: " << test_name << " ---" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        test_func();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Test completed in: " << duration.count() << " ms" << std::endl;
    }

    static void print_statistics(const std::string& name, const std::vector<double>& values) {
        if (values.empty()) return;
        
        auto minmax = std::minmax_element(values.begin(), values.end());
        double sum = std::accumulate(values.begin(), values.end(), 0.0);
        double mean = sum / values.size();
        
        std::vector<double> sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());
        double median = sorted_values[sorted_values.size() / 2];
        
        std::cout << std::format("{} - Count: {}, Min: {:.2f}, Max: {:.2f}, Mean: {:.2f}, Median: {:.2f}",
            name, values.size(), *minmax.first, *minmax.second, mean, median) << std::endl;
    }
};

// Test connection establishment performance
void test_connection_performance() {
    TCPConnectionManager manager;
    std::atomic<int> successful_connections{0};
    std::atomic<int> failed_connections{0};
    std::vector<double> connection_times;
    std::mutex times_mutex;
    
    manager.newConnection.connect([&](const TCPConnInfo& conn) {
        ++successful_connections;
    });
    
    // Create server
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 13000);
    if (serverInfo.sockfd == 0) {
        std::cerr << "Failed to create server for performance test" << std::endl;
        return;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const int num_connections = 1000;
    std::cout << std::format("Testing {} connection establishments...", num_connections) << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create connections in batches to avoid overwhelming the system
    const int batch_size = 50;
    for (int batch = 0; batch < num_connections / batch_size; ++batch) {
        std::vector<std::future<void>> futures;
        
        for (int i = 0; i < batch_size; ++i) {
            futures.push_back(std::async(std::launch::async, [&]() {
                auto conn_start = std::chrono::high_resolution_clock::now();
                
                TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 13000);
                
                auto conn_end = std::chrono::high_resolution_clock::now();
                auto conn_duration = std::chrono::duration_cast<std::chrono::microseconds>(conn_end - conn_start);
                
                if (clientInfo.sockfd != 0) {
                    std::lock_guard<std::mutex> lock(times_mutex);
                    connection_times.push_back(conn_duration.count() / 1000.0); // Convert to milliseconds
                } else {
                    ++failed_connections;
                }
            }));
        }
        
        // Wait for batch to complete
        for (auto& future : futures) {
            future.wait();
        }
        
        // Small delay between batches
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Let connections settle
    
    std::cout << std::format("Connection Performance Results:") << std::endl;
    std::cout << std::format("  Total time: {} ms", total_duration.count()) << std::endl;
    std::cout << std::format("  Successful connections: {}", successful_connections.load()) << std::endl;
    std::cout << std::format("  Failed connections: {}", failed_connections.load()) << std::endl;
    std::cout << std::format("  Success rate: {:.1f}%", 
        (double)successful_connections.load() / num_connections * 100.0) << std::endl;
    std::cout << std::format("  Connections per second: {:.1f}", 
        (double)successful_connections.load() / (total_duration.count() / 1000.0)) << std::endl;
    
    PerformanceTest::print_statistics("Connection Times (ms)", connection_times);
    
    manager.stop();
}

// Test data throughput performance
void test_data_throughput() {
    TCPConnectionManager manager;
    std::atomic<int> bytes_sent{0};
    std::atomic<int> bytes_received{0};
    std::atomic<int> messages_received{0};
    
    std::shared_ptr<TCPConnection> serverConn;
    
    manager.newConnection.connect([&](const TCPConnInfo& conn) {
        auto connPtr = manager.getConnection(conn).lock();
        if (connPtr) {
            serverConn = connPtr;
            connPtr->newDataArrived.connect([&](const std::vector<char>& data) {
                bytes_received += data.size();
                ++messages_received;
            });
        }
    });
    
    // Create server and client
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 13010);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 13010);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    if (clientInfo.sockfd == 0) {
        std::cerr << "Failed to establish client connection for throughput test" << std::endl;
        manager.stop();
        return;
    }
    
    // Prepare test data
    const int message_size = 1024; // 1KB messages
    const int num_messages = 1000;
    std::string test_message(message_size, 'A');
    
    std::cout << std::format("Testing data throughput: {} messages of {} bytes each...", 
        num_messages, message_size) << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Send messages
    for (int i = 0; i < num_messages; ++i) {
        bool sent = manager.write(clientInfo, test_message);
        if (sent) {
            bytes_sent += test_message.size();
        }
        
        // Small delay every 100 messages to prevent overwhelming
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    // Wait for all messages to be received
    auto timeout = std::chrono::seconds(10);
    auto wait_start = std::chrono::high_resolution_clock::now();
    
    while (messages_received < num_messages && 
           std::chrono::high_resolution_clock::now() - wait_start < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << std::format("Data Throughput Results:") << std::endl;
    std::cout << std::format("  Total time: {} ms", total_duration.count()) << std::endl;
    std::cout << std::format("  Bytes sent: {}", bytes_sent.load()) << std::endl;
    std::cout << std::format("  Bytes received: {}", bytes_received.load()) << std::endl;
    std::cout << std::format("  Messages received: {}/{}", messages_received.load(), num_messages) << std::endl;
    std::cout << std::format("  Data integrity: {:.1f}%", 
        (double)bytes_received.load() / bytes_sent.load() * 100.0) << std::endl;
    
    if (total_duration.count() > 0) {
        double throughput_mbps = (double)bytes_received.load() / (1024.0 * 1024.0) / (total_duration.count() / 1000.0);
        std::cout << std::format("  Throughput: {:.2f} MB/s", throughput_mbps) << std::endl;
        std::cout << std::format("  Messages per second: {:.1f}", 
            (double)messages_received.load() / (total_duration.count() / 1000.0)) << std::endl;
    }
    
    manager.stop();
}

// Test concurrent client performance
void test_concurrent_clients() {
    TCPConnectionManager manager;
    std::atomic<int> total_connections{0};
    std::atomic<int> total_messages_sent{0};
    std::atomic<int> total_messages_received{0};
    
    manager.newConnection.connect([&](const TCPConnInfo& conn) {
        ++total_connections;
        auto connPtr = manager.getConnection(conn).lock();
        if (connPtr) {
            connPtr->newDataArrived.connect([&](const std::vector<char>& data) {
                ++total_messages_received;
            });
        }
    });
    
    // Create server
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 13020);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const int num_clients = 100;
    const int messages_per_client = 10;
    
    std::cout << std::format("Testing {} concurrent clients, {} messages each...", 
        num_clients, messages_per_client) << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch concurrent clients
    std::vector<std::future<void>> client_futures;
    
    for (int client_id = 0; client_id < num_clients; ++client_id) {
        client_futures.push_back(std::async(std::launch::async, [&, client_id]() {
            // Connect client
            TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 13020);
            if (clientInfo.sockfd == 0) {
                return; // Failed to connect
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Let connection establish
            
            // Send messages
            for (int msg = 0; msg < messages_per_client; ++msg) {
                std::string message = std::format("Client_{:03d}_Message_{:03d}", client_id, msg);
                bool sent = manager.write(clientInfo, message);
                if (sent) {
                    ++total_messages_sent;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Keep connection alive
        }));
    }
    
    // Wait for all clients to complete
    for (auto& future : client_futures) {
        future.wait();
    }
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << std::format("Concurrent Clients Results:") << std::endl;
    std::cout << std::format("  Total time: {} ms", total_duration.count()) << std::endl;
    std::cout << std::format("  Successful connections: {}/{}", total_connections.load(), num_clients) << std::endl;
    std::cout << std::format("  Messages sent: {}", total_messages_sent.load()) << std::endl;
    std::cout << std::format("  Messages received: {}", total_messages_received.load()) << std::endl;
    std::cout << std::format("  Message delivery rate: {:.1f}%", 
        (double)total_messages_received.load() / total_messages_sent.load() * 100.0) << std::endl;
    
    if (total_duration.count() > 0) {
        std::cout << std::format("  Overall message rate: {:.1f} messages/second", 
            (double)total_messages_received.load() / (total_duration.count() / 1000.0)) << std::endl;
    }
    
    manager.stop();
}

// Test server broadcast performance
void test_broadcast_performance() {
    TCPConnectionManager manager;
    std::atomic<int> connected_clients{0};
    std::atomic<int> total_broadcasts_received{0};
    
    // Create server using TCPServer class
    TCPServer server(manager);
    server.start("127.0.0.1", 13030);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const int num_clients = 50;
    std::vector<TCPConnInfo> clients;
    
    std::cout << std::format("Setting up {} clients for broadcast test...", num_clients) << std::endl;
    
    // Connect clients and set up data reception
    for (int i = 0; i < num_clients; ++i) {
        TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 13030);
        if (clientInfo.sockfd != 0) {
            clients.push_back(clientInfo);
            ++connected_clients;
            
            auto connPtr = manager.getConnection(clientInfo).lock();
            if (connPtr) {
                connPtr->newDataArrived.connect([&](const std::vector<char>& data) {
                    ++total_broadcasts_received;
                });
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Let all connections stabilize
    
    const int num_broadcasts = 100;
    std::cout << std::format("Broadcasting {} messages to {} clients...", num_broadcasts, connected_clients.load()) << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Send broadcasts
    for (int i = 0; i < num_broadcasts; ++i) {
        std::string message = std::format("Broadcast_Message_{:04d}", i);
        server.broadcast(message);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Wait for all broadcasts to be received
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    int expected_total = num_broadcasts * connected_clients.load();
    
    std::cout << std::format("Broadcast Performance Results:") << std::endl;
    std::cout << std::format("  Total time: {} ms", total_duration.count()) << std::endl;
    std::cout << std::format("  Connected clients: {}", connected_clients.load()) << std::endl;
    std::cout << std::format("  Broadcasts sent: {}", num_broadcasts) << std::endl;
    std::cout << std::format("  Total messages received: {}/{}", total_broadcasts_received.load(), expected_total) << std::endl;
    std::cout << std::format("  Broadcast efficiency: {:.1f}%", 
        (double)total_broadcasts_received.load() / expected_total * 100.0) << std::endl;
    
    if (total_duration.count() > 0) {
        std::cout << std::format("  Broadcast rate: {:.1f} broadcasts/second", 
            (double)num_broadcasts / (total_duration.count() / 1000.0)) << std::endl;
        std::cout << std::format("  Message delivery rate: {:.1f} messages/second", 
            (double)total_broadcasts_received.load() / (total_duration.count() / 1000.0)) << std::endl;
    }
    
    manager.stop();
}

// Test memory usage under load
void test_memory_usage() {
    std::cout << "\n--- Memory Usage Test ---" << std::endl;
    std::cout << "Creating and destroying many connections to test memory management..." << std::endl;
    
    const int cycles = 10;
    const int connections_per_cycle = 100;
    
    for (int cycle = 0; cycle < cycles; ++cycle) {
        std::cout << std::format("Memory test cycle {}/{}", cycle + 1, cycles) << std::endl;
        
        TCPConnectionManager manager;
        std::atomic<int> connections_created{0};
        
        manager.newConnection.connect([&](const TCPConnInfo& conn) {
            ++connections_created;
        });
        
        // Create server
        TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 13040 + cycle);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Create many connections
        std::vector<TCPConnInfo> connections;
        for (int i = 0; i < connections_per_cycle; ++i) {
            TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 13040 + cycle);
            if (clientInfo.sockfd != 0) {
                connections.push_back(clientInfo);
            }
            
            if (i % 20 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Close all connections
        for (const auto& conn : connections) {
            manager.closeConn(conn);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        manager.stop();
        
        std::cout << std::format("  Created {} connections", connections.size()) << std::endl;
        
        // Small delay between cycles to allow cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Memory usage test completed. Check for memory leaks manually." << std::endl;
}

// Test latency under different loads
void test_latency_under_load() {
    TCPConnectionManager manager;
    std::vector<double> latencies;
    std::mutex latencies_mutex;
    
    manager.newConnection.connect([&](const TCPConnInfo& conn) {
        auto connPtr = manager.getConnection(conn).lock();
        if (connPtr) {
            connPtr->newDataArrived.connect([&](const std::vector<char>& data) {
                std::string received(data.begin(), data.end());
                
                // Parse timestamp from message
                if (received.find("PING_") == 0) {
                    try {
                        size_t pos = received.find("_TIME_");
                        if (pos != std::string::npos) {
                            std::string timestamp_str = received.substr(pos + 6);
                            long long sent_time = std::stoll(timestamp_str);
                            
                            auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                            
                            double latency = (now - sent_time) / 1000.0; // Convert to milliseconds
                            
                            std::lock_guard<std::mutex> lock(latencies_mutex);
                            latencies.push_back(latency);
                        }
                    } catch (...) {
                        // Ignore parsing errors
                    }
                }
            });
        }
    });
    
    // Create server and client
    TCPConnInfo serverInfo = manager.openListenSocket("127.0.0.1", 13050);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TCPConnInfo clientInfo = manager.openConnection("127.0.0.1", 13050);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    const int num_pings = 1000;
    std::cout << std::format("Testing latency with {} ping messages...", num_pings) << std::endl;
    
    // Send ping messages with timestamps
    for (int i = 0; i < num_pings; ++i) {
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        std::string message = std::format("PING_{:04d}_TIME_{}", i, timestamp);
        manager.write(clientInfo, message);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // Wait for responses
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    std::cout << std::format("Latency Test Results:") << std::endl;
    std::cout << std::format("  Ping messages sent: {}", num_pings) << std::endl;
    std::cout << std::format("  Responses received: {}", latencies.size()) << std::endl;
    
    if (!latencies.empty()) {
        PerformanceTest::print_statistics("Latency (ms)", latencies);
    }
    
    manager.stop();
}

int main() {
    std::cout << "=== TCP Connection Manager Performance Tests ===" << std::endl;
    std::cout << "Running performance tests for TCP connection manager..." << std::endl;
    std::cout << "Note: These tests may take several minutes to complete." << std::endl;

    PerformanceTest::measure_time("Connection Performance", test_connection_performance);
    PerformanceTest::measure_time("Data Throughput", test_data_throughput);
    PerformanceTest::measure_time("Concurrent Clients", test_concurrent_clients);
    PerformanceTest::measure_time("Broadcast Performance", test_broadcast_performance);
    PerformanceTest::measure_time("Memory Usage", test_memory_usage);
    PerformanceTest::measure_time("Latency Under Load", test_latency_under_load);

    std::cout << "\n=== Performance Testing Complete ===" << std::endl;
    std::cout << "All performance tests have been executed." << std::endl;
    std::cout << "Review the results above for performance characteristics." << std::endl;

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}

