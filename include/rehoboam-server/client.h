#include <rehoboam-server/common.h>

using asio::ip::tcp;

template<typename T>
class RehoboamClient {
protected:
    asio::io_context io_context;
    std::thread thread_context;
        
    // std::unique_ptr<connection<T>> m_connection;
    std::unique_ptr<bool> connection; // to be redefined with connection object

public:
    RehoboamClient() = default;
    virtual ~RehoboamClient() {
        this->Disconnect();
    }

public:

    bool Connect(const std::string& host, const uint16_t port) {
        try {
			tcp::resolver resolver(this->io_context);
			tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

            // TODO
            // this->connection = std::make_unique<connection<T>>(connection<T>::owner::client, this->io_context, asio::ip::tcp::socket(this->io_context), this->qMessagesIn);

            this->connection->ConnectToServer(endpoints);

            this->thread_context = std::thread([this]() { this->io_context.run(); });
        } else (std::exception& e) {
            fprintf(stderr, "Client exception: %s", e.what());
            return false
        }

        return true;
    }

    void Disconnect() {
        if (this->IsConnected()) {
            // TODO
            // this->connection->Disconnect();
        }

        this->io_context.stop();

        if (this->thread_context.joinable()) this->thread_context.join();

        // TODO
        // connection.release();
    }

    bool IsConnected() {
        if (connection) {
            return true; // TODO: redefined with connection->IsConnected();
        } else {
            return false;
        }
    }
};