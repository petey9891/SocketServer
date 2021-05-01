#pragma once

#include <iostream>
#include <ctime>
#include <asio.hpp>

enum ClientType {
    server,
    client
};

std::string inline ClientTypeToString(ClientType type) {
    switch (type) {
        case server: return "SERVER";
        case client: return "CLIENT";
    }
}

std::string timestamp() {
    std::time_t now= std::time(0);
    std::tm* now_tm= std::gmtime(&now);
    char buffer[42];
    std::strftime(buffer, 42, "%Y-%m-%d %X", now_tm);
    return buffer;
}

void inline LOG(std::string message) {

    std::cout << "[" << timestamp() << "]" << " [SELF] " << message << std::endl;
}

void inline LOG(std::string message, std::string data) {
    std::cout << "[" << timestamp() << "]" << " [SELF] " << message << ": " << data << std::endl;
}

void inline LOG(std::string message, asio::ip::tcp::endpoint endpoint) {
    std::cout << "[" << timestamp() << "]" << " [" << endpoint << "] " << message << std::endl;
}

void inline LOG(std::string message, asio::ip::tcp::endpoint endpoint, std::string data) {
    std::cout << "[" << timestamp() << "]" << " [" << endpoint << "] " << message << ": " << data << std::endl;
}
