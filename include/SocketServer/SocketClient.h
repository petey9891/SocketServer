#pragma once

#include <SocketServer/common.h>
#include <SocketServer/tsqueue.h>
#include <SocketServer/SocketConnection.h>
#include <thread>

using asio::ip::tcp;

template<typename T>
class SocketClient {
protected:
    asio::io_context io_context;
    std::thread thread_context;
    std::thread message_thread;
        
    std::unique_ptr<SocketConnection<T>> m_connection;

private:
    tsqueue<OwnedMessage<T>> qMessagesIn;
    std::string certPath;
    std::string keyPath;
    std::string caPath;

public:
    SocketClient(std::string certPath, std::string keyPath, std::string caPath): certPath(certPath), keyPath(keyPath), caPath(caPath) {};
    virtual ~SocketClient() {
        this->Disconnect();
    }

public:

    // Connect to the server
    bool Connect(const std::string& host, const uint16_t port) {
        try {
			tcp::resolver resolver(this->io_context);
			tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

            asio::ssl::context ssl_context(asio::ssl::context::sslv23);

            ssl_context.set_options(
                asio::ssl::context::default_workarounds 
                | asio::ssl::context::no_sslv2
                | asio::ssl::context::single_dh_use);

            ssl_context.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);
            ssl_context.load_verify_file(this->caPath);

            ssl_context.use_certificate_file(this->certPath, asio::ssl::context::pem);
            ssl_context.use_private_key_file(this->keyPath, asio::ssl::context::pem);

            this->m_connection = std::make_unique<SocketConnection<T>>(SocketConnection<T>::owner::client, this->io_context, ssl_context, this->qMessagesIn);           

            this->m_connection->ConnectToServer(endpoints);

            this->thread_context = std::thread([this]() { this->io_context.run(); });
        } catch (std::exception& e) {
            fprintf(stderr, "Client exception: %s\n", e.what());
            return false;
        }

        return true;
    }

    // Disconnect from the server
    void Disconnect() {
        if (this->IsConnected()) {
            this->m_connection->Disconnect();
        }

        this->io_context.stop();

        if (this->thread_context.joinable()) this->thread_context.join();
        if (this->message_thread.joinable()) this->message_thread.join();

        this->m_connection.release();
    }

    // Returns true if connected to the server
    bool IsConnected() {
        if (this->m_connection) {
            return this->m_connection->IsConnected();
        } else {
            return false;
        }
    }

    void HandleMessages() {
        this->message_thread = std::thread([this]() {
            while (true) {
                this->qMessagesIn.wait();
                while (!this->qMessagesIn.empty()) {
                    auto ownedMessage = this->qMessagesIn.pop_front();
                    this->OnMessageRecieved(ownedMessage.message);
                }
            }
        });
    }

    void HandleMessagesNoThread() {
        this->qMessagesIn.wait();
        while (!this->qMessagesIn.empty()) {
            auto ownedMessage = this->qMessagesIn.pop_front();
            this->OnMessageRecieved(ownedMessage.message);
        }
    }

    void Send(const Message<T>msg) {
        if (this->IsConnected()) {
            this->m_connection->Send(msg);
        }
    }

    tsqueue<OwnedMessage<T>>& IncomingMessages() {
        return this->qMessagesIn;
    }

protected: 
    virtual void OnMessageRecieved(Message<T>& msg) {

    }
};