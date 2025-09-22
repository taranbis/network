# TCP Connection Manager - Comprehensive Test Suite

## Quick Start

### 1. Build and Run All Tests (Recommended)
```cmd
build_and_test.bat
```
This script will:
- Configure the project with CMake
- Build all executables including tests
- Optionally run the complete test suite

### 2. Run Tests Only (if already built)
```cmd
run_all_tests.bat
```

### 3. Run Individual Test Suites
```cmd
# Integration tests (comprehensive functionality)
build\Debug\TCP_Tests.exe

# Unit tests (edge cases and error conditions)
build\Debug\TCP_Unit_Tests.exe

# Performance tests (high-load scenarios)
build\Debug\TCP_Performance_Tests.exe
```

## What This Test Suite Covers

### ✅ Complete Functionality Testing
- **Basic Operations**: Connection creation, data transfer, cleanup
- **Multiple Servers**: Different ports, concurrent servers
- **Multiple Clients**: Many clients to single server
- **Data Integrity**: Bidirectional communication verification
- **Connection Lifecycle**: Connect/disconnect cycles
- **Error Handling**: Invalid inputs, network failures
- **Thread Safety**: Concurrent operations
- **Signal System**: Event notifications
- **DNS Resolution**: Hostname lookup functionality

### ✅ Edge Cases and Error Conditions
- Invalid IP addresses and ports
- DNS lookup failures
- Resource exhaustion scenarios
- Rapid connection cycles
- Memory management edge cases
- IPv4/IPv6 compatibility
- Port binding edge cases
- Server lifecycle management (start/stop cycles)
- Thread management and cleanup verification
- Connection map (m_connections) integrity
- Memory leak detection across multiple cycles

### ✅ Performance and Load Testing
- **High Volume**: 1000+ connections
- **Throughput**: MB/s data transfer rates
- **Concurrency**: 100+ simultaneous clients
- **Latency**: Response time measurements
- **Memory Usage**: Resource consumption patterns
- **Broadcast Efficiency**: Server-to-multiple-clients performance

## Test Results

The test suite provides detailed output including:
- ✅ Pass/Fail status for each test
- 📊 Performance metrics (connections/sec, MB/s, latency)
- 📈 Statistical analysis (min, max, mean, median)
- 🔍 Detailed failure messages with context
- 📋 Summary reports

### Sample Output
```
--- Running test: Multiple Clients to Server ---
Testing multiple clients connecting to single server...
Creating server on port 12370...
Connecting 5 clients...
PASS: Client 1 should connect
PASS: Client 2 should connect
...
PASS: Should have 5 new connections
Test completed: Multiple Clients to Server

=== TEST RESULTS ===
Passed: 127
Failed: 0
Total: 127
ALL TESTS PASSED!
```

## Test Architecture

### 3-Tier Testing Approach
1. **Integration Tests**: End-to-end functionality testing
2. **Unit Tests**: Component-specific and edge case testing  
3. **Performance Tests**: Load and stress testing

### Custom Test Framework
- Assertion-based testing with detailed messages
- Automatic result tracking and reporting
- Exception handling and error recovery
- Performance measurement capabilities

### Thread-Safe Design
- Atomic counters for concurrent operations
- Mutex-protected shared data structures
- Event-driven connection tracking
- Proper resource cleanup

## System Requirements

- **OS**: Windows (WinSock required)
- **Compiler**: MSVC with C++20 support
- **Dependencies**: Boost, CMake
- **Network**: Working TCP/IP stack
- **Ports**: Access to ports 12300-13100 (test range)

## Troubleshooting

### Common Issues

**Build Failures**
- Ensure all dependencies are installed (Boost, CMake)
- Check that MSVC compiler supports C++20
- Verify CMake can find all required packages

**Test Failures**
- Check Windows Firewall settings (allow test executables)
- Ensure test ports are not in use by other applications
- Some IPv6 tests may fail if IPv6 is not configured

**Performance Variations**
- Results depend on system capabilities
- Network configuration affects performance
- System load impacts timing-sensitive tests

### Debug Steps
1. Run individual test suites to isolate issues
2. Check build output for compilation errors
3. Review test output for specific failure messages
4. Verify network connectivity and firewall settings

## Test Coverage Summary

| Category | Tests | Coverage |
|----------|--------|----------|
| **Basic Operations** | 15+ | Connection, data transfer, cleanup |
| **Multi-Server** | 10+ | Multiple servers, port management |
| **Multi-Client** | 12+ | Concurrent clients, load handling |
| **Error Handling** | 20+ | Invalid inputs, failure scenarios |
| **Performance** | 8+ | Throughput, latency, concurrency |
| **Edge Cases** | 25+ | Resource limits, rapid cycles |
| **Thread Safety** | 10+ | Concurrent operations, race conditions |
| **Server Lifecycle** | 5+ | Start/stop cycles, connection tracking |
| **Memory Management** | 8+ | Leak detection, resource cleanup |

## Contributing

To add new tests:
1. Choose appropriate test file based on test type
2. Follow existing test function patterns
3. Use unique port numbers (increment last used port)
4. Add proper cleanup and error handling
5. Update documentation

### Test Function Template
```cpp
void test_new_feature() {
    std::cout << "Testing new feature..." << std::endl;
    
    TCPConnectionManager manager;
    // ... test implementation ...
    
    TestFramework::assertTrue(condition, "Description");
    manager.stop();
}
```

## License

Same as main project license.

---

**Need Help?** 
- Check `TEST_DOCUMENTATION.md` for detailed information
- Review test output for specific error messages  
- Ensure all prerequisites are met before running tests
