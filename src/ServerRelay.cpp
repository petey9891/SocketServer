#include <SocketServer/common.h>
#include <SocketServer/SocketServer.h>
#include <SocketServer/SocketConnection.h>
#include "config.h"
#include <unordered_set>

class ServerRelay: public SocketServer<MessageType> {
public:
    ServerRelay(uint16_t port, std::string certPath, std::string keyPath, std::string caPath): SocketServer(port, certPath, keyPath, caPath) {};

protected:
    bool OnClientConnect(std::shared_ptr<SocketConnection<MessageType> > client) override {
        std::string ip = client->socket().remote_endpoint().address().to_string();
        if (whitelist.find(ip) != whitelist.end()) {
            return true;
        }
        return false; 
    }
    void OnMessageRecieved(std::shared_ptr<SocketConnection<MessageType> > client, Message<MessageType>& msg) override {
        switch (msg.header.id) {
            case ServerPing:
                // Simply bounce back the message
                client->Send(msg);
                break;
            case CubeDisplayOnOff:
                this->MessageAllClients(msg, client);
                break;
            case CubeBrightness:
                this->MessageAllClients(msg, client);
                break;
            case CubePulse:
                this->MessageAllClients(msg, client);
                break;
            case CubeRehoboam:
                this->MessageAllClients(msg, client);
                break;
            case ServerShutdown:
                this->MessageAllClients(msg, client);
                break;
            case Success:
                break;
        }
    }
};

int main(void) {
    ServerRelay server(port, certPath, keyPath, caPath);
    server.Start();

    while (true) {
        server.HandleRequestsNoThread();
    }

    return 0;
}