#pragma once

#include <SocketServer/common.h>
#include <SocketServer/tsqueue.h>

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
    std::deque<std::shared_ptr<SocketConnection<T>>> deqConnections;    

    asio::io_context io_context;
    asio::ip::tcp::acceptor acceptor;
    asio::ssl::context ssl_context;

    std::thread server_thread;
    std::thread request_thread;

private:
    std::string certPath;
    std::string keyPath;
    std::string caPath;

public:
    // Create the server and listen to the desired port
    SocketServer(uint16_t port, std::string certPath, std::string keyPath, std::string caPath)
        : acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), ssl_context(asio::ssl::context::sslv23), certPath(certPath), keyPath(keyPath), caPath(caPath)
    {
        this->ssl_context.set_options(
            asio::ssl::context::default_workarounds 
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::single_dh_use);

        /**
        *   verify client auth
        */
        this->ssl_context.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);
        this->ssl_context.load_verify_file(this->caPath);

        this->ssl_context.use_certificate_file(this->certPath, asio::ssl::context::pem);
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

        std::cout << "[SERVER] Started" << std::endl;
        return true;
    };

    void Stop() {	
        this->io_context.stop();

        if (this->server_thread.joinable()) this->server_thread.join();
        if (this->request_thread.joinable()) this->request_thread.join();

        std::cout << "[SERVER] Stopped" << std::endl;
    }

    // ASYNC    
    void WaitForConnection() {
        std::shared_ptr<SocketConnection<T>> conn = std::make_shared<SocketConnection<T>>(SocketConnection<T>::owner::server, this->io_context, this->ssl_context, this->qMessagesIn);
        this->acceptor.async_accept(conn->socket(),
            [this, conn](std::error_code err) {
                // Triggered by incoming SocketConnection request
                if (!err) {
                    // Display some useful(?) information
                    std::cout << "[SERVER] New Connection: " << conn->socket().remote_endpoint() << "\n";

                    if (this->OnClientConnect(conn)) {
                        std::cout << "[SERVER] Connection approved" << std::endl;
                        
                        this->deqConnections.push_back(std::move(conn));
                                     
                        this->deqConnections.back()->ConnectToClient(this);
                    } else {
                        std::cout << "[SERVER] Connection denied from: " << conn->socket().remote_endpoint() << "\n";
                    }
                }
                else {
                    std::cout << "[SERVER] New Connection Error: " << err.message() << std::endl;
                }

                // Prime the asio context with more work - again simply wait for
                // another SocketConnection...
                this->WaitForConnection();
        });
    };

    void MessageClient(std::shared_ptr<SocketConnection<T>> client, const Message<T>& msg) {
        if (client && client->IsConnected()) {
            client->Send(msg);
        } else {
            this->OnClientDisconnect(client);

            client.reset();

            this->deqConnections.erase(std::remove(this->deqConnections.begin(), this->deqConnections.end(), client), this->deqConnections.end());
        }
    }

    void MessageAllClients(const Message<T>& msg, std::shared_ptr<SocketConnection<T>> pIgnoreClient = nullptr) {
        bool invalidClientsExist = false;

        for (auto& client : this->deqConnections) {
            // Make sure the client is connected
            if (client && client->IsConnected()) {
                if (client != pIgnoreClient) {
                    client->Send(msg);
                }
            } else {
                std::cout << "[SERVER] Hmmm this client seems to have disconnected" << std::endl;
                // This client shouldn't be contacted, so assume it has been disconnected
                OnClientDisconnect(client);
                client.reset();

                invalidClientsExist = true;
            }
        }

        // Remove dead clients all in one go
        if (invalidClientsExist) {
            this->deqConnections.erase(std::remove(this->deqConnections.begin(), this->deqConnections.end(), nullptr), this->deqConnections.end());
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
    virtual bool OnClientConnect(std::shared_ptr<SocketConnection<T> > client) {
        return false;
    }

    virtual void OnClientDisconnect(std::shared_ptr<SocketConnection<T> > client) {

    }

    virtual void OnMessageRecieved(std::shared_ptr<SocketConnection<T> > client, Message<T>& msg) {

    }

    virtual void OnClientValidated() {

    }
};