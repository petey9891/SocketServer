#include <rehoboam-server/common.h>
#include <rehoboam-server/client.h>

class Client: public RehoboamClient<MessageType> {
public:
    void PingServer() {
        Message<MessageType> message;
        message.header.id = ServerPing;

        this->Send(message);
    }

};

int main(void) {
    Client client;

    client.Connect("127.0.0.1", 60000);

    bool first = false;
    while (client.IsConnected()) {
        if (!first) {
            client.PingServer();
            first = true;
        }
    }

    return 0;
}