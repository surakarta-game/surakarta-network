#include <thread>
#include "network_framework.h"
#include "private-include/message.h"
#include "private-include/play.h"
#include "private-include/socket_log_wrapper.h"

#define PORT 6666

int main() {
    auto logger = std::make_shared<SurakartaLoggerStdout>();
    auto service = std::make_shared<SurakartaNetworkService>(logger->CreateSublogger("server "));
    NetworkFramework::Server server(service, PORT);

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
    auto socket3 = std::make_shared<SurakartaNetworkSocketLogWrapper>(
        NetworkFramework::ConnectToServer("localhost", PORT),
        logger->CreateSublogger("client3"));
    socket3->Send(SurakartaNetworkMessageReady("user3", PieceColor::NONE, 0));

    // Test game started yet not ended
    auto socket4 = std::make_shared<SurakartaNetworkSocketLogWrapper>(
        NetworkFramework::ConnectToServer("localhost", PORT),
        logger->CreateSublogger("client4"));
    socket4->Send(SurakartaNetworkMessageReady("user4", PieceColor::NONE, 1));
    auto socket5 = std::make_shared<SurakartaNetworkSocketLogWrapper>(
        NetworkFramework::ConnectToServer("localhost", PORT),
        logger->CreateSublogger("client5"));
    socket5->Send(SurakartaNetworkMessageReady("user5", PieceColor::NONE, 1));

    // Test implicit resign
    auto socket6 = std::make_shared<SurakartaNetworkSocketLogWrapper>(
        NetworkFramework::ConnectToServer("localhost", PORT),
        logger->CreateSublogger("client6"));
    socket6->Send(SurakartaNetworkMessageReady("user6", PieceColor::NONE, 2));
    auto socket7 = std::make_shared<SurakartaNetworkSocketLogWrapper>(
        NetworkFramework::ConnectToServer("localhost", PORT),
        logger->CreateSublogger("client7"));
    socket7->Send(SurakartaNetworkMessageReady("user7", PieceColor::NONE, 2));
    socket6->Close();

    std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait for server to process the message
    service->ShutdownService();
    server.Shutdown();

    return 0;
}
