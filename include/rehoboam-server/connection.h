#pragma once

#include <rehoboam-server/common.h>

using asio::ip::tcp;

// foward declare
template <typename T>
class RehoboamServer;

template <typename T>
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

    // All messages to be sent to the remove side
    tsqueue<Message<T>> qMessagesOut;

    // All messages that are incoming to the parent
    tsqueue<OwnedMessage<T>>& qMessagesIn;

    // A temporary message to be passed around
    Message<T> msgTmpIn;

    owner ownerType;

public:
    connection(owner parent, asio::io_context& asioContext, tcp::socket socket, tsqueue<OwnedMessage<T>>& qIn)
        : asioContext(asioContext), socket(std::move(socket)), qMessagesIn(qIn)
    {
        this->ownerType = parent;
    }

    virtual ~connection() {}

public:
    void ConnectToClient(RehoboamServer<T>* server) {
        // Only servers can connect to clients
        if (this->ownerType == owner::server) {
            if (this->socket.is_open()) {
                this->ReadHeader();
            }
        }
    }

    void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
        // Only clients can connect to a server
        if (this->ownerType == owner::client) {
            asio::async_connect(this->socket, endpoints,
                [this](std::error_code err, asio::ip::tcp::endpoint endpoint) {
                    if (!err) {
                        this->ReadHeader();
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

public:
    // ASYNC - Send a message
    void Send(const Message<T>& msg) {
        asio::post(this->asioContext, 
            [this, msg]() {
                bool isWritingMessage = this->qMessagesOut.empty();
                this->qMessagesOut.push_back(msg);
                if (!isWritingMessage) {
                    this->WriteHeader();
                }
            }
        );
    }

private:
    // ASYNC - Prime context to write a message header
    void WriteHeader() {
        asio::async_write(this->socket, asio::buffer(&this->qMessagesOut.front().header, sizeof(MessageHeader<T>)),
            [this](std::error_code err, std::size_t length) {
                if (!err) {
                    // Check if the message header just sent also had a body
                    if (this->qMessagesOut.front().body.size() > 0) {
                        // It would be nice to also send the body...
                        // this->WriteBody();
                    } else {
                        this->qMessagesOut.pop_front();

                        // If the queue isn't empty, keep working
                        if (!this->qMessagesOut.empty()) {
                            this->WriteHeader();
                        }
                    }
                } else {
                    printf("Write header fail\n");
                    this->socket.close();
                }
            }
        );
    }

    // ASYNC - Prime context ready to read a message body
    void ReadHeader() {
        asio::async_read(this->socket, asio::buffer(&this->msgTmpIn.header, sizeof(MessageHeader<T>)),
            [this](std::error_code err, std::size_t length) {
                if (!err) {
                    // Check if the header just read also has a body
                    if (this->msgTmpIn.header.size > 0) {
                        // It would be nice to know what else was sent...
                        this->msgTmpIn.body.resize(this->msgTmpIn.header.size);
                        // this->ReadBody();
                    } else {
                        this->AddToIncomingMessageQueue();
                    }
                } else {
                    printf("Read header fail\n");
                    this->socket.close(); 
                }
            }
        );
    }

    void AddToIncomingMessageQueue() {
        // If it is a server, throw it into the queue as a "owned message"
        if (this->ownerType == owner::server) {
            this->qMessagesIn.push_back({ this->shared_from_this(), this->msgTmpIn });
        } else {
            this->qMessagesIn.push_back({ nullptr, this->msgTmpIn });
        }

        this->ReadHeader();
    }
};