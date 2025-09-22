# TCP Connection Manager Test Suite

## Overview

This comprehensive test suite provides thorough testing for the TCP Connection Manager functionality, covering everything from basic operations to high-load scenarios and edge cases.

## Test Structure

The test suite is organized into three main categories:

### 1. Integration Tests (`test_tcp_connection_manager.cpp`)
**Executable**: `TCP_Tests.exe`

Comprehensive functionality testing that covers the complete workflow of the TCP connection manager:

- **Basic Connection Manager**: Creation and destruction
- **DNS Lookup**: Hostname resolution functionality  
- **Listen Socket Creation**: Server socket setup
- **Client Connection**: Basic client-server connections
- **Multiple Servers**: Multiple servers on different ports
- **Multiple Clients to Server**: Multiple clients connecting to single server
- **Data Transfer**: Bidirectional data communication
- **Connection Lifecycle**: Connect/disconnect cycles and cleanup
- **Error Handling**: Invalid operations and error conditions
- **Signal/Slot Mechanism**: Event notification system
- **Concurrent Operations**: Thread safety and parallel operations
- **TCPServer Integration**: High-level server class functionality
- **Stress Test**: High-volume connection testing

### 2. Unit Tests (`unit_tests_tcp.cpp`)
**Executable**: `TCP_Unit_Tests.exe`

Focused testing for edge cases, error conditions, and specific components:

- **TargetedSignal Class**: Socket-specific signal routing
- **TCPConnInfo Comparison**: Connection info operators and container usage
- **Invalid Operations**: Error handling for invalid inputs
- **DNS Lookup Edge Cases**: Empty hostnames, IPv6, invalid addresses
- **Source Binding**: Client connections with specific source addresses
- **Rapid Cycles**: Fast connection/disconnection sequences
- **Resource Cleanup**: Memory management and resource deallocation
- **Port Edge Cases**: Port 0, high ports, privileged ports
- **IP Version Handling**: IPv4 vs IPv6 address processing
- **Data Integrity**: Message correctness under load
- **Server Lifecycle**: Multiple start/stop cycles with connection verification
- **Thread Management**: Thread creation, cleanup, and proper resource management
- **Connection Map Integrity**: Verification of m_connections map consistency
- **Memory Leak Detection**: Multi-cycle testing for resource leaks

### 3. Performance Tests (`performance_tests_tcp.cpp`)
**Executable**: `TCP_Performance_Tests.exe`

High-load and performance testing:

- **Connection Performance**: Connection establishment speed and success rates
- **Data Throughput**: Bandwidth and message processing rates
- **Concurrent Clients**: Multiple simultaneous client performance
- **Broadcast Performance**: Server broadcast efficiency
- **Memory Usage**: Memory management under load
- **Latency Under Load**: Response times with various loads

## Test Coverage

### Functionality Coverage
- ✅ Connection establishment (client and server)
- ✅ Data transmission (bidirectional)
- ✅ Connection cleanup and resource management
- ✅ Error handling and edge cases
- ✅ Multi-threading and concurrency
- ✅ Signal/slot event system
- ✅ DNS resolution
- ✅ IPv4 and IPv6 support
- ✅ High-level server abstractions

### Error Scenarios Tested
- ✅ Invalid IP addresses
- ✅ Invalid ports
- ✅ Connection failures
- ✅ DNS resolution failures
- ✅ Resource exhaustion
- ✅ Rapid connect/disconnect cycles
- ✅ Invalid socket operations
- ✅ Memory management errors

### Performance Scenarios
- ✅ High connection volume (1000+ connections)
- ✅ High data throughput (MB/s measurements)
- ✅ Concurrent client operations (100+ clients)
- ✅ Broadcast efficiency
- ✅ Latency measurements
- ✅ Memory usage patterns

## Running the Tests

### Prerequisites
1. Build the project using CMake:
   ```cmd
   cmake --build build --config Debug
   ```

2. Ensure all test executables are built:
   - `build\Debug\TCP_Tests.exe`
   - `build\Debug\TCP_Unit_Tests.exe`
   - `build\Debug\TCP_Performance_Tests.exe`

### Running All Tests
Execute the test runner script:
```cmd
run_all_tests.bat
```

### Running Individual Test Suites

**Integration Tests:**
```cmd
build\Debug\TCP_Tests.exe
```

**Unit Tests:**
```cmd
build\Debug\TCP_Unit_Tests.exe
```

**Performance Tests:**
```cmd
build\Debug\TCP_Performance_Tests.exe
```

## Test Results Interpretation

### Success Criteria
- **Integration Tests**: All functional tests should pass
- **Unit Tests**: All edge case tests should pass
- **Performance Tests**: Performance metrics within acceptable ranges

### Common Issues and Solutions

1. **Port Already in Use**
   - Tests use different port ranges to avoid conflicts
   - If tests fail due to port conflicts, ensure no other applications are using the test ports (12300-13100)

2. **Windows Firewall**
   - Windows may prompt for firewall permissions
   - Allow the test executables through the firewall for proper testing

3. **Resource Limits**
   - Performance tests may hit system limits on connections
   - Adjust test parameters if needed for your system

4. **IPv6 Support**
   - Some IPv6 tests may fail if IPv6 is not properly configured
   - This is expected on systems without IPv6 support

## Test Framework Features

### Custom Test Framework
- Assertion-based testing with detailed failure messages
- Automatic test counting and result reporting
- Exception handling and error reporting
- Performance measurement and statistics

### Connection Tracking
- Event-based connection monitoring
- Data reception verification
- Connection lifecycle tracking
- Thread-safe counters and collections

### Performance Measurement
- High-resolution timing
- Statistical analysis (min, max, mean, median)
- Throughput calculations
- Latency measurements

## Adding New Tests

To add new tests to the suite:

1. **For Integration Tests**: Add new test functions to `test_tcp_connection_manager.cpp`
2. **For Unit Tests**: Add new test functions to `unit_tests_tcp.cpp`
3. **For Performance Tests**: Add new test functions to `performance_tests_tcp.cpp`

### Test Function Template
```cpp
void test_new_functionality() {
    std::cout << "Testing new functionality..." << std::endl;
    
    TCPConnectionManager manager;
    ConnectionTracker tracker;
    
    // Setup test scenario
    // ...
    
    // Execute test
    // ...
    
    // Verify results
    TestFramework::assertTrue(condition, "Description of what should be true");
    
    manager.stop();
}
```

Remember to:
- Use unique port numbers to avoid conflicts
- Clean up resources properly
- Add appropriate delays for async operations
- Handle expected exceptions
- Provide descriptive assertion messages

## Continuous Integration

These tests are designed to be run in CI/CD environments:
- No interactive prompts (except for manual runs)
- Clear exit codes (0 = success, non-zero = failure)
- Detailed logging for debugging failures
- Configurable test parameters

## Test Maintenance

Regular maintenance tasks:
1. Update port ranges if conflicts occur
2. Adjust timeouts based on system performance
3. Add tests for new functionality
4. Review and update performance benchmarks
5. Ensure compatibility with new Windows/compiler versions

## Known Limitations

1. **Platform Specific**: Tests are designed for Windows with WinSock
2. **Resource Dependent**: Performance results vary by system capabilities
3. **Network Dependent**: Some tests require working network stack
4. **Timing Sensitive**: Some tests may be sensitive to system load

## Support

For issues with the test suite:
1. Check that all prerequisites are met
2. Verify the build completed successfully
3. Run tests individually to isolate issues
4. Check Windows Event Viewer for system-level errors
5. Review test output for specific failure messages
