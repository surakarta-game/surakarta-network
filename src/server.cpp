#include <condition_variable>
#include <csignal>
#include <mutex>
#include "network_framework.h"
#include "surakarta.h"
#include "surakarta_network.h"

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
    if (argc > 1) {
        int port = std::stoi(argv[1]);
        auto logger = std::make_shared<SurakartaLoggerStdout>();
        NetworkFramework::Server server(std::make_shared<SurakartaNetworkService>(logger), port);
        signal(SIGINT, onSignal);

        std::unique_lock lock(mutex);
        condition_variable.wait(lock, [&] { return !running; });

        logger->Log("Server is shutting down...");
        return 0;
    } else {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
}
