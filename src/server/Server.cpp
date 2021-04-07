#include <rehoboam-server/common.h>
#include <rehoboam-server/server.h>
#include <rehoboam-server/connection.h>

class Server: public RehoboamServer<MessageType> {
private:
    bool power = false;

public:
    Server(uint16_t port): RehoboamServer(port) {};

protected:
    bool OnClientConnect(std::shared_ptr<connection<MessageType> > client) override {
        return true;
    }

    void OnMessageRecieved(std::shared_ptr<connection<MessageType> > client, Message<MessageType>& msg) override {
        switch (msg.header.id) {
            case ServerPing:
                // Simply bounce back the message
                client->Send(msg);
                break;
            case CubeDisplayOnOff:
                if (!power) {
                    power = true;
                    printf("The power is on\n");
                } else {
                    power = false;
                    printf("The power is off\n");
                }
                this->Acknowledge(client);
                break;
            case CubeBrightness:
                uint8_t value;
                msg >> value;
                printf("Brightness: %d\n", value);
                this->Acknowledge(client);
                break;
            case CubePulse:
                printf("The cube is pulsing\n");
                this->Acknowledge(client);
                break;
            case CubeRehoboam:
                printf("The cube is rehoboaming\n");
                this->Acknowledge(client);
                break;
            case Success:
                break;
        }
    }

private:
    void Acknowledge(std::shared_ptr<connection<MessageType> > client) {
        Message<MessageType> res;
        res.header.id = Success;

        this->MessageClient(client, res);
    }
};


int main(void) {
    Server server(60000);
    server.Start();

    server.HandleRequests();

    while (true) {
        // This is just here so it doesn't exit the program immediately since it is running in different threads
    }

    return 0;
}