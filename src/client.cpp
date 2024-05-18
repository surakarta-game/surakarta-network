// This file is copied and modified fom /home/nictheboy/Documents/surakarta-network/third-party/surakarta-core/src/main.cpp

#include "surakarta.h"
#include "surakarta_network.h"

#include <climits>
#include <cstring>
#include <thread>

#define ANSI_CLEAR_SCREEN "\033[2J"
#define ANSI_MOVE_TO_START "\033[H"

#define WIN_MIME 1
#define WIN_RANDOM 2
#define STALEMATE 3

class ExposiveSurakartaDaemon : public SurakartaDaemon {
   public:
    ExposiveSurakartaDaemon(
        int board_size,
        int max_no_capture_round,
        std::shared_ptr<AgentFactory> black_agent_factory,
        std::shared_ptr<AgentFactory> white_agent_factory)
        : SurakartaDaemon(board_size, max_no_capture_round, black_agent_factory, white_agent_factory) {}

    std::shared_ptr<SurakartaGameInfo> GameInfo() {
        return game_.GetGameInfo();
    }

    std::shared_ptr<SurakartaBoard> Board() {
        return game_.GetBoard();
    }
};

int play(std::string address,
         int port,
         std::string username,
         int room_number,
         PieceColor requested_color = PieceColor::NONE,
         int depth = SurakartaMoveWeightUtil::DefaultDepth,
         double alpha = SurakartaMoveWeightUtil::DefaultAlpha,
         double beta = SurakartaMoveWeightUtil::DefaultBeta) {
    const auto move_weight_util_factory = std::make_shared<SurakartaAgentMineFactory::SurakartaMoveWeightUtilFactory>(depth, alpha, beta);
    const auto agent_factory_mine = std::make_shared<SurakartaAgentMineFactory>(move_weight_util_factory);
    const auto agent_factory_remote = std::make_shared<SurakartaAgentRemoteFactory>(address, port, username, room_number, requested_color);
    const auto my_colour = GlobalRandomGenerator().getInstance()() % 2 ? PieceColor::BLACK : PieceColor::WHITE;
    const auto agent_factory_black = my_colour == PieceColor::BLACK ? (std::shared_ptr<SurakartaDaemon::AgentFactory>)agent_factory_mine : agent_factory_remote;
    const auto agent_factory_white = my_colour == PieceColor::WHITE ? (std::shared_ptr<SurakartaDaemon::AgentFactory>)agent_factory_mine : agent_factory_remote;
    auto daemon = ExposiveSurakartaDaemon(BOARD_SIZE, MAX_NO_CAPTURE_ROUND, agent_factory_black, agent_factory_white);

    const auto black_pieces = std::make_shared<std::vector<SurakartaPositionWithId>>();
    const auto white_pieces = std::make_shared<std::vector<SurakartaPositionWithId>>();
    bool piece_lists_initialized = false;
    SurakartaOnBoardUpdateUtil on_board_update_util(black_pieces, white_pieces, daemon.Board());

    daemon.OnUpdateBoard.AddListener([&]() {
        if (!piece_lists_initialized) {
            const auto lists = SurakartaInitPositionListsUtil(daemon.Board()).InitPositionList();
            *black_pieces = *lists.black_list;
            *white_pieces = *lists.white_list;
            piece_lists_initialized = true;
        }
        const auto opt_trace = on_board_update_util.UpdateAndGetTrace();
        if (opt_trace.has_value()) {
            PieceColor moved_colour = PieceColor::NONE;
            for (auto& item : *black_pieces) {
                if (item.id == opt_trace->moved_piece.id)
                    moved_colour = PieceColor::BLACK;
            }
            for (auto& item : *white_pieces) {
                if (item.id == opt_trace->moved_piece.id)
                    moved_colour = PieceColor::WHITE;
            }
            if (moved_colour == PieceColor::NONE)
                throw std::runtime_error("moved piece not found in black_pieces or white_pieces");
            const auto guard = opt_trace.value().is_capture ? SurakartaTemporarilyChangeColorGuardUtil(
                                                                  daemon.Board(),
                                                                  SurakartaPosition(
                                                                      opt_trace.value().captured_piece.x,
                                                                      opt_trace.value().captured_piece.y),
                                                                  ReverseColor(moved_colour))
                                                            : SurakartaTemporarilyChangeColorGuardUtil();
            for (auto& fragment : opt_trace.value().path) {
                SurakartaTemporarilyChangeColorGuardUtil guard1(daemon.Board(), fragment.From(), PieceColor::NONE);
                SurakartaTemporarilyChangeColorGuardUtil guard2(daemon.Board(), fragment.To(), moved_colour);
                std::cout << ANSI_CLEAR_SCREEN << ANSI_MOVE_TO_START;
                std::cout << "B: " << (my_colour == PieceColor::BLACK ? "Mine" : "Random") << std::endl;
                std::cout << "W: " << (my_colour == PieceColor::WHITE ? "Mine" : "Random") << std::endl;
                std::cout << std::endl;
                std::cout << *daemon.Board() << std::endl;
            }
        }
    });

    daemon.Execute();

    const bool is_stalemate = daemon.GameInfo()->Winner() == SurakartaPlayer::NONE;
    const bool has_win = !((daemon.GameInfo()->Winner() == SurakartaPlayer::BLACK) ^ (my_colour == PieceColor::BLACK));
    return is_stalemate ? STALEMATE : (has_win ? WIN_MIME : WIN_RANDOM);
}

int main(int argc, char** argv) {
    std::string address;
    int port;
    std::string username = "user";
    int room_number = 0;
    PieceColor requested_color = PieceColor::NONE;
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
        }
    }
    if (argc >= 3) {
        address = argv[1];
        port = atoi(argv[2]);
        play(address, port, username, room_number, requested_color, depth, alpha, beta);
    } else {
        std::cout << "Usage: " << argv[0] << " <address> <port> [args..]" << std::endl;
        std::cout << "Args:" << std::endl;
        std::cout << "  -u|--username <username>  The username to use, default: \"user\"" << std::endl;
        std::cout << "  -r|--room     <room>      The room number to join, default: 0" << std::endl;
        std::cout << "  -c|--color    <color>     The color to request, default: none" << std::endl;
        std::cout << "  -d|--depth    <depth>     The depth of the search tree, default: " << SurakartaMoveWeightUtil::DefaultDepth << std::endl;
        std::cout << "  -a|--alpha    <alpha>     The reduce rate for being captured, default: " << SurakartaMoveWeightUtil::DefaultAlpha << std::endl;
        std::cout << "  -b|--beta     <beta>      The reduce rate for capturing, default: " << SurakartaMoveWeightUtil::DefaultBeta << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  " << argv[0] << " 127.0.0.1 7777 -u user -r 1 -c black -d 5 -a 1.1 -b 0.9" << std::endl;
    }
    return 0;
}
