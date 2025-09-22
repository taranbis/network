#include <iostream>
#include <format>
#include <thread>

#include "tcp_server.hpp"

void printingFunction(std::vector<char> buffer) {
    std::cout << "[main] ----- Number of bytes read: " << buffer.size() << std::endl;
    std::cout << "[main] ----- Message: ";

    for (const auto& b : buffer) { std::cout << b; }
    std::cout << std::endl;
}

int main()
{
    
}