#include <SocketServer/common.h>
#include <SocketServer/SocketClient.h>

#include <iostream>
#include <chrono>
#include "../config.h"

class Client: public SocketClient<MessageType> {
private:
    bool power = false;

public:
    Client(std::string ca, std::string key): SocketClient(ca, key) {}

    void OnMessageRecieved(Message<MessageType>& msg) override {
        switch (msg.header.id) {
            case ServerPing:
                break;
            case CubeDisplayOnOff:
                if (!power) {
                    power = true;
                    printf("The power is on\n");
                } else {
                    power = false;
                    printf("The power is off\n");
                }
                break;
            case CubeBrightness:
                uint8_t value;
                msg >> value;
                printf("Brightness: %d\n", value);
                break;
            case CubePulse:
                printf("The cube is pulsing\n");
                break;
            case CubeRehoboam:
                printf("The cube is rehoboaming\n");
                break;
            case ServerShutdown:
                printf("The cube is shutting down\n");
                break;
            case Success:
                break;
        }
    }
};

int main(void) {
    Client client(caPath, keyPath);

    client.Connect(host, port);

    printf("[CLIENT] Connected\n");
    while (true) {
        client.HandleMessagesNoThread();
    }

    client.Disconnect();

    return 0;
}