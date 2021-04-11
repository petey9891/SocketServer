#pragma once

#include <rehoboam-server/common.h>
#include <rehoboam-server/tsqueue.h>

#include <thread>
#include <iostream>
#include <deque>

using asio::ip::tcp;

template<typename T>
class SocketServer {
protected:
    // Thread safe queue for incoming messages
    tsqueue<OwnedMessage<T> > qMessagesIn;

    // Container of active validated connections
    std::deque<std::shared_ptr<connection<T>>> deqConnections;    

    asio::io_context io_context;
    asio::ip::tcp::acceptor acceptor;
    asio::ssl::context ssl_context;

    std::thread server_thread;
    std::thread request_thread;

private:
    std::string caPath;
    std::string keyPath;

public:
    // Create the server and listen to the desired port
    SocketServer(uint16_t port, std::string caPath, std::string keyPath)
        : acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), ssl_context(asio::ssl::context::sslv23), caPath(caPath), keyPath(keyPath) 
    {
        this->ssl_context.set_options(
            asio::ssl::context::default_workarounds 
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::single_dh_use);
        this->ssl_context.use_certificate_file(this->caPath, asio::ssl::context::pem);
        this->ssl_context.use_private_key_file(this->keyPath, asio::ssl::context::pem);
    }
    
    virtual ~SocketServer() {
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
        std::shared_ptr<connection<T>> conn = std::make_shared<connection<T>>(connection<T>::owner::server, this->io_context, this->ssl_context, this->qMessagesIn);
        this->acceptor.async_accept(conn->socket(),
            [this, conn](std::error_code err) {
                // Triggered by incoming connection request
                if (!err) {
                    // Display some useful(?) information
                    std::cout << "[SERVER] New Connection: " << conn->socket().remote_endpoint() << "\n";

                    if (this->OnClientConnect(conn)) {
                        printf("[SERVER] Connection approved\n");
                        
                        this->deqConnections.push_back(std::move(conn));
                                     
                        this->deqConnections.back()->ConnectToClient(this);
                    } else {
                        std::cout << "[SERVER] Connection denied from: " << conn->socket().remote_endpoint() << "\n";
                    }
                }
                else {
                    printf("[SERVER] New Connection Error: %s\n", err.message().c_str());
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

    void HandleRequestsNoThread() {
        this->qMessagesIn.wait();
        while (!this->qMessagesIn.empty()) {
            auto ownedMessage = this->qMessagesIn.pop_front();

            this->OnMessageRecieved(ownedMessage.remote, ownedMessage.message);
        }
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