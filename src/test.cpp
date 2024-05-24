#include <thread>
#include "network_framework.h"
#include "private-include/play.h"

#define PORT 6666

int main() {
    auto logger = std::make_shared<SurakartaLoggerStdout>();
    NetworkFramework::Server server(std::make_unique<SurakartaNetworkService>(logger->CreateSublogger("server ")), PORT);
    auto client_thread_1 = std::thread([logger]() {
        play("127.0.0.1", PORT, "user1", 0, PieceColor::NONE, logger->CreateSublogger("client1"));
    });
    auto client_thread_2 = std::thread([logger]() {
        play("127.0.0.1", PORT, "user2", 0, PieceColor::NONE, logger->CreateSublogger("client2"));
    });
    client_thread_1.join();
    client_thread_2.join();
    return 0;
}
