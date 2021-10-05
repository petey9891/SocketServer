#pragma once

#include <iostream>
#include <ctime>
#include <asio.hpp>

enum ProgramType {
    server,
    client
};

std::string inline ProgramTypeToString(ProgramType type) {
    switch (type) {
        case server: return "SERVER";
        case client: return "CLIENT";
    }
}

std::string inline timestamp() {
    std::time_t now= std::time(0);
    std::tm* now_tm= std::gmtime(&now);
    char buffer[42];
    std::strftime(buffer, 42, "%Y-%m-%d %X", now_tm);
    return buffer;
}



enum LogLevel {
    INFO,
    DEBUG,
    ERROR
};

#define CURRENT_LOG_LEVEL DEBUG

bool inline validateLog(LogLevel level) {
    // adding this logic to lock out logging when level is error
    // if (CURRENT_LOG_LEVEL == ERROR && level == ERROR) {
    //     return true;
    // } else if (CURRENT_LOG_LEVEL == DEBUG && level == DEBUG) {
    //     return true;
    // } else if (CURRENT_LOG_LEVEL == INFO)

    if (CURRENT_LOG_LEVEL == DEBUG && level == DEBUG) {
        return true;
    }
    else if (CURRENT_LOG_LEVEL == ERROR && level == ERROR) {
        return true;
    } else {
        if (level == INFO || level == ERROR) {
            return true;
        }
    }
    // if (level == INFO || level == ERROR || (CURRENT_LOG_LEVEL == DEBUG && level == DEBUG)) {
    //     return true;
    // }
    return false;
}

std::string inline levelToSring(LogLevel level) {
    switch (level) {
        case INFO: 
            return "INFO";
        case DEBUG:
            return "DEBUG";
        case ERROR:
            return "ERROR";
        default:
            return "INFO"; 
    }
}

void inline LOG(LogLevel level, std::string message) {
    if (validateLog(level)) {
        std::cout << "[" << levelToSring(level) << "] " << "[" << timestamp() << "]" <<  " [SELF] " << message << std::endl;
    }
}

void inline LOG(LogLevel level, std::string message, std::string data) {
    if (validateLog(level)) {
        std::cout << "[" << levelToSring(level) << "] " << "[" << timestamp() << "]" << " [SELF] " << message << ": " << data << std::endl;
    }
}

void inline LOG(LogLevel level, std::string message, asio::ip::tcp::endpoint endpoint) {
    if (validateLog(level)) {
        std::cout << "[" << levelToSring(level) << "] " << "[" << timestamp() << "]" << " [" << endpoint << "] " << message << std::endl;
    }
}

void inline LOG(LogLevel level, std::string message, asio::ip::tcp::endpoint endpoint, std::string data) {
    if (validateLog(level)) {
        std::cout << "[" << levelToSring(level) << "] " << "[" << timestamp() << "]" << " [" << endpoint << "] " << message << ": " << data << std::endl;
    }
}