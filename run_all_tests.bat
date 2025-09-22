@echo off
echo ======================================================
echo TCP Connection Manager - Comprehensive Test Suite
echo ======================================================
echo.

REM Check if build directory exists
if not exist "build\Debug" (
    echo Error: Build directory not found. Please build the project first.
    echo Run: cmake --build build --config Debug
    pause
    exit /b 1
)

REM Check if test executables exist
if not exist "build\Debug\TCP_Tests.exe" (
    echo Error: TCP_Tests.exe not found. Please build the project first.
    pause
    exit /b 1
)

if not exist "build\Debug\TCP_Unit_Tests.exe" (
    echo Error: TCP_Unit_Tests.exe not found. Please build the project first.
    pause
    exit /b 1
)

if not exist "build\Debug\TCP_Performance_Tests.exe" (
    echo Error: TCP_Performance_Tests.exe not found. Please build the project first.
    pause
    exit /b 1
)

echo All test executables found. Starting test execution...
echo.

REM Run comprehensive integration tests
echo ======================================================
echo Running Comprehensive Integration Tests
echo ======================================================
echo.
build\Debug\TCP_Tests.exe
if %ERRORLEVEL% neq 0 (
    echo.
    echo WARNING: Some integration tests failed!
    echo.
) else (
    echo.
    echo All integration tests passed!
    echo.
)

echo.
echo Press any key to continue to unit tests...
pause >nul

REM Run focused unit tests
echo ======================================================
echo Running Focused Unit Tests
echo ======================================================
echo.
build\Debug\TCP_Unit_Tests.exe
if %ERRORLEVEL% neq 0 (
    echo.
    echo WARNING: Some unit tests failed!
    echo.
) else (
    echo.
    echo All unit tests passed!
    echo.
)

echo.
echo Press any key to continue to performance tests...
pause >nul

REM Run performance tests
echo ======================================================
echo Running Performance Tests
echo ======================================================
echo.
echo NOTE: Performance tests may take several minutes to complete.
echo These tests will stress-test the TCP connection manager.
echo.
build\Debug\TCP_Performance_Tests.exe

echo.
echo ======================================================
echo Test Suite Complete
echo ======================================================
echo.
echo All tests have been executed. Review the output above
echo for any failures or performance issues.
echo.
echo Test Summary:
echo - Integration Tests: Comprehensive functionality testing
echo - Unit Tests: Edge cases and error conditions
echo - Performance Tests: High-load and stress testing
echo.
pause

