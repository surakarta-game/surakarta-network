#include "surakarta_agent_remote.h"
#include "exception.h"
#include "message.h"
#include "network_framework.h"
#include "socket_log_wrapper.h"

class SurakartaAgentRemoteFactoryImpl;

class SurakartaAgentRemoteImpl : public SurakartaAgentBase {
   public:
    SurakartaAgentRemoteImpl(std::shared_ptr<SurakartaBoard> board,
                             std::shared_ptr<SurakartaGameInfo> game_info,
                             std::shared_ptr<SurakartaRuleManager> rule_manager,
                             PieceColor my_color,
                             std::shared_ptr<NetworkFramework::Socket> socket);

    ~SurakartaAgentRemoteImpl();

    SurakartaMove CalculateMove() override {
        if (remote_game_ended_) {
            printf(
                "Warning: SurakartaAgentRemoteImpl::CalculateMove() is called"
                " after the game has ended\n");
            return SurakartaMove(SurakartaPosition(0, 0), SurakartaPosition(0, 0), my_color);
        }
        auto trace = on_board_update_util_.UpdateAndGetTrace();
        if (trace.has_value() == false) {
            // first move
            if (my_color != PieceColor::BLACK) {
                printf(
                    "Warning: SurakartaAgentRemoteImpl::CalculateMove() is called"
                    " when no move has been made and my_color != PieceColor::BLACK\n");
            }
        } else {
            auto from = trace->path[0].From();
            auto to = trace->path[trace->path.size() - 1].To();
            auto message_send = SurakartaNetworkMessageMove(from, to);
            socket_->Send(message_send);
        }
    receive:
        auto response_optional = socket_->Receive();
        if (response_optional.has_value()) {
            auto response = response_optional.value();
            if (response.opcode == OPCODE::MOVE_OP) {
                auto decoded = SurakartaNetworkMessageMove(response);
                auto move = SurakartaMove(decoded.From(), decoded.To(), my_color);
                auto guard = SurakartaTemporarilyApplyMoveGuardUtil(board_, move);
                on_board_update_util_.UpdateAndGetTrace();
                return move;
            } else if (response.opcode == OPCODE::END_OP) {
                auto decoded = SurakartaNetworkMessageEnd(response);
                OnRemoteGameEnded.Invoke(decoded.IllegalMoveReason(), decoded.EndReason(), my_color);
                remote_game_ended_ = true;
                return SurakartaMove(SurakartaPosition(0, 0), SurakartaPosition(0, 0), my_color);
            } else if (response.opcode == OPCODE::CHAT_OP) {
                auto decoded = SurakartaNetworkMessageChat(response);
                OnChatMessageArrived.Invoke(decoded.Username(), decoded.ChatMessage());
                goto receive;
            } else {
                throw SurakartaNetworkUnexpectedMessageException(response);
            }
        } else {
            throw SurakartaNetworkUnexpectedEofException();
        }
    }

    SurakartaEvent<std::optional<SurakartaIllegalMoveReason>, SurakartaEndReason, PieceColor> OnRemoteGameEnded;
    SurakartaEvent<std::string, std::string> OnChatMessageArrived;

   private:
    bool remote_game_ended_ = false;
    PieceColor my_color;
    std::shared_ptr<NetworkFramework::Socket> socket_;
    SurakartaInitPositionListsUtil::PositionLists position_lists_;
    SurakartaOnBoardUpdateUtil on_board_update_util_;

    friend class SurakartaAgentRemoteFactoryImpl;
    std::mutex mutex_;
    SurakartaAgentRemoteFactoryImpl* factory_;
};

class SurakartaAgentRemoteFactoryImpl : public SurakartaDaemon::AgentFactory {
   public:
    SurakartaAgentRemoteFactoryImpl(
        std::string address,
        int port,
        std::string username,
        int room_id,
        PieceColor requested_color,
        std::shared_ptr<SurakartaLogger> logger) {
        auto raw_socket = NetworkFramework::ConnectToServer(address, port);
        socket_ = std::make_shared<SurakartaNetworkSocketLogWrapper>(std::move(raw_socket), logger);
        socket_->Send(SurakartaNetworkMessageReady(username, requested_color, room_id));
        auto response_optional = socket_->Receive();
        if (response_optional.has_value()) {
            auto response = response_optional.value();
            if (response.opcode == OPCODE::READY_OP) {
                assigned_color_ = SurakartaNetworkMessageReady(response).Color();
            } else if (response.opcode == OPCODE::REJECT_OP) {
                auto decoded = SurakartaNetworkMessageReject(response);
                throw SurakartaNetworkRejectedException(decoded.Username(), decoded.Reason());
            } else {
                throw SurakartaNetworkUnexpectedMessageException(response);
            }
        } else {
            throw SurakartaNetworkUnexpectedEofException();
        }
    }

    ~SurakartaAgentRemoteFactoryImpl();

    std::unique_ptr<SurakartaAgentBase> CreateAgent(
        std::shared_ptr<SurakartaGameInfo> game_info,
        std::shared_ptr<SurakartaBoard> board,
        std::shared_ptr<SurakartaRuleManager> rule_manager,
        SurakartaDaemon& daemon,
        PieceColor my_color) override {
        if (assigned_color_ != ReverseColor(my_color)) {
            throw SurakartaNetworkAgentColorMismatchException(assigned_color_, ReverseColor(my_color));
        }
        auto agent = std::make_unique<SurakartaAgentRemoteImpl>(
            board, game_info, rule_manager, my_color, socket_);
        agent->factory_ = this;
        agent_ = agent.get();
        agent->OnChatMessageArrived.AddListener([this](std::string username, std::string chat_message) {
            OnChatMessageArrived.Invoke(username, chat_message);
        });
        agent->OnRemoteGameEnded.AddListener([this](std::optional<SurakartaIllegalMoveReason> illegal_move_reason,
                                                    SurakartaEndReason end_reason,
                                                    PieceColor winner) {
            OnRemoteGameEnded.Invoke(illegal_move_reason, end_reason, winner);
        });
        daemon.OnGameEnded.AddListener([this](auto) {
            std::lock_guard lock(mutex_);
            if (agent_ && !agent_->remote_game_ended_) {
                auto trace = agent_->on_board_update_util_.UpdateAndGetTrace();
                if (trace.has_value() == false) {
                    // do nothing
                } else {
                    auto from = trace->path[0].From();
                    auto to = trace->path[trace->path.size() - 1].To();
                    auto message_send = SurakartaNetworkMessageMove(from, to);
                    socket_->Send(message_send);
                }
                auto response_opt = socket_->Receive();
                if (response_opt.has_value()) {
                    if (response_opt.value().opcode == OPCODE::END_OP) {
                        auto decoded = SurakartaNetworkMessageEnd(response_opt.value());
                        OnRemoteGameEnded.Invoke(decoded.IllegalMoveReason(), decoded.EndReason(), assigned_color_);
                    } else {
                        throw SurakartaNetworkUnexpectedMessageException(response_opt.value());
                    }
                } else {
                    throw SurakartaNetworkUnexpectedEofException();
                }
            }
        });
        return agent;
    }

    PieceColor AssignedColor() const {
        return assigned_color_;
    }

    SurakartaEvent<std::optional<SurakartaIllegalMoveReason>, SurakartaEndReason, PieceColor> OnRemoteGameEnded;
    SurakartaEvent<std::string, std::string> OnChatMessageArrived;

   private:
    PieceColor assigned_color_;
    std::shared_ptr<NetworkFramework::Socket> socket_;

    friend class SurakartaAgentRemoteImpl;
    std::mutex mutex_;
    SurakartaAgentRemoteImpl* agent_;
};

SurakartaAgentRemoteImpl::SurakartaAgentRemoteImpl(
    std::shared_ptr<SurakartaBoard> board,
    std::shared_ptr<SurakartaGameInfo> game_info,
    std::shared_ptr<SurakartaRuleManager> rule_manager,
    PieceColor my_color,
    std::shared_ptr<NetworkFramework::Socket> socket)
    : SurakartaAgentBase(board, game_info, rule_manager),
      my_color(my_color),
      socket_(socket),
      position_lists_(SurakartaInitPositionListsUtil(board).InitPositionList()),
      on_board_update_util_(position_lists_.black_list, position_lists_.white_list, board) {}

SurakartaAgentRemoteImpl::~SurakartaAgentRemoteImpl() {
    std::lock_guard lock(mutex_);
    if (factory_ != nullptr) {
        std::lock_guard lock(factory_->mutex_);
        factory_->agent_ = nullptr;
    }
}

SurakartaAgentRemoteFactoryImpl::~SurakartaAgentRemoteFactoryImpl() {
    std::lock_guard lock(mutex_);
    if (agent_ != nullptr) {
        std::lock_guard lock(agent_->mutex_);
        agent_->factory_ = nullptr;
    }
}

SurakartaAgentRemoteFactory::SurakartaAgentRemoteFactory(
    std::string address,
    int port,
    std::string username,
    int room_id,
    PieceColor requested_color,
    std::shared_ptr<SurakartaLogger> logger)
    : impl_(std::make_unique<SurakartaAgentRemoteFactoryImpl>(
          address,
          port,
          username,
          room_id,
          requested_color,
          logger)) {
    impl_->OnRemoteGameEnded.AddListener([this](std::optional<SurakartaIllegalMoveReason> illegal_move_reason,
                                                SurakartaEndReason end_reason,
                                                PieceColor winner) {
        OnRemoteGameEnded.Invoke(illegal_move_reason, end_reason, winner);
    });
    impl_->OnChatMessageArrived.AddListener([this](std::string username, std::string chat_message) {
        OnChatMessageArrived.Invoke(username, chat_message);
    });
}

PieceColor SurakartaAgentRemoteFactory::AssignedColor() const {
    return impl_->AssignedColor();
}

std::unique_ptr<SurakartaAgentBase> SurakartaAgentRemoteFactory::CreateAgent(
    std::shared_ptr<SurakartaGameInfo> game_info,
    std::shared_ptr<SurakartaBoard> board,
    std::shared_ptr<SurakartaRuleManager> rule_manager,
    SurakartaDaemon& daemon,
    PieceColor my_color) {
    return impl_->CreateAgent(game_info, board, rule_manager, daemon, my_color);
}
