#pragma once

// Include required headers from the standard library and ASIO for logging functionality.
#include <iostream>
#include <ctime>
#include <asio.hpp>

namespace Rehoboam {
    namespace SocketLibrary {

        // Define the current log level for the application.
        #define CURRENT_LOG_LEVEL INFO

        /**
         * @enum ProgramType
         * @brief Enum to represent the type of the running program (server or client).
         */
        enum ProgramType {
            server,
            client
        };

        /**
         * @brief Converts a ProgramType enum to its string representation.
         * @param type The ProgramType enum to convert.
         * @return A string representation of the ProgramType.
         */
        std::string inline ProgramTypeToString(ProgramType type) {
            switch (type) {
                case server: return "SERVER";
                case client: return "CLIENT";
                default: return "UNKNOWN";
            }
        }

        /**
         * @brief Generates a timestamp string in the format YYYY-MM-DD HH:MM:SS.
         * @return A timestamp string.
         */
        std::string inline timestamp() {
            std::time_t now = std::time(0);
            std::tm* now_tm = std::gmtime(&now);
            char buffer[42];
            std::strftime(buffer, 42, "%Y-%m-%d %X", now_tm);
            return buffer;
        }

        /**
         * @enum LogLevel
         * @brief Enum to represent different levels of log messages.
         */
        enum LogLevel {
            INFO,
            DEBUG,
            ERROR
        };

        /**
         * @brief Validates if a log message of a given level should be displayed.
         * @param level The LogLevel of the message.
         * @return True if the message should be logged, false otherwise.
         */
        bool inline validateLog(LogLevel level) {
            return (CURRENT_LOG_LEVEL == level) || (level == INFO) || (level == ERROR);
        }

        /**
         * @brief Converts a LogLevel enum to its string representation.
         * @param level The LogLevel enum to convert.
         * @return A string representation of the LogLevel.
         */
        std::string inline levelToString(LogLevel level) {
            switch (level) {
                case INFO: return "INFO";
                case DEBUG: return "DEBUG";
                case ERROR: return "ERROR";
                default: return "UNKNOWN";
            }
        }

        /**
         * @brief Converts a boolean value to its string representation.
         * @param value The boolean value to convert.
         * @return "true" if value is true, "false" otherwise.
         */
        std::string inline boolToString(bool value) {
            return value ? "true" : "false";
        }

        /**
         * @brief Logs a message with a given LogLevel.
         * @param level The LogLevel of the message.
         * @param message The message to log.
         */
        void inline LOG(LogLevel level, std::string message) {
            if (validateLog(level)) {
                std::cout << "[" << levelToString(level) << "] " << "[" << timestamp() << "]" <<  " [SELF] " << message << std::endl;
            }
        }

        /**
         * @brief Logs a message with a given LogLevel and a condition.
         * @param level The LogLevel of the message.
         * @param message The message to log.
         * @param condition A boolean condition to be logged alongside the message.
         */
        void inline LOG(LogLevel level, std::string message, bool condition) {
            if (validateLog(level)) {
                std::cout << "[" << levelToString(level) << "] " << "[" << timestamp() << "]" <<  " [SELF] " << message << ": " << boolToString(condition) << std::endl;
            }
        }

        /**
         * @brief Logs a message with a given LogLevel and additional data.
         * @param level The LogLevel of the message.
         * @param message The message to log.
         * @param data Additional data to be logged alongside the message.
         */
        void inline LOG(LogLevel level, std::string message, std::string data) {
            if (validateLog(level)) {
                std::cout << "[" << levelToString(level) << "] " << "[" << timestamp() << "]" << " [SELF] " << message << ": " << data << std::endl;
            }
        }

        /**
         * @brief Logs a message with a given LogLevel, associating it with a network endpoint.
         * @param level The LogLevel of the message.
         * @param message The message to log.
         * @param endpoint The network endpoint associated with the message.
         */
        void inline LOG(LogLevel level, std::string message, asio::ip::tcp::endpoint endpoint) {
            if (validateLog(level)) {
                std::cout << "[" << levelToString(level) << "] " << "[" << timestamp() << "]" << " [" << endpoint << "] " << message << std::endl;
            }
        }

        /**
         * @brief Logs a message with a given LogLevel, associating it with a network endpoint and additional data.
         * @param level The LogLevel of the message.
         * @param message The message to log.
         * @param endpoint The network endpoint associated with the message.
         * @param data Additional data to be logged alongside the message.
         */
        void inline LOG(LogLevel level, std::string message, asio::ip::tcp::endpoint endpoint, std::string data) {
            if (validateLog(level)) {
                std::cout << "[" << levelToString(level) << "] " << "[" << timestamp() << "]" << " [" << endpoint << "] " << message << ": " << data << std::endl;
            }
        }
    }  // namespace SocketLibrary
}  // namespace Rehoboam