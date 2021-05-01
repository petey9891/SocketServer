#include <SocketServer/common.h>
#include <SocketServer/SocketServer.h>
#include <SocketServer/SocketConnection.h>
#include "../config.h"
#include <unordered_set>


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

    void OnMessageRecieved(std::shared_ptr<SocketConnection<MessageType> > client, Message<MessageType>& msg) override {
        switch (msg.header.id) {
            case ServerPing:
                // Simply bounce back the message
                client->Send(msg);
                break;
            case CubeDisplayOnOff:
                // this->Acknowledge(client);
                this->MessageAllClients(msg, client);
                break;
            case CubeBrightness:
                // this->Acknowledge(client);
                this->MessageAllClients(msg, client);
                break;
            case CubePulse:
                // this->Acknowledge(client);
                this->MessageAllClients(msg, client);
                break;
            case CubeRehoboam:
                // this->Acknowledge(client);
                this->MessageAllClients(msg, client);
                break;
            case ServerShutdown:
                // this->Acknowledge(client);
                this->MessageAllClients(msg, client);
                break;
            case Success:
                break;
        }
    }

private:
    void Acknowledge(std::shared_ptr<SocketConnection<MessageType> > client) {
        Message<MessageType> res;
        res.header.id = Success;

        this->MessageClient(client, res);
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