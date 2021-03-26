#pragma once

#include <rehoboam-server/common.h>
#include <thread>
#include <iostream>

using asio::ip::tcp;

template<typename T>
class RehoboamServer {
protected:
    asio::io_context io_context;
    std::thread thread_context;
    asio::ip::tcp::acceptor acceptor;

public:
    // Create the server and listen to the desired port
    RehoboamServer(uint16_t port): acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}
    virtual ~RehoboamServer() {
        this->Stop();
    }

    // Start the server
    bool Start() {
        try {   
            this->WaitForConnection();

            // Launch the asio context in its own thread
            this->thread_context = std::thread([this]() { this->io_context.run(); });
        } catch (std::exception& e) {
            fprintf(stderr, "[SERVER] Exception %s\n", e.what());
            return false;
        }

        printf("[SERVER] Started\n");
        return true;
    };

    void Stop() {	
        this->io_context.stop();

        if (this->thread_context.joinable()) this->thread_context.join();

        printf("[SERVER] Stopped\n");
    }

    // ASYNC    
    void WaitForConnection() {
        this->acceptor.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket socket) {
                // Triggered by incoming connection request
                if (!ec) {
                    // Display some useful(?) information
                    std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";
                }
                else {
                    printf("[SERVER] New Connection Error: %s\n", ec.message().c_str());
                }

                // Prime the asio context with more work - again simply wait for
                // another connection...
                this->WaitForConnection();
        });
    };

    void MessageClient();
    void HandleRequest();

protected:
    // Server class should override these functions
    virtual bool OnClientConnect() {
        return false;
    }

    virtual void OnClientDisconnect() {

    }

    virtual void OnMessageRecieved() {

    }

    virtual void OnClientValidated() {

    }
};