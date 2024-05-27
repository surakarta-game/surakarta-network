#include <condition_variable>
#include <csignal>
#include <mutex>
#include "network_framework.h"
#include "private-include/reverse_proxy_service.h"
#include "private-include/socket_log_wrapper.h"
#include "private-include/socket_raw_log_wrapper.h"
#include "surakarta.h"

bool running = true;
std::mutex mutex;
std::condition_variable condition_variable;

void onSignal(int signal) {
    if (signal == SIGINT) {
        std::unique_lock<std::mutex> lock(mutex);
        running = false;
        condition_variable.notify_all();
    }
}

int main(int argc, char** argv) {
    if (argc > 3) {
        int port = std::stoi(argv[1]);
        std::string server_address = argv[2];
        int server_port = std::stoi(argv[3]);
        auto logger = std::make_shared<SurakartaLoggerStdout>();
        auto service = std::make_shared<ReverseProxyService>(server_address, server_port, [&](auto socket) {
            auto prefixed_logger = logger->CreateSublogger(socket->PeerAddress() + ":" + std::to_string(socket->PeerPort()));
            return std::make_shared<SurakartaNetworkSocketLogWrapper>(
                std::make_shared<SurakartaNetworkSocketRawLogWrapper>(socket, prefixed_logger), prefixed_logger);
        });
        NetworkFramework::Server server(service, port);
        signal(SIGINT, onSignal);

        std::unique_lock lock(mutex);
        condition_variable.wait(lock, [&] { return !running; });

        logger->Log("Server is shutting down...");
        server.Shutdown();
        return 0;
    } else {
        printf("Usage: %s <port> <server_address> <server_port>\n", argv[0]);
        return 1;
    }
}
