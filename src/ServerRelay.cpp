#include <SocketServer/common.h>
#include <SocketServer/SocketServer.h>
#include <SocketServer/SocketConnection.h>
#include "config.h"

class ServerRelay: public SocketServer<MessageType> {
public:
    ServerRelay(uint16_t port, std::string caPath, std::string keyPath): SocketServer(port, caPath, keyPath) {};

protected:
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
    // ServerRelay server(port, caPath, keyPath);
    ServerRelay server(port, caPath, keyPath);
    server.Start();

    while (true) {
        server.HandleRequestsNoThread();
    }

    return 0;
}