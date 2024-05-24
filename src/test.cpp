#include <thread>
#include "network_framework.h"
#include "private-include/play.h"

int main() {
    constexpr int port = 6666;
    auto logger = std::make_shared<SurakartaLoggerStdout>();
    NetworkFramework::Server server(std::make_unique<SurakartaNetworkService>(logger->CreateSublogger("server ")), port);
    auto client_thread_1 = std::thread([logger]() {
        play("127.0.0.1", port, "user1", 0, PieceColor::NONE, logger->CreateSublogger("client1"));
    });
    auto client_thread_2 = std::thread([logger]() {
        play("127.0.0.1", port, "user2", 0, PieceColor::NONE, logger->CreateSublogger("client2"));
    });
    client_thread_1.join();
    client_thread_2.join();
    return 0;
}
