#pragma once

#include <stdexcept>
#include <Rehoboam/SocketLibrary/common.h>

namespace Rehoboam {
    namespace SocketLibrary {

        // Forward declare
        template <typename T>
        class SocketServer;

        typedef asio::ssl::stream<asio::ip::tcp::socket> ssl_socket;

        /**
         * @class SocketConnection
         * @brief Represents a network connection between a client and a server using SSL.
         * @tparam T The type of the message that will be sent over the connection.
         */
        template <typename T>
        class SocketConnection: public std::enable_shared_from_this<SocketConnection<T>> {
        public:
            /**
             * @enum Owner
             * @brief Represents the ownership of the SocketConnection.
             */
            enum class Owner { server, client };

            /**
             * @brief Construct a new Socket Connection object.
             * @param parent The owner of this connection (either server or client).
             * @param m_asio_context The ASIO context shared between all network operations.
             * @param m_ssl_context The SSL context used to secure the connection.
             * @param qIn The thread-safe queue for incoming messages.
             */
            SocketConnection(Owner parent, asio::io_context& m_asio_context, asio::ssl::context& m_ssl_context, tsqueue<OwnedMessage<T>>& qIn)
                : m_asio_context(m_asio_context), m_ssl_context(m_ssl_context), m_socket(m_asio_context, m_ssl_context), m_qMessagesIn(qIn)
            {
                this->m_owner = parent;
            }

            /**
            * @brief Destroy the Socket Connection object.
            */
            virtual ~SocketConnection() {}

            /**
             * @brief Get the SSL socket stream object.
             * @return ssl_socket& Reference to the SSL socket.
             */
            ssl_socket& ssl_socket_stream() {
                return this->m_socket;
            }

            /**
             * @brief Get the lowest layer of the SSL socket.
             * @return ssl_socket::lowest_layer_type& Reference to the lowest layer of the SSL socket.
             */
            ssl_socket::lowest_layer_type& socket() {
                return this->m_socket.lowest_layer();
            }

            /**
             * @brief Disconnect the socket connection.
             */
            void Disconnect() {
                LOG(DEBUG, "Checking if client is still connected before closing the socket");
                if (this->IsConnected()) {
                    LOG(DEBUG, "The client is still connected... closing the socket");
                    LOG(DEBUG, "The current socket connection is open", this->IsConnected());
                    LOG(DEBUG, "The current m_socket.lowest_layer() connection is open", this->m_socket.lowest_layer().is_open());

                    this->socket().close();

                    LOG(DEBUG, "The socket is now closed");
                }
            }

            /**
             * @brief Check if the socket connection is open.
             * @return true If the socket is open.
             * @return false If the socket is closed.
             */
            bool IsConnected() {
                return this->socket().is_open();
            }

            /**
             * @brief Send a message asynchronously.
             * @param msg The message to be sent.
             */
            void Send(const Message<T>& msg) {
                asio::post(this->m_asio_context, 
                    [this, msg]() {
                        bool isWritingMessage = !this->m_qMessagesOut.empty();
                        this->m_qMessagesOut.push_back(msg);
                        if (!isWritingMessage) {
                            this->WriteHeader();
                        }
                    }
                );
            }

            /**
             * @brief Read the message header asynchronously from a client.
             * @param server Pointer to the server instance.
             * @param conn Shared pointer to this SocketConnection instance.
             */
            void ReadHeaderFromClient(SocketServer<T>* server, std::shared_ptr<SocketConnection<T>> conn) {
                asio::async_read(this->m_socket, asio::buffer(&this->m_msgTmpIn.header, sizeof(MessageHeader<T>)),
                    [this, server, conn](std::error_code err, std::size_t length) {
                        if (!err) {
                            // Check if there is also a body
                            if (this->m_msgTmpIn.header.size > 0) {
                                // It would be nice to know what else was sent...
                                this->m_msgTmpIn.body.resize(this->m_msgTmpIn.header.size);
                                
                                this->ReadBodyFromClient(server, conn);
                            } else {
                                this->AddToIncomingMessageQueueFromClient(server, conn);
                            }
                        } else {
                            LOG(INFO, "Disconnected from client", this->socket().remote_endpoint());
                            this->Disconnect();
                            
                            server->removeConnection(conn);
                            LOG(DEBUG, "Client connection has been removed from store");
                        }
                    }
                );
            }

            /**
             * @brief Read the message header asynchronously from a server.
             * @tparam ErrorCompletion Type of the error completion handler.
             * @param handler The error completion handler.
             */
            template<typename ErrorCompletion>
            void ReadHeaderFromServer(ErrorCompletion&& handler) {
                asio::async_read(this->m_socket, asio::buffer(&this->m_msgTmpIn.header, sizeof(MessageHeader<T>)),
                    [this, handler](std::error_code err, std::size_t length) {
                        if (!err) {
                            // Check if there is also a body
                            if (this->m_msgTmpIn.header.size > 0) {
                                // It would be nice to know what else was sent...
                                this->m_msgTmpIn.body.resize(this->m_msgTmpIn.header.size);
                                
                                this->ReadBodyFromServer(handler);
                            } else {
                                this->AddToIncomingMessageQueueFromServer(handler);
                            }
                        } else {
                            this->Disconnect();
                            handler(std::runtime_error("Unexpectedly disconnected from the server"));
                        }
                    }
                );
            }

        protected:
            // This context is shared with the whole asio instance
            asio::io_context& m_asio_context;

            // SSL context for securing network communications.
            asio::ssl::context& m_ssl_context;

            // SSL socket that represents the connection.
            ssl_socket m_socket;

            // Thread-safe queue for outgoing messages.
            tsqueue<Message<T>> m_qMessagesOut;

            // Thread-safe queue for incoming messages.
            tsqueue<OwnedMessage<T>>& m_qMessagesIn;

            // Temporary storage for the current incoming message.
            Message<T> m_msgTmpIn;

            // The ownership of this connection.
            Owner m_owner;

        private:
            /**
             * @brief Write the message header asynchronously.
             */
            void WriteHeader() {
                asio::async_write(this->m_socket, asio::buffer(&this->m_qMessagesOut.front().header, sizeof(MessageHeader<T>)),
                    [this](std::error_code err, std::size_t length) {
                        if (!err) {
                            // Check if there is also a body
                            if (this->m_qMessagesOut.front().body.size() > 0) {
                                // It would be nice to also send the body...
                                this->WriteBody();
                            } else {
                                this->m_qMessagesOut.pop_front();

                                // If the queue isn't empty, keep working
                                if (!this->m_qMessagesOut.empty()) {
                                    this->WriteHeader();
                                }
                            }
                        } else {
                            LOG(ERROR, "Write header fail -- closing socket", this->socket().remote_endpoint(), err.message());
                            this->Disconnect();
                        }
                    }
                );
            }

            /**
             * @brief Write the message body asynchronously.
             */
            void WriteBody() {
                asio::async_write(this->m_socket, asio::buffer(this->m_qMessagesOut.front().body.data(), this->m_qMessagesOut.front().body.size()),
                    [this](std::error_code err, std::size_t length) {
                        if (!err) {
                            // Sending was successful so we are done with this message
                            this->m_qMessagesOut.pop_front();

                            // If there are still messages to send, issue a task to write the header
                            if (!this->m_qMessagesOut.empty()) {
                                this->WriteHeader();
                            }
                        } else {
                            LOG(ERROR, "Write body fail -- closing socket", this->socket().remote_endpoint(), err.message());
                            this->Disconnect();
                        }
                    }
                );
            }

            /**
             * @brief Read the message body asynchronously from a client.
             * @param server Pointer to the server instance.
             * @param conn Shared pointer to this SocketConnection instance.
             */
            void ReadBodyFromClient(SocketServer<T>* server, std::shared_ptr<SocketConnection<T>> conn) {
                // If this function is called then that means the header has already been read and in the request we have a body
                // The space for that body has already been allocated in the m_msgTmpIn object
                asio::async_read(this->m_socket, asio::buffer(this->m_msgTmpIn.body.data(), this->m_msgTmpIn.body.size()),
                    [this, server, conn](std::error_code err, std::size_t length) {
                        if (!err) {
                            this->AddToIncomingMessageQueueFromClient(server, conn);
                        } else {
                            LOG(ERROR, "Read body fail -- closing socket to client", this->socket().remote_endpoint(), err.message());
                            this->Disconnect();
                        }
                    }
                );
            }

            /**
             * @brief Read the message body asynchronously from a server.
             * @tparam ErrorCompletion Type of the error completion handler.
             * @param handler The error completion handler.
             */
            template<typename ErrorCompletion>
            void ReadBodyFromServer(ErrorCompletion&& handler) {
                // If this function is called then that means the header has already been read and in the request we have a body
                // The space for that body has already been allocated in the m_msgTmpIn object
                asio::async_read(this->m_socket, asio::buffer(this->m_msgTmpIn.body.data(), this->m_msgTmpIn.body.size()),
                    [this, handler](std::error_code err, std::size_t length) {
                        if (!err) {
                            this->AddToIncomingMessageQueueFromServer(handler);
                        } else {
                            LOG(ERROR, "Read body fail -- closing socket to server", err.message());
                            this->Disconnect();
                            handler(std::runtime_error("Disconnected from server after read body fail"));
                        }
                    }
                );
            }

            /**
             * @brief Add the incoming message to the queue.
             * @tparam TCallable Type of the callable continuation function.
             * @param continueReading The continuation function to continue reading messages.
             */
            template<typename TCallable>
            void AddToIncomingMessageQueue(TCallable&& continueReading) {
                // If it is a server, throw it into the queue as an "owned message"
                if (this->m_owner == Owner::server) {
                    this->m_qMessagesIn.push_back({ this->shared_from_this(), this->m_msgTmpIn });
                } else {
                    this->m_qMessagesIn.push_back({ nullptr, this->m_msgTmpIn });
                }

                this->m_msgTmpIn.clear();

                continueReading();
            }

            /**
             * @brief Add the incoming message to the queue from a client.
             * @param server Pointer to the server instance.
             * @param conn Shared pointer to this SocketConnection instance.
             */
            void AddToIncomingMessageQueueFromClient(SocketServer<T>* server, std::shared_ptr<SocketConnection<T>> conn) {
                AddToIncomingMessageQueue([this, server, conn]() {
                    this->ReadHeaderFromClient(server, conn);
                });
            }

            /**
             * @brief Add the incoming message to the queue from a server.
             */
            template<typename ErrorCompletion>
            void AddToIncomingMessageQueueFromServer(ErrorCompletion&& handler) {
                AddToIncomingMessageQueue([this, handler = std::forward<ErrorCompletion>(handler)]() mutable {
                    this->ReadHeaderFromServer(std::move(handler));
                });
            }
        };
    }  // namespace SocketLibrary
}  // namespace Rehoboam