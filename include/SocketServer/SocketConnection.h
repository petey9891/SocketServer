#pragma once

#include <SocketServer/common.h>

using asio::ip::tcp;

// foward declare
template <typename T>
class SocketServer;

typedef asio::ssl::stream<asio::ip::tcp::socket> ssl_socket;

template <typename T>
class SocketConnection: public std::enable_shared_from_this<SocketConnection<T>> {
public:
    // A SocketConnection is either owned by a server or a client and it behaves differently depending on who
    enum class owner {
        server,
        client
    };

protected:
    // This context is shared with the whole asio instance
    asio::io_context& asioContext;
    asio::ssl::context& ssl_context;

    // Each SocketConnection has a unique socket to a remote 
    ssl_socket _socket;

    // All messages to be sent to the remove side
    tsqueue<Message<T>> qMessagesOut;

    // All messages that are incoming to the parent
    tsqueue<OwnedMessage<T>>& qMessagesIn;

    // A temporary message ato be passed around
    Message<T> msgTmpIn;

    owner ownerType;

public:
    SocketConnection(owner parent, asio::io_context& asioContext, asio::ssl::context& ssl_context, tsqueue<OwnedMessage<T>>& qIn)
        : asioContext(asioContext), ssl_context(ssl_context), _socket(asioContext, ssl_context), qMessagesIn(qIn)
    {
        this->ownerType = parent;
    }

    virtual ~SocketConnection() {}

public:
    ssl_socket& ssl_socket_stream() {
        return this->_socket;
    }

    ssl_socket::lowest_layer_type& socket() {
        return this->_socket.lowest_layer();
    }

    void Disconnect() {
        LOG(DEBUG, "Checking if client is still connected before closing the socket");
        if (this->IsConnected()) {
            LOG(DEBUG, "The client is still connected... closing the socket");
            LOG(DEBUG, "The current socket connection is open: %d", this->IsConnected());
            LOG(DEBUG, "The current _socket.lowest_layer() connection is open: %d", this->_socket.lowest_layer().is_open());
            this->socket().close();
            LOG(DEBUG, "The socket is now closed");

        }
    }

    bool IsConnected() {
        return this->socket().is_open();
    }

    // bool IsConnected() const {
    //     // Can't call this->socket() here
    //     // return this->socket().is_open();
    //     return this->_socket.lowest_layer().is_open();
    // }

public:
    // ASYNC - Send a message
    void Send(const Message<T>& msg) {
        asio::post(this->asioContext, 
            [this, msg]() {
                bool isWritingMessage = !this->qMessagesOut.empty();
                this->qMessagesOut.push_back(msg);
                if (!isWritingMessage) {
                    this->WriteHeader();
                }
            }
        );
    }

    void ReadHeaderFromClient(SocketServer<T>* server, std::shared_ptr<SocketConnection<T>> conn) {
        asio::async_read(this->_socket, asio::buffer(&this->msgTmpIn.header, sizeof(MessageHeader<T>)),
            [this, server, conn](std::error_code err, std::size_t length) {
                LOG(DEBUG, "Reading header from client");
                if (!err) {
                    // Check if the header just read also has a body
                    if (this->msgTmpIn.header.size > 0) {
                        // It would be nice to know what else was sent...
                        this->msgTmpIn.body.resize(this->msgTmpIn.header.size);
                        
                        this->ReadBodyFromClient(server, conn);
                    } else {
                        this->AddToIncomingMessageQueueFromClient(server, conn);
                    }
                } else {
                    LOG(INFO, "Disconnected from client", this->socket().remote_endpoint());
                    this->Disconnect();
                    LOG(DEBUG, "Client has been disconnected");
                    server->removeConnection(conn);
                    LOG(DEBUG, "Client connection has been removed from store");
                }
            }
        );
    }

        // ASYNC - Prime context ready to read a message header
    void ReadHeaderFromServer() {
        asio::async_read(this->_socket, asio::buffer(&this->msgTmpIn.header, sizeof(MessageHeader<T>)),
            [this](std::error_code err, std::size_t length) {
                if (!err) {
                    // Check if the header just read also has a body
                    if (this->msgTmpIn.header.size > 0) {
                        // It would be nice to know what else was sent...
                        this->msgTmpIn.body.resize(this->msgTmpIn.header.size);
                        
                        this->ReadBodyFromServer();
                    } else {
                        this->AddToIncomingMessageQueueFromServer();
                    }
                } else {
                    LOG(INFO, "Disconnected from server");
                    this->Disconnect();
                    LOG(DEBUG, "Client has been disconnected");
                }
            }
        );
    }

private:
    // ASYNC - Prime context to write a message header
    void WriteHeader() {
        asio::async_write(this->_socket, asio::buffer(&this->qMessagesOut.front().header, sizeof(MessageHeader<T>)),
            [this](std::error_code err, std::size_t length) {
                if (!err) {
                    // Check if the message header just sent also had a body
                    if (this->qMessagesOut.front().body.size() > 0) {
                        // It would be nice to also send the body...
                        this->WriteBody();
                    } else {
                        this->qMessagesOut.pop_front();

                        // If the queue isn't empty, keep working
                        if (!this->qMessagesOut.empty()) {
                            this->WriteHeader();
                        }
                    }
                } else {
                    LOG(ERROR, "Write header fail -- closing socket", this->socket().remote_endpoint(), err.message());
                    this->Disconnect();
                    LOG(DEBUG, "Client has been disconnected");
                }
            }
        );
    }

    void WriteBody() {
        asio::async_write(this->_socket, asio::buffer(this->qMessagesOut.front().body.data(), this->qMessagesOut.front().body.size()),
            [this](std::error_code err, std::size_t length) {
                if (!err) {
                    // Sending was successful so we are done with this message
                    this->qMessagesOut.pop_front();

                    // If there are still messages to send, issue a task to write the header
                    if (!this->qMessagesOut.empty()) {
                        this->WriteHeader();
                    }
                } else {
                    LOG(ERROR, "Write body fail -- closing socket", this->socket().remote_endpoint(), err.message());
                    this->Disconnect();
                    LOG(DEBUG, "Client has been disconnected");
                }
            }
        );
    }

    // ASYNC - Prime context ready to read a message body
    void ReadBodyFromClient(SocketServer<T>* server, std::shared_ptr<SocketConnection<T>> conn) {
        // If this function is called then that means the header has already been read and in the request we have a body
        // The space for that body has already been allocated in the msgTmpIn object
        asio::async_read(this->_socket, asio::buffer(this->msgTmpIn.body.data(), this->msgTmpIn.body.size()),
            [this, server, conn](std::error_code err, std::size_t length) {
                if (!err) {
                    this->AddToIncomingMessageQueueFromClient(server, conn);
                } else {
                    LOG(ERROR, "Read body fail -- closing socket to client", this->socket().remote_endpoint(), err.message());
                    this->Disconnect();
                    LOG(DEBUG, "Client has been disconnected");
                }
            }
        );
    }

    void ReadBodyFromServer() {
        // If this function is called then that means the header has already been read and in the request we have a body
        // The space for that body has already been allocated in the msgTmpIn object
        asio::async_read(this->_socket, asio::buffer(this->msgTmpIn.body.data(), this->msgTmpIn.body.size()),
            [this](std::error_code err, std::size_t length) {
                if (!err) {
                    this->AddToIncomingMessageQueueFromServer();
                } else {
                    LOG(ERROR, "Read body fail -- closing socket to server", err.message());
                    this->Disconnect();
                    LOG(DEBUG, "Client has been disconnected");
                }
            }
        );
    }

        void AddToIncomingMessageQueueFromClient(SocketServer<T>* server, std::shared_ptr<SocketConnection<T>> conn) {
        // If it is a server, throw it into the queue as a "owned message"
        if (this->ownerType == owner::server) {
            this->qMessagesIn.push_back({ this->shared_from_this(), this->msgTmpIn });
        } else {
            this->qMessagesIn.push_back({ nullptr, this->msgTmpIn });
        }

        this->msgTmpIn.clear();

        this->ReadHeaderFromClient(server, conn);
    }

    void AddToIncomingMessageQueueFromServer() {
        // If it is a server, throw it into the queue as a "owned message"
        if (this->ownerType == owner::server) {
            this->qMessagesIn.push_back({ this->shared_from_this(), this->msgTmpIn });
        } else {
            this->qMessagesIn.push_back({ nullptr, this->msgTmpIn });
        }

        this->msgTmpIn.clear();

        this->ReadHeaderFromServer();
    }
};