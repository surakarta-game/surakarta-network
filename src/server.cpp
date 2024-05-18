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

int main() {
    int port = 7777;
    auto logger = std::make_shared<SurakartaLoggerStdout>();
    NetworkFramework::Server server(std::make_shared<SurakartaNetworkServiceFactory>(), port);
    signal(SIGINT, onSignal);

    std::unique_lock lock(mutex);
    condition_variable.wait(lock, [&] { return !running; });

    logger->Log("Server is shutting down...");
    return 0;
}
