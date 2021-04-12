#include <rehoboam-server/common.h>
#include <rehoboam-server/client.h>

#include <iostream>
#include <chrono>

class Client: public SocketClient<MessageType> {
public:
    Client(std::string ca, std::string key): SocketClient(ca, key) {}

    void OnOff() {
        Message<MessageType> message;
        message.header.id = CubeDisplayOnOff;
        this->Send(message);
    }

    void Brightness(uint8_t brightness) {
        Message<MessageType> message;
        message.header.id = CubeBrightness;

        message << brightness;
        this->Send(message);
    }

    void Pulse() {
        Message<MessageType> message;
        message.header.id = CubePulse;
        this->Send(message);
    }

    void Rehoboam() {
        Message<MessageType> message;
        message.header.id = CubeRehoboam;
        this->Send(message);
    }

    void Ping() {
        Message<MessageType> message;
        message.header.id = ServerPing;

		// Caution with this...
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();		

		message << timeNow;

        this->Send(message);
    }

    void Shutdown() {
        Message<MessageType> message;
        message.header.id = ServerShutdown;
        this->Send(message);
    }
};

int main(void) {
    std::string ca = "server.pem";
    std::string key = "server-key.pem";
    Client client(ca, key);

    client.Connect("127.0.0.1", 60000);

    printf("Commands:\n\n1)\tOn/Off\n2)\tBrightness <value>\n3)\tPulse\n4)\tRehoboam\n5)\tPing\n6)\tShutdown\n");

    uint8_t input;

    bool waitingForAck = false;
    while (client.IsConnected()) {
        printf("\n>>> ");
        std::cin >> input;

        if (input == '1') {
            client.OnOff();
            waitingForAck = true;
        } else if (input == '2') {
            int brightness;
            printf(">>> Input a brightness level: ");
            std::cin >> brightness;

            if (brightness < 1 || brightness > 100) {
                printf("Brightness must be within 1-100\n");
            } else  {
                client.Brightness(brightness);
                waitingForAck = true;
            }
        } else if (input == '3') {
            client.Pulse();
            waitingForAck = true;
        } else if (input == '4') {
            client.Rehoboam();
            waitingForAck = true;
        } else if (input == '5') {
            client.Ping();
            waitingForAck = true;
        } else if (input == '6') {
            char confirm;
            printf(">>> Are you sure you want to shutdown? (y/n): ");
            std::cin >> confirm;

            if (confirm == 'y') {
                client.Shutdown();
                waitingForAck = true;
            }
        }

        while (waitingForAck) {  
            if (!client.IncomingMessages().empty()) {
                Message<MessageType> msg = client.IncomingMessages().pop_front().message;

                if (msg.header.id == Success) {
                    printf("request is acknowleged\n");
                    waitingForAck = false;
                } else if (msg.header.id == ServerPing) {
                    // Server has responded to a ping request
                    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point timeThen;
                    msg >> timeThen;
                    std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
                    waitingForAck = false;
                }
            }
        }
    }

    client.Disconnect();

    return 0;
}