#include <thread>
#include "network_framework.h"
#include "private-include/play.h"

int main() {
    constexpr int port = 6666;
    NetworkFramework::Server server(std::make_shared<SurakartaNetworkServiceFactory>(), port);
    auto client_thread_1 = std::thread([]() {
        play("127.0.0.1", port, "user1", 0, PieceColor::NONE,
             std::make_unique<SurakartaLogger>(
                 std::make_unique<SurakartaLoggerStreamWithPrefix>(
                     std::make_unique<SurakartaLoggerStreamStdout>(), "[1] ")));
    });
    auto client_thread_2 = std::thread([]() {
        play("127.0.0.1", port, "user2", 0, PieceColor::NONE,
             std::make_unique<SurakartaLogger>(
                 std::make_unique<SurakartaLoggerStreamWithPrefix>(
                     std::make_unique<SurakartaLoggerStreamStdout>(), "[2] ")));
    });
    client_thread_1.join();
    client_thread_2.join();
    return 0;
}
