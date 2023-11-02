#include <Rehoboam/SocketLibrary/common.h>
#include <Rehoboam/SocketLibrary/SocketServer.h>
#include <Rehoboam/SocketLibrary/SocketConnection.h>

#include "../config.h"
#include <unordered_set>

using namespace Rehoboam::SocketLibrary;

class Server: public SocketServer<MessageType> {
private:
    std::unordered_set<std::string> whitelist = { "127.0.0.1" };

public:
    Server(uint16_t port, std::string certPath, std::string keyPath, std::string caPath): SocketServer(port, certPath, keyPath, caPath) {};

protected:
    bool OnClientConnect(std::shared_ptr<SocketConnection<MessageType> > client) override {
        std::string ip = client->socket().remote_endpoint().address().to_string();
        if (this->whitelist.find(ip) != this->whitelist.end()) {
            return true;
        }
        return false;
    }

    void OnMessageReceived(std::shared_ptr<SocketConnection<MessageType>> client, Message<MessageType>& msg) override {
        if (msg.header.id == MessageType::Success) return;

        if (msg.header.id == MessageType::ServerPing) {
            client->Send(msg);
        } else {
            this->MessageAllClients(msg, client);
        }
    }
};

int main(void) {
    Server server(port, certPath, keyPath, caPath);
    server.Start();

    while (true) {
        server.HandleRequestsNoThread();
    }

    return 0;
}