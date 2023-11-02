#include <Rehoboam/SocketLibrary/common.h>
#include <Rehoboam/SocketLibrary/SocketClient.h>

#include <iostream>
#include <chrono>
#include "../config.h"

using namespace Rehoboam::SocketLibrary;

class Cube: public SocketClient<MessageType> {
private:
    bool power = false;

public:
    Cube(const std::string& host, const uint16_t port, std::string cert, std::string key, std::string ca)
        : SocketClient(host, port, cert, key, ca, CUBE)
    {}

    void OnMessageReceived(Message<MessageType>& msg) override {
        if (msg.header.id == MessageType::Success) return;

        if (msg.header.id == MessageType::ServerPing) {
            printf("Heartbeat has been acknowledge\n");
        } else if (msg.header.id == MessageType::CubeDisplayToggle) {
            if (!power) {
                power = true;
                printf("The power is on\n");
            } else {
                power = false;
                printf("The power is off\n");
            }
        } else if (msg.header.id == MessageType::CubeBrightness) {
            uint8_t value;
            msg >> value;
            printf("Brightness: %d\n", value);
        } else if (msg.header.id == MessageType::CubeDisplayChange) {
            uint8_t value;
            msg >> value;
            printf("New Display: %d\n", value);
        }
    }
};

int main(void) {
    Cube cube(host, port, clientCertPath, clientKeyPath, caPath);

    cube.Connect();

    printf("[CUBE] Connected\n");
    while (true) {
        cube.HandleMessagesNoThread();
    }

    cube.Disconnect();

    return 0;
}