// This file is copied and modified fom /home/nictheboy/Documents/surakarta-network/third-party/surakarta-core/src/main.cpp

#include <cstring>
#include "private-include/play.h"

int main(int argc, char** argv) {
    std::string address;
    int port;
    std::string username = "user";
    int room_number = 0;
    PieceColor requested_color = PieceColor::NONE;
    bool visual = false;
    int depth = SurakartaMoveWeightUtil::DefaultDepth;
    double alpha = SurakartaMoveWeightUtil::DefaultAlpha;
    double beta = SurakartaMoveWeightUtil::DefaultBeta;
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--username") == 0 || strcmp(argv[i], "-u") == 0) {
            username = argv[++i];
        } else if (strcmp(argv[i], "--room") == 0 || strcmp(argv[i], "-r") == 0) {
            room_number = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--color") == 0 || strcmp(argv[i], "-c") == 0) {
            auto color_str = std::string(argv[++i]);
            if (color_str == "black" || color_str == "BLACK") {
                requested_color = PieceColor::BLACK;
            } else if (color_str == "white" || color_str == "WHITE") {
                requested_color = PieceColor::WHITE;
            } else {
                requested_color = PieceColor::NONE;
            }
        } else if (strcmp(argv[i], "--depth") == 0 || strcmp(argv[i], "-d") == 0) {
            depth = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--alpha") == 0 || strcmp(argv[i], "-a") == 0) {
            alpha = atof(argv[++i]);
        } else if (strcmp(argv[i], "--beta") == 0 || strcmp(argv[i], "-b") == 0) {
            beta = atof(argv[++i]);
        } else if (strcmp(argv[i], "--visual") == 0 || strcmp(argv[i], "-v") == 0) {
            visual = true;
        }
    }
    if (argc >= 3) {
        address = argv[1];
        port = atoi(argv[2]);
        play(address, port, username, room_number, requested_color,
             std::make_shared<SurakartaLoggerStdout>(), visual, depth, alpha, beta);
    } else {
        std::cout << "Usage: " << argv[0] << " <address> <port> [args..]" << std::endl;
        std::cout << "Args:" << std::endl;
        std::cout << "  -u|--username <username>  The username to use, default: \"user\"" << std::endl;
        std::cout << "  -r|--room     <room>      The room number to join, default: 0" << std::endl;
        std::cout << "  -c|--color    <color>     The color to request, default: none" << std::endl;
        std::cout << "  -v|--visual               Enable visual mode" << std::endl;
        std::cout << "  -d|--depth    <depth>     The depth of the search tree, default: " << SurakartaMoveWeightUtil::DefaultDepth << std::endl;
        std::cout << "  -a|--alpha    <alpha>     The reduce rate for being captured, default: " << SurakartaMoveWeightUtil::DefaultAlpha << std::endl;
        std::cout << "  -b|--beta     <beta>      The reduce rate for capturing, default: " << SurakartaMoveWeightUtil::DefaultBeta << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  " << argv[0] << " 127.0.0.1 7777 -u user -r 1 -c black -d 5 -a 1.1 -b 0.9" << std::endl;
    }
    return 0;
}
