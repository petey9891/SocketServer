#pragma once

#include <SocketServer/common.h>
#include <SocketServer/tsqueue.h>

#include <thread>
#include <deque>

using asio::ip::tcp;

template<typename T>
class SocketServer {
public:
    // Create the server and listen to the desired port
    SocketServer(uint16_t port, std::string certPath, std::string keyPath, std::string caPath)
        : acceptor(io_context, tcp::endpoint(tcp::v4(), port)), ssl_context(asio::ssl::context::sslv23), 
          certPath(certPath), keyPath(keyPath), caPath(caPath)
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
            LOG(ERROR, "Exception", e.what());
            return false;
        }

        LOG(INFO, "Started");
        return true;
    };

    void Stop() {	
        this->io_context.stop();

        if (this->server_thread.joinable()) this->server_thread.join();
        if (this->request_thread.joinable()) this->request_thread.join();

        LOG(INFO, "Stopped");
    }

    // ASYNC    
    void WaitForConnection() {
        std::shared_ptr<SocketConnection<T>> conn = std::make_shared<SocketConnection<T>>(SocketConnection<T>::owner::server, this->io_context, this->ssl_context, this->qMessagesIn);
        this->acceptor.async_accept(conn->socket(),
            [this, conn](std::error_code err) {
                // Triggered by incoming SocketConnection request
                LOG(DEBUG, "Recieved new connection");
                if (!err) {
                    // Display some useful(?) information
                    LOG(INFO, "New Connection", conn->socket().remote_endpoint());

                    if (this->OnClientConnect(conn)) {                
                        this->deqConnections.push_back(std::move(conn));
                        this->ConnectToClient(conn);
                    } else {
                        LOG(INFO, "Connection denied", conn->socket().remote_endpoint());
                        conn->Disconnect();
                        LOG(DEBUG, "The new connection has been disconnected");
                    }
                }
                else {
                    LOG(ERROR, "New Connection Error",  conn->socket().remote_endpoint(), err.message());
                }
                LOG(DEBUG, "Done with new conneciton, Waiting for a new one...");
                // Prime the asio context with more work - again simply wait for
                // another SocketConnection...
                this->WaitForConnection();
        });
    };

    void ConnectToClient(std::shared_ptr<SocketConnection<T>> conn) {
         conn->ssl_socket_stream().async_handshake(asio::ssl::stream_base::server,
            [this, conn](const std::error_code err) {
                if (!err) {
                    LOG(INFO, "Connection approved", conn->socket().remote_endpoint());
                    conn->ReadHeaderFromClient(this, conn);
                } else {
                    LOG(ERROR, "Handshake error", conn->socket().remote_endpoint(), err.message());
                }
            }
        );
    }

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
        for (auto& client : this->deqConnections) {
            // Make sure the client is connected
            if (client && client->IsConnected()) {
                if (client != pIgnoreClient) {
                    client->Send(msg);
                }
            } else {
                // This client shouldn't be contacted, so assume it has been disconnected
                OnClientDisconnect(client);
                client.reset();
            }
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

    void removeConnection(std::shared_ptr<SocketConnection<T>> conn) {
        this->deqConnections.erase(std::remove(this->deqConnections.begin(), this->deqConnections.end(), conn), this->deqConnections.end());
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

protected:
    // Thread safe queue for incoming messages
    tsqueue<OwnedMessage<T> > qMessagesIn;

    // Container of active validated connections
    std::deque<std::shared_ptr<SocketConnection<T>>> deqConnections;   

private:
    asio::io_context io_context;
    asio::ip::tcp::acceptor acceptor;
    asio::ssl::context ssl_context;

    std::thread server_thread;
    std::thread request_thread;

    std::string certPath;
    std::string keyPath;
    std::string caPath;
};