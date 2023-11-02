#pragma once

#include <thread>
#include <Rehoboam/SocketLibrary/common.h>
#include <Rehoboam/SocketLibrary/tsqueue.h>
#include <Rehoboam/SocketLibrary/SocketConnection.h>

namespace Rehoboam {
    namespace SocketLibrary {

        /**
         * @class SocketServer
         * @brief A template class that represents a server able to handle connections and communicate over SSL/TLS.
         *
         * @tparam T The message type.
         */
        template<typename T>
        class SocketClient {

        public:
            /**
             * @brief Constructor for SocketClient.
             *
             * @param host The server's hostname or IP address.
             * @param port The server's port number.
             * @param certPath Path to the SSL certificate file.
             * @param keyPath Path to the SSL private key file.
             * @param caPath Path to the SSL certificate authority file.
             * @param type The type of client.
             */
            SocketClient(const std::string& host, const uint16_t port, std::string certPath, std::string keyPath, std::string caPath, ClientType type)
                : m_host(host), m_port(port), m_certPath(certPath), m_keyPath(keyPath), m_caPath(caPath), m_clientType(type), m_ssl_context(asio::ssl::context::sslv23)
            {
                this->Initialize();
            };

            /**
            * @brief Destructor for SocketClient.
            */
            virtual ~SocketClient() {
                this->Disconnect();
            }

            /**
            * @brief Initialize SSL context and settings.
            */
            void Initialize() {
                    this->m_ssl_context.set_options(
                        asio::ssl::context::default_workarounds 
                        | asio::ssl::context::no_sslv2
                        | asio::ssl::context::single_dh_use);

                    this->m_ssl_context.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);
                    this->m_ssl_context.load_verify_file(this->m_caPath);

                    this->m_ssl_context.use_certificate_file(this->m_certPath, asio::ssl::context::pem);
                    this->m_ssl_context.use_private_key_file(this->m_keyPath, asio::ssl::context::pem);
            }

            /**
            * @brief Start the connection process to the server.
            */
            void Connect() {
                this->m_thread_context = std::thread([this]() {
                    LOG(INFO, "Connecting to server");
                    LOG(INFO, "Auto-connect is turned on");
                    this->AttemptConnection();
                    this->m_io_context.run();
                });
            }

            /**
            * @brief Attempt to establish a connection to the server.
            */
            void AttemptConnection() {
                try {
                    asio::ip::tcp::resolver resolver(this->m_io_context);
                    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(this->m_host, std::to_string(this->m_port));

                    this->m_connection = std::make_unique<SocketConnection<T>>(SocketConnection<T>::Owner::client, this->m_io_context, this->m_ssl_context, this->m_qMessagesIn);

                    this->ConnectToServer(endpoints);
                } catch (std::exception& e) {
                    LOG(ERROR, "Exception", e.what());
                }
            }

            /**
             * @brief Attempt to connect to the server using the provided endpoints.
             *
             * @param endpoints The endpoints to attempt connection to.
             */
            void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
                asio::async_connect(this->m_connection->socket(), endpoints,
                    [this, endpoints](std::error_code err, asio::ip::tcp::endpoint endpoint) {
                        if (!err) {
                            this->m_connection->ssl_socket_stream().async_handshake(asio::ssl::stream_base::client,
                                [this](std::error_code hErr) {
                                    LOG(INFO, "Connected to server");
                                    if (!hErr) {

                                        if (this->m_clientType == CUBE) {
                                            LOG(INFO, "Initializing heartbeat");
                                            this->Pulse();
                                        }

                                        this->m_connection->ReadHeaderFromServer(
                                            [this](std::runtime_error rErr) {
                                                LOG(ERROR, "Connection Error", rErr.what());
                                                LOG(INFO, "Initiating Auto-connect");
                                                this->Reconnect();
                                            }
                                        );
                                    } else {
                                        LOG(ERROR, "Handshake Error", hErr.message());
                                        this->Reconnect();
                                    }
                                }
                            );
                        } else {
                            this->Reconnect();
                        }
                    }
                );
            }

            /**
            * @brief Reconnect to the server after a connection loss.
            */
            void Reconnect() {
                this->m_connection->socket().close();
                this->m_connection.reset();
                this->m_pulse_timer.cancel();

                this->m_timer.expires_from_now(asio::chrono::seconds(5));
                this->m_timer.async_wait([this](const std::error_code& err) {
                    if (!err) {
                        this->AttemptConnection();
                    } else {
                        LOG(ERROR, "Reconnection error", err.message());
                    }
                });
            }

            /**
             * @brief Send a heartbeat message to the server to keep the connection alive.
             */
            void Pulse() {
                if (this->IsConnected()) {
                    this->m_pulse_timer.expires_from_now(asio::chrono::seconds(10));
                    this->m_pulse_timer.async_wait([this](const std::error_code& err) {
                        if (!err) {
                            LOG(DEBUG, "Sending pulse check");
                            Message<MessageType> message;
                            message.header.id = ServerPing;
                            this->m_connection->Send(message);
                            this->Pulse();
                        } else {
                            LOG(ERROR, "Heartbeat error:", err.message());
                        }
                    });
                }
            }

            /**
             * @brief Disconnect from the server.
             */
            void Disconnect() {
                if (this->IsConnected()) {
                    this->m_connection->Disconnect();
                }

                this->m_io_context.stop();

                if (this->m_thread_context.joinable()) this->m_thread_context.join();
                if (this->m_message_thread.joinable()) this->m_message_thread.join();

                this->m_connection.release();
            }

            /**
             * @brief Check if the client is currently connected to the server.
             *
             * @return True if connected, false otherwise.
             */
            bool IsConnected() {
                if (this->m_connection) {
                    return this->m_connection->IsConnected();
                } else {
                    return false;
                }
            }

            /**
             * @brief Start a thread to handle incoming messages.
             */
            void HandleMessages() {
                this->m_message_thread = std::thread([this]() {
                    while (true) {
                        this->m_qMessagesIn.wait();
                        while (!this->m_qMessagesIn.empty()) {
                            auto ownedMessage = this->m_qMessagesIn.pop_front();
                            this->OnMessageReceived(ownedMessage.message);
                        }
                    }
                });
            }

            /**
             * @brief Handle incoming messages without creating a new thread.
             */
            void HandleMessagesNoThread() {
                this->m_qMessagesIn.wait();
                while (!this->m_qMessagesIn.empty()) {
                    auto ownedMessage = this->m_qMessagesIn.pop_front();
                    this->OnMessageReceived(ownedMessage.message);
                }
            }

            /**
             * @brief Send a message to the server.
             *
             * @param msg The message to send.
             */
            void Send(const Message<T>msg) {
                if (this->IsConnected()) {
                    this->m_connection->Send(msg);
                }
            }

            /**
             * @brief Access the queue of incoming messages.
             *
             * @return A reference to the queue of incoming messages.
             */
            tsqueue<OwnedMessage<T>>& IncomingMessages() {
                return this->m_qMessagesIn;
            }

        protected:
            /**
             * @brief Virtual function to handle incoming messages. To be implemented by derived classes.
             *
             * @param msg The incoming message.
             */
            virtual void OnMessageReceived(Message<T>& msg) {

            }

            // The ASIO io_context used for async operations.
            asio::io_context m_io_context;

            // Thread to run the ASIO io_context.
            std::thread m_thread_context;

            // Thread to handle incoming messages.
            std::thread m_message_thread;

            // The SocketConnection object managing the client's connection to the server.
            std::unique_ptr<SocketConnection<T>> m_connection;

        private:
            
            // Thread-safe queue for incoming messages.
            tsqueue<OwnedMessage<T>> m_qMessagesIn;

            // Connection configuration
            // The server's hostname or IP address.
            std::string m_host;
            
            // The server's port number.
            uint16_t m_port;
            
            // Path to the SSL certificate file.
            std::string m_certPath;
            
            // Path to the SSL private key file.
            std::string m_keyPath;
            
            // Path to the SSL certificate authority file.
            std::string m_caPath;
            
            // The type of client.
            ClientType m_clientType;
            
            // ASIO SSL context for secure connections.
            asio::ssl::context m_ssl_context;

            // Timers for connection management
            // Timer for handling reconnection attempts.
            asio::steady_timer m_timer { this->m_io_context, asio::chrono::seconds(2) };
            
            // Timer for sending heartbeat messages to the server.
            asio::steady_timer m_pulse_timer { this->m_io_context, asio::chrono::seconds(10) };

            // Counter for connection errors.
            uint8_t m_errorCount;

        };
    }  // namespace SocketLibrary
}  // namespace Rehoboam