#pragma once

#include <rehoboam-server/common.h>

using asio::ip::tcp;

template<typename T>
class connection: public std::enable_shared_from_this<connection<T>> {
protected:
    // Each connection has a unique socket to a remote 
    asio::ip::tcp::socket socket;

    // This context is shared with the whole asio instance
    asio::io_context& asioContext;

    owner ownerType;

public:
    // A connection is either owned by a server or a client and it behaves differently depending on who
    enum class owner {
        server,
        client
    }

public:
    connection(owner parent, asio::io_context asioContext, tcp::socket socket): context(context), socket(std::move(socket)) {
        this->ownerType = parent;
    }

    virtual ~connection() {}

    void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
        // Only clients can connect to a server
        if (this->ownerType == owner::client) {
            
        }
    }
};