#include <filesystem>
#include "../src/private-include/play.h"
#include "manual_record_socket_wrapper.h"
#include "network_framework.h"
#include "reverse_proxy_service.h"
#include "socket_raw_log_wrapper.h"
#include "surakarta.h"
#include "surakarta_network.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <server_address> <server_port> <color>" << std::endl;
        return 1;
    }

    std::string server_address = argv[1];
    int server_port = std::stoi(argv[2]);
    auto color_str = std::string(argv[3]);
    auto color = (color_str == "BLACK" || color_str == "black")
                     ? PieceColor::BLACK
                 : (color_str == "WHITE" || color_str == "white")
                     ? PieceColor::WHITE
                     : PieceColor::NONE;

    int reverse_proxy_port = 30000 + GlobalRandomGenerator::getInstance()() % 10000;
    auto path = std::filesystem::current_path() / "record" / "client";
    std::filesystem::create_directory(std::filesystem::current_path() / "record");
    std::filesystem::create_directory(std::filesystem::current_path() / "record" / "client");
    auto run_log_path = (path / ("Team_3_" + std::to_string(GlobalRandomGenerator::getInstance()()) + ".log")).string();
    auto logger = std::make_shared<SurakartaLogger>(
        std::make_shared<SurakartaLoggerStreamMultiple>(
            std::make_shared<SurakartaLoggerStreamStdout>(),
            std::make_shared<SurakartaLoggerStreamFile>(run_log_path.c_str())));
    auto reverse_proxy_server = NetworkFramework::Server(
        std::make_shared<ReverseProxyService>(
            server_address,
            server_port,
            [path](auto socket) {
                return std::make_shared<SurakartaNetworkSocketRawLogWrapper>(
                    std::make_shared<SurakartaManualRecordSocketWrapper>(socket, (path / "Team_3.txt").string(), PieceColor::BLACK),
                    logger);
            }),
        reverse_proxy_port);
    play(
        "localhost",
        reverse_proxy_port,
        "client",
        1,
        color,
        logger,
        false,
        3);
    return 0;
}
