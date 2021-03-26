#include <rehoboam-server/common.h>
#include <rehoboam-server/client.h>

class Client: public RehoboamClient<MessageType> {
};

int main(void) {
    Client client;

    client.Connect("127.0.0.1", 60000);

    while (client.IsConnected()) {

    }

    return 0;
}