// This file is copied and modified fom /home/nictheboy/Documents/surakarta-network/third-party/surakarta-core/src/main.cpp

#include "surakarta.h"
#include "surakarta_network.h"

#define WIN_MIME 1
#define WIN_RANDOM 2
#define STALEMATE 3

#define ANSI_CLEAR_SCREEN "\033[2J"
#define ANSI_MOVE_TO_START "\033[H"

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

inline int play(std::string address,
                int port,
                std::string username,
                int room_number,
                PieceColor requested_color = PieceColor::NONE,
                std::shared_ptr<SurakartaLogger> logger = std::make_shared<SurakartaLoggerNull>(),
                bool visual = false,
                int depth = SurakartaMoveWeightUtil::DefaultDepth,
                double alpha = SurakartaMoveWeightUtil::DefaultAlpha,
                double beta = SurakartaMoveWeightUtil::DefaultBeta) {
    const auto move_weight_util_factory = std::make_shared<SurakartaAgentMineFactory::SurakartaMoveWeightUtilFactory>(depth, alpha, beta);
    const auto agent_factory_mine = std::make_shared<SurakartaAgentMineFactory>(move_weight_util_factory);
    const auto agent_factory_remote = std::make_shared<SurakartaAgentRemoteFactory>(
        address, port, username, room_number, requested_color, logger);
    const auto my_colour = agent_factory_remote->AssignedColor();
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
                if (visual) {
                    // std::cout << ANSI_CLEAR_SCREEN << ANSI_MOVE_TO_START;
                    std::cout << std::endl;
                    std::cout << "B: " << (my_colour == PieceColor::BLACK ? "Mine" : "Random") << std::endl;
                    std::cout << "W: " << (my_colour == PieceColor::WHITE ? "Mine" : "Random") << std::endl;
                    std::cout << std::endl;
                    std::cout << *daemon.Board() << std::endl;
                }
            }
        }
    });

    daemon.Execute();

    const bool is_stalemate = daemon.GameInfo()->Winner() == SurakartaPlayer::NONE;
    const bool has_win = !((daemon.GameInfo()->Winner() == SurakartaPlayer::BLACK) ^ (my_colour == PieceColor::BLACK));
    return is_stalemate ? STALEMATE : (has_win ? WIN_MIME : WIN_RANDOM);
}