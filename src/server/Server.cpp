#include <rehoboam-server/common.h>
#include <rehoboam-server/server.h>
#include <rehoboam-server/connection.h>


class Server: public RehoboamServer<MessageType> {
public:
    Server(uint16_t port): RehoboamServer(port) {};

protected:
    bool OnClientConnect(std::shared_ptr<connection<MessageType> > client) override {
        return true;
    }

    void OnMessageRecieved(std::shared_ptr<connection<MessageType> > client, Message<MessageType>& msg) override {
        switch (msg.header.id) {
            case ServerPing:
                printf("We got pinged, batman!\n");
                break;
        }
    }
};


int main(void) {
    Server server(60000);
    server.Start();

    while (1) {
        server.HandleRequest();
    }

    return 0;
}