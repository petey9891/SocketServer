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

    std::string host;
    uint16_t port;


    std::string certPath;
    std::string keyPath;
    std::string caPath;

    ClientType clientType;
    asio::ssl::context ssl_context;

    asio::steady_timer m_timer { this->io_context, asio::chrono::seconds(2) };
    asio::steady_timer pulse_timer { this->io_context, asio::chrono::seconds(10) };

    uint8_t errorCount = 0;

public:
    SocketClient(const std::string& host, const uint16_t port, std::string certPath, std::string keyPath, std::string caPath, ClientType type)
        : host(host), port(port), certPath(certPath), keyPath(keyPath), caPath(caPath), clientType(type), ssl_context(asio::ssl::context::sslv23)
    {
        this->Initialize();
    };
    
    virtual ~SocketClient() {
        this->Disconnect();
    }

public:

    void Initialize() {
            this->ssl_context.set_options(
                asio::ssl::context::default_workarounds 
                | asio::ssl::context::no_sslv2
                | asio::ssl::context::single_dh_use);

            this->ssl_context.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);
            this->ssl_context.load_verify_file(this->caPath);

            this->ssl_context.use_certificate_file(this->certPath, asio::ssl::context::pem);
            this->ssl_context.use_private_key_file(this->keyPath, asio::ssl::context::pem);
    }

    // Connect to the server
    void Connect() {
        this->thread_context = std::thread([this]() {
            LOG(INFO, "Connecting to server");
            LOG(INFO, "Autoconnect is turned on");
            this->AttemptConnection();
            this->io_context.run();
        });
    }

    void AttemptConnection() {
        try {
			tcp::resolver resolver(this->io_context);
			tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

            this->m_connection = std::make_unique<SocketConnection<T>>(SocketConnection<T>::owner::client, this->io_context, ssl_context, this->qMessagesIn);           

            this->ConnectToServer(endpoints);
        } catch (std::exception& e) {
            LOG(ERROR, "Exception", e.what());
        }
    }

    void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
        asio::async_connect(this->m_connection->socket(), endpoints,
            [this, endpoints](std::error_code err, asio::ip::tcp::endpoint endpoint) {
                if (!err) {
                    this->m_connection->ssl_socket_stream().async_handshake(asio::ssl::stream_base::client,
                        [this](std::error_code hErr) {
                            LOG(INFO, "Connected to server");
                            if (!hErr) {

                                if (this->clientType == CUBE) {
                                    LOG(INFO, "Initializing heartbeat");
                                    this->Pulse();
                                }

                                this->m_connection->ReadHeaderFromServer(
                                    [this](std::runtime_error rErr) {
                                        LOG(ERROR, "Connection Error", rErr.what());
                                        LOG(INFO, "Initiating autoconnect");
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

    void Reconnect() {
        this->m_connection->socket().close();
        this->m_connection.reset();
        this->pulse_timer.cancel();

        this->m_timer.expires_from_now(asio::chrono::seconds(5));
        this->m_timer.async_wait([this](const std::error_code& err) {
            if (!err) {
                this->AttemptConnection();
            } else {
                LOG(ERROR, "Reconnection error", err.message());
            }
        });
    }

    void Pulse() {
        if (this->IsConnected()) {
            this->pulse_timer.expires_from_now(asio::chrono::seconds(10));
            this->pulse_timer.async_wait([this](const std::error_code& err) {
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