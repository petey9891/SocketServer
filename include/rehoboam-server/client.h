#pragma once

#include <rehoboam-server/common.h>
#include <rehoboam-server/tsqueue.h>
#include <rehoboam-server/connection.h>
#include <thread>

using asio::ip::tcp;

template<typename T>
class RehoboamClient {
protected:
    asio::io_context io_context;
    std::thread thread_context;
        
    std::unique_ptr<connection<T>> m_connection;

private:
    tsqueue<OwnedMessage<T>> qMessagesIn;

public:
    RehoboamClient() {};
    virtual ~RehoboamClient() {
        this->Disconnect();
    }

public:

    // Connect to the server
    bool Connect(const std::string& host, const uint16_t port) {
        try {
			tcp::resolver resolver(this->io_context);
			tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

            this->m_connection = std::make_unique<connection<T>>(connection<T>::owner::client, this->io_context, asio::ip::tcp::socket(this->io_context), this->qMessagesIn);           

            this->m_connection->ConnectToServer(endpoints);

            this->thread_context = std::thread([this]() { this->io_context.run(); });
        } catch (std::exception& e) {
            fprintf(stderr, "Client exception: %s", e.what());
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

public:
    void Send(const Message<T>msg) {
        if (this->IsConnected()) {
            this->m_connection->Send(msg);
        }
    }

    tsqueue<OwnedMessage<T>>& IncomingMessages() {
        return this->qMessagesIn;
    }
};