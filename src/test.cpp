#include <thread>
#include "network_framework.h"
#include "private-include/message.h"
#include "private-include/play.h"
#include "private-include/socket_log_wrapper.h"

#define PORT 6666

int main() {
    auto logger = std::make_shared<SurakartaLoggerStdout>();
    NetworkFramework::Server server(std::make_unique<SurakartaNetworkService>(logger->CreateSublogger("server ")), PORT);

    // Test normal game
    auto client_thread_1 = std::thread([logger]() {
        play("127.0.0.1", PORT, "user1", 0, PieceColor::NONE, logger->CreateSublogger("client1"));
    });
    auto client_thread_2 = std::thread([logger]() {
        play("127.0.0.1", PORT, "user2", 0, PieceColor::NONE, logger->CreateSublogger("client2"));
    });
    client_thread_1.join();
    client_thread_2.join();

    // Test game not started
    auto socket = std::make_shared<SurakartaNetworkSocketLogWrapper>(
        NetworkFramework::ConnectToServer("localhost", PORT),
        logger->CreateSublogger("client3"));
    socket->Send(SurakartaNetworkMessageReady("user3", PieceColor::NONE, 0));
    std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait for server to process the message
    server.Shutdown();

    return 0;
}
