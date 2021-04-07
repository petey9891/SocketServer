#pragma once

#include <rehoboam-server/common.h>
#include <rehoboam-server/tsqueue.h>

#include <thread>
#include <iostream>
#include <deque>

using asio::ip::tcp;

template<typename T>
class RehoboamServer {
protected:
    // Thread safe queue for incoming messages
    tsqueue<OwnedMessage<T> > qMessagesIn;

    // Container of active validated connections
    std::deque<std::shared_ptr<connection<T>>> deqConnections;    

    asio::io_context io_context;
    std::thread server_thread;
    std::thread request_thread;
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
            this->server_thread = std::thread([this]() { this->io_context.run(); });
        } catch (std::exception& e) {
            fprintf(stderr, "[SERVER] Exception %s\n", e.what());
            return false;
        }

        printf("[SERVER] Started\n");
        return true;
    };

    void Stop() {	
        this->io_context.stop();

        if (this->server_thread.joinable()) this->server_thread.join();
        if (this->request_thread.joinable()) this->request_thread.join();

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

                    // Create a new conneciton to handle the client
                    std::shared_ptr<connection<T>> conn = std::make_shared<connection<T>>(connection<T>::owner::server, this->io_context, std::move(socket), this->qMessagesIn);


                    if (this->OnClientConnect(conn)) {
                        printf("[SEVER] Connection approved\n");
                        
                        this->deqConnections.push_back(std::move(conn));
                                     
                        this->deqConnections.back()->ConnectToClient(this);
                    } else {
                        std::cout << "[SERVER] Connection denied from: " << socket.remote_endpoint() << "\n";
                    }
                }
                else {
                    printf("[SERVER] New Connection Error: %s\n", ec.message().c_str());
                }

                // Prime the asio context with more work - again simply wait for
                // another connection...
                this->WaitForConnection();
        });
    };

    void MessageClient(std::shared_ptr<connection<T>> client, const Message<T>& msg) {
        if (client && client->IsConnected()) {
            client->Send(msg);
        } else {
            this->OnClientDisconnect(client);

            client.reset();

            this->deqConnections.erase(std::remove(this->deqConnections.begin(), this->deqConnections.end(), client), this->deqConnections.end());
        }
    }


    void HandleRequests() {
        this->request_thread = std::thread([this]() { 
            while (true) {
                this->qMessagesIn.wait();
                while (!this->qMessagesIn.empty()) {
                    auto ownedMessage = this->qMessagesIn.pop_front();

                    this->OnMessageRecieved(ownedMessage.remote, ownedMessage.message);
                }
            }
        });
        
    }

protected:
    // Server class should override these functions
    virtual bool OnClientConnect(std::shared_ptr<connection<T> > client) {
        return false;
    }

    virtual void OnClientDisconnect(std::shared_ptr<connection<T> > client) {

    }

    virtual void OnMessageRecieved(std::shared_ptr<connection<T> > client, Message<T>& msg) {

    }

    virtual void OnClientValidated() {

    }
};