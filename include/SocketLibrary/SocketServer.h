#pragma once

#include <thread>
#include <deque>

#include <Rehoboam/SocketLibrary/common.h>
#include <Rehoboam/SocketLibrary/tsqueue.h>

namespace Rehoboam {
    namespace SocketLibrary {

        /**
         * @class SocketServer
         * @brief A template class that represents a server able to handle connections and communicate over SSL/TLS.
         *
         * @tparam T The message type.
         */
        template<typename T>
        class SocketServer {

        public:
            /**
             * @brief Constructor that initializes the server and starts listening on the specified port.
             *
             * @param port The port number to listen on.
             * @param certPath Path to the server's certificate file.
             * @param keyPath Path to the server's private key file.
             * @param caPath Path to the Certificate Authority file.
             */
            SocketServer(uint16_t port, std::string certPath, std::string keyPath, std::string caPath)
                : m_acceptor(m_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), m_ssl_context(asio::ssl::context::sslv23), 
                m_certPath(certPath), m_keyPath(keyPath), m_caPath(caPath)
            {
                this->m_ssl_context.set_options(
                    asio::ssl::context::default_workarounds 
                    | asio::ssl::context::no_sslv2
                    | asio::ssl::context::single_dh_use);

                // Verify client auth
                this->m_ssl_context.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);
                this->m_ssl_context.load_verify_file(this->m_caPath);

                this->m_ssl_context.use_certificate_file(this->m_certPath, asio::ssl::context::pem);
                this->m_ssl_context.use_private_key_file(this->m_keyPath, asio::ssl::context::pem);
            }

            /**
             * @brief Destructor that stops the server.
             */
            virtual ~SocketServer() {
                this->Stop();
            }

            /**
             * @brief Starts the server.
             *
             * @return true If the server is successfully started.
             * @return false If an exception occurs during startup.
             */
            bool Start() {
                try {   
                    this->WaitForConnection();

                    // Launch the asio context in its own thread
                    this->m_server_thread = std::thread([this]() { this->m_io_context.run(); });
                } catch (std::exception& e) {
                    LOG(ERROR, "Exception", e.what());
                    return false;
                }

                LOG(INFO, "Started");
                return true;
            };

            /**
             * @brief Stops the server and cleans up resources.
             */
            void Stop() {	
                this->m_io_context.stop();

                if (this->m_server_thread.joinable()) this->m_server_thread.join();
                if (this->m_request_thread.joinable()) this->m_request_thread.join();

                LOG(INFO, "Stopped");
            }

            /**
             * @brief Waits asynchronously for a client to connect.
             */
            void WaitForConnection() {
                std::shared_ptr<SocketConnection<T>> conn = std::make_shared<SocketConnection<T>>(SocketConnection<T>::Owner::server, this->m_io_context, this->m_ssl_context, this->m_qMessagesIn);
                this->m_acceptor.async_accept(conn->socket(),
                    [this, conn](std::error_code err) {
                        LOG(DEBUG, "Received a new connection");
                        if (!err) {
                            LOG(INFO, "New Connection", conn->socket().remote_endpoint());

                            if (this->OnClientConnect(conn)) {                
                                this->m_deqConnections.push_back(std::move(conn));
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
                        LOG(DEBUG, "Done with new connection, Waiting for a new one...");
                        this->WaitForConnection();
                });
            };

            /**
             * @brief Connects to a client and performs the SSL handshake.
             *
             * @param conn The connection object representing the client.
             */
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

            /**
             * @brief Sends a message to a specific client.
             *
             * @param client The client to send the message to.
             * @param msg The message to be sent.
             */
            void MessageClient(std::shared_ptr<SocketConnection<T>> client, const Message<T>& msg) {
                if (client && client->IsConnected()) {
                    client->Send(msg);
                } else {
                    this->OnClientDisconnect(client);

                    client.reset();

                    this->m_deqConnections.erase(std::remove(this->m_deqConnections.begin(), this->m_deqConnections.end(), client), this->m_deqConnections.end());
                }
            }

            /**
             * @brief Sends a message to all connected clients.
             *
             * @param msg The message to be sent.
             * @param pIgnoreClient An optional parameter specifying a client to exclude from the broadcast.
             */
            void MessageAllClients(const Message<T>& msg, std::shared_ptr<SocketConnection<T>> pIgnoreClient = nullptr) {
                for (auto& client : this->m_deqConnections) {
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

            /**
             * @brief Handles incoming messages in a separate thread.
             */
            void HandleRequests() {
                this->m_request_thread = std::thread([this]() { 
                    while (true) {
                        this->m_qMessagesIn.wait();
                        while (!this->m_qMessagesIn.empty()) {
                            auto ownedMessage = this->m_qMessagesIn.pop_front();

                            this->OnMessageReceived(ownedMessage.remote, ownedMessage.message);
                        }
                    }
                });    
            }

            /**
             * @brief Handles incoming messages without creating a separate thread.
             */
            void HandleRequestsNoThread() {
                this->m_qMessagesIn.wait();
                while (!this->m_qMessagesIn.empty()) {
                    auto ownedMessage = this->m_qMessagesIn.pop_front();

                    this->OnMessageReceived(ownedMessage.remote, ownedMessage.message);
                }
            }

            /**
             * @brief Removes a connection from the list of active connections.
             *
             * @param conn The connection to remove.
             */
            void removeConnection(std::shared_ptr<SocketConnection<T>> conn) {
                this->m_deqConnections.erase(std::remove(this->m_deqConnections.begin(), this->m_deqConnections.end(), conn), this->m_deqConnections.end());
            }

        protected:
            /**
             * @brief Called when a client successfully connects.
             *
             * @param client The connected client.
             * @return true If the connection is accepted.
             * @return false If the connection is rejected.
             */
            virtual bool OnClientConnect(std::shared_ptr<SocketConnection<T> > client) {
                return false;
            }

            /**
             * @brief Called when a client disconnects.
             *
             * @param client The disconnected client.
             */
            virtual void OnClientDisconnect(std::shared_ptr<SocketConnection<T> > client) {

            }

            /**
             * @brief Called when a message is received from a client.
             *
             * @param client The client that sent the message.
             * @param msg The received message.
             */
            virtual void OnMessageReceived(std::shared_ptr<SocketConnection<T> > client, Message<T>& msg) {

            }

            /**
             * @brief Called when a client is validated.
             */
            virtual void OnClientValidated() {

            }

        protected:
            // Thread-safe queue for incoming messages.
            tsqueue<OwnedMessage<T> > m_qMessagesIn;

            // Container of active validated connections.
            std::deque<std::shared_ptr<SocketConnection<T>>> m_deqConnections;   

        private:
            
            // The ASIO context for handling IO operations.
            asio::io_context m_io_context;
            
            // The TCP acceptor for accepting incoming connections.
            asio::ip::tcp::acceptor m_acceptor;
            
            // The SSL context for handling SSL/TLS operations.
            asio::ssl::context m_ssl_context;
            
            // The thread running the ASIO context.
            std::thread m_server_thread;
            
            // The thread for handling incoming requests.
            std::thread m_request_thread;

            
            // Path to the server's certificate file.
            std::string m_certPath;
            
            // Path to the server's private key file.
            std::string m_keyPath;
            
            // Path to the Certificate Authority file.
            std::string m_caPath;
        };
    }  // namespace SocketLibrary
}  // namespace Rehoboam