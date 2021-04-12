#include <SocketServer/common.h>
#include <SocketServer/SocketServer.h>
#include <SocketServer/SocketConnection.h>
#include "../config.h"

class Server: public SocketServer<MessageType> {
public:
    Server(uint16_t port, std::string caPath, std::string keyPath): SocketServer(port, caPath, keyPath) {};

protected:
    bool OnClientConnect(std::shared_ptr<SocketConnection<MessageType> > client) override {
        return true;
    }

    void OnMessageRecieved(std::shared_ptr<SocketConnection<MessageType> > client, Message<MessageType>& msg) override {
        printf("[SERVER] Handling message %d\n", msg.header.id);
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
    Server server(port, caPath, keyPath);
    server.Start();

    while (true) {
        server.HandleRequestsNoThread();
    }

    return 0;
}