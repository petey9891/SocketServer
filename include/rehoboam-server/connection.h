#pragma once

#include <rehoboam-server/common.h>

using asio::ip::tcp;

template<typename T>
class connection: public std::enable_shared_from_this<connection<T>> {
public:
    // A connection is either owned by a server or a client and it behaves differently depending on who
    enum class owner {
        server,
        client
    };

protected:
    // This context is shared with the whole asio instance
    asio::io_context& asioContext;

    // Each connection has a unique socket to a remote 
    asio::ip::tcp::socket socket;

    tsqueue<owned_message<T>>& qMessagesIn;

    owner ownerType;

public:
    connection(owner parent, asio::io_context& asioContext, tcp::socket socket, tsqueue<owned_message<T>>& qIn)
        : asioContext(asioContext), socket(std::move(socket)), qMessagesIn(qIn)
    {
        this->ownerType = parent;
    }

    virtual ~connection() {}

    void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
        // Only clients can connect to a server
        if (this->ownerType == owner::client) {
            asio::async_connect(this->socket, endpoints,
                [](std::error_code err, asio::ip::tcp::endpoint endpoint) {
                    if (!err) {
                        printf("Connected to the server!\n");
                    }
                }
            );
        }
    }

    void Disconnect() {
        if (this->IsConnected()) {
            asio::post(this->asioContext, [this]() { this->socket.close(); });
        }
    }

    bool IsConnected() const {
        return this->socket.is_open();
    }
};