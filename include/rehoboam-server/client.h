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
        
    std::unique_ptr<connection<T>> serverConnection;

private:
    tsqueue<owned_message<T>> qMessagesIn;

public:
    RehoboamClient() {};
    virtual ~RehoboamClient() {
        this->Disconnect();
    }

public:

    bool Connect(const std::string& host, const uint16_t port) {
        try {
			tcp::resolver resolver(this->io_context);
			tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

            this->serverConnection = std::make_unique<connection<T>>(connection<T>::owner::client, this->io_context, asio::ip::tcp::socket(this->io_context), this->qMessagesIn);           

            this->serverConnection->ConnectToServer(endpoints);

            this->thread_context = std::thread([this]() { this->io_context.run(); });
        } catch (std::exception& e) {
            fprintf(stderr, "Client exception: %s", e.what());
            return false;
        }

        return true;
    }

    void Disconnect() {
        if (this->IsConnected()) {
            this->serverConnection->Disconnect();
        }

        this->io_context.stop();

        if (this->thread_context.joinable()) this->thread_context.join();

        this->serverConnection.release();
    }

    bool IsConnected() {
        if (this->serverConnection) {
            return this->serverConnection->IsConnected();
        } else {
            return false;
        }
    }
};