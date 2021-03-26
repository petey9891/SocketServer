#include <rehoboam-server/common.h>
#include <rehoboam-server/server.h>


class Server: public RehoboamServer<MessageType> {
public:
    Server(uint16_t port): RehoboamServer(port) {};

protected:
    bool OnClientConnect() override {
        return true;
    }
};


int main(void) {
    Server server(60000);
    server.Start();

    while (1) {
        // server.HandleRequest();
    }

    return 0;
}