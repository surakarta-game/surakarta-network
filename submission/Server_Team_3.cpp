#include <condition_variable>
#include <filesystem>
#include "manual_record_socket_wrapper.h"
#include "network_framework.h"
#include "reverse_proxy_service.h"
#include "surakarta.h"
#include "surakarta_network.h"

class OnClosedSocketWrapper : public NetworkFramework::Socket {
   public:
    OnClosedSocketWrapper(std::shared_ptr<NetworkFramework::Socket> socket, std::function<void()> on_closed)
        : socket(socket), on_closed(on_closed) {}

    void Close() override {
        socket->Close();
        on_closed();
    }

    void Send(NetworkFramework::Message message) override { socket->Send(message); }
    std::optional<NetworkFramework::Message> Receive() override { return socket->Receive(); }
    std::string PeerAddress() const override { return socket->PeerAddress(); }
    int PeerPort() const override { return socket->PeerPort(); }

   private:
    std::shared_ptr<NetworkFramework::Socket> socket;
    std::function<void()> on_closed;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <server_port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    int inner_server_port = 30000 + GlobalRandomGenerator::getInstance()() % 10000;
    auto path = std::filesystem::current_path() / "record" / "server";
    std::filesystem::create_directory(std::filesystem::current_path() / "record");
    std::filesystem::create_directory(std::filesystem::current_path() / "record" / "server");
    auto run_log_path = path / ("Team_3_" + std::to_string(GlobalRandomGenerator::getInstance()()) + ".log");

    std::mutex mutex;
    std::unique_lock lock(mutex);
    std::condition_variable on_socket_closed;
    int closed_count = 0;

    std::shared_ptr<NetworkFramework::Server> inner_server;
    std::shared_ptr<NetworkFramework::Server> exposed_server;
    inner_server = std::make_shared<NetworkFramework::Server>(
        std::make_shared<SurakartaNetworkService>(std::make_shared<SurakartaLogger>(
            std::make_shared<SurakartaLoggerStreamMultiple>(
                std::make_shared<SurakartaLoggerStreamStdout>(),
                std::make_shared<SurakartaLoggerStreamFile>(run_log_path.c_str())))),
        inner_server_port);
    exposed_server = std::make_shared<NetworkFramework::Server>(
        std::make_shared<ReverseProxyService>(
            "localhost",
            inner_server_port,
            [&](auto socket) {
                return std::make_shared<OnClosedSocketWrapper>(
                    std::make_shared<SurakartaManualRecordSocketWrapper>(socket, path / "Team_3.txt", PieceColor::BLACK),
                    [&]() {
                        std::unique_lock lock(mutex);
                        closed_count++;
                        on_socket_closed.notify_all();
                    });
            }),
        port);

    on_socket_closed.wait(lock, [&]() { return closed_count == 2; });
    return 0;
}
