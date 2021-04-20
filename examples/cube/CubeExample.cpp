#include <SocketServer/common.h>
#include <SocketServer/SocketClient.h>

#include <iostream>
#include <chrono>
#include "../config.h"

class Cube: public SocketClient<MessageType> {
private:
    bool power = false;

public:
    Cube(std::string cert, std::string key, std::string ca): SocketClient(cert, key, ca) {}

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
    Cube cube(clientCertPath, clientKeyPath, caPath);

    cube.Connect(host, port);

    printf("[CUBE] Connected\n");
    while (true) {
        cube.HandleMessagesNoThread();
    }

    cube.Disconnect();

    return 0;
}