#include "surakarta_network_service.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include "exception_as_eof_wrapper.h"
#include "message.h"
#include "opcode.h"
#include "socket_log_wrapper.h"
#include "surakarta.h"

class SurakartaNetworkServiceImpl : public NetworkFramework::Service {
   public:
    SurakartaNetworkServiceImpl(std::shared_ptr<SurakartaLogger> logger)
        : logger_(logger) {}

    enum class RoomStatus {
        EMPTY,
        WAITING_SECOND_PLAYER,
        PLAYING,
        ENDED,
        CLOSED,
        REMOVED,
    };

    struct Room {
        const int id;  // This field can be access without lock, since it is only written once
        mutable std::mutex mutex;
        std::condition_variable when_game_started_or_initialization_failed;
        RoomStatus status = RoomStatus::EMPTY;
        const std::shared_ptr<NetworkFramework::Socket> first_player_socket;
        std::shared_ptr<NetworkFramework::Socket> second_player_socket;
        const SurakartaNetworkMessageReady first_player_message;
        std::shared_ptr<SurakartaDaemon> daemon;
        std::shared_ptr<std::thread> daemon_thread;
        std::shared_ptr<SurakartaAgentInteractiveHandler> first_player_handler;
        std::shared_ptr<SurakartaAgentInteractiveHandler> second_player_handler;
        PieceColor first_player_color, second_player_color;
        std::string first_player_username, second_player_username;

        Room(int id,
             std::shared_ptr<NetworkFramework::Socket> first_player_socket,
             SurakartaNetworkMessageReady first_player_message)
            : id(id), first_player_socket(first_player_socket), first_player_message(first_player_message) {}

        RoomStatus Status() const {
            std::lock_guard lock(mutex);
            return status;
        }

        // returns:
        // 0: is first, success
        // 1: is first, failed
        // 2: is second
        int WaitingIfIsFirst(std::shared_ptr<SurakartaLogger> logger) {
            std::unique_lock lock(mutex);
            if (status == RoomStatus::WAITING_SECOND_PLAYER)
                return 2;
            logger->Log("Room created.");
            status = RoomStatus::WAITING_SECOND_PLAYER;
            // now nothing to do, just wait
            when_game_started_or_initialization_failed.wait(lock, [this] {
                return status != RoomStatus::WAITING_SECOND_PLAYER;
            });
            return status != RoomStatus::PLAYING;
        }

        void StartFailed() {
            std::lock_guard lock(mutex);
            status = RoomStatus::EMPTY;
            when_game_started_or_initialization_failed.notify_all();
        }

        void StartRoom(
            std::shared_ptr<NetworkFramework::Socket> _second_player_socket,
            PieceColor _first_player_color,
            PieceColor _second_player_color,
            std::string _first_player_username,
            std::string _second_player_username,
            std::shared_ptr<SurakartaAgentInteractiveHandler> _first_player_handler,
            std::shared_ptr<SurakartaAgentInteractiveHandler> _second_player_handler,
            std::shared_ptr<SurakartaDaemon> _daemon,
            std::shared_ptr<std::thread> _daemon_thread) {
            std::lock_guard lock(mutex);
            second_player_socket = _second_player_socket;
            first_player_color = _first_player_color;
            second_player_color = _second_player_color;
            first_player_username = _first_player_username;
            second_player_username = _second_player_username;
            first_player_handler = _first_player_handler;
            second_player_handler = _second_player_handler;
            daemon = _daemon;
            daemon_thread = _daemon_thread;
            status = RoomStatus::PLAYING;
            when_game_started_or_initialization_failed.notify_all();
        }
    };

    mutable std::mutex mutex;
    std::vector<std::shared_ptr<Room>> rooms;

    std::shared_ptr<Room> GetOrCreateRoom(
        SurakartaNetworkMessageReady message,
        std::shared_ptr<NetworkFramework::Socket> socket_of_first_player) {
        std::lock_guard<std::mutex> lock(mutex);
        for (int i = 0; i < (int)rooms.size(); i++) {
            if (rooms[i]->id == message.RoomId()) {
                return rooms[i];
            }
        }
        auto room = std::make_shared<Room>(
            message.RoomId(), socket_of_first_player, message);
        rooms.push_back(room);
        return room;
    }

    void ShutdownAndRemoveRoom(std::shared_ptr<Room> room,
                               std::shared_ptr<SurakartaLogger> logger) {
        if (room->status == RoomStatus::REMOVED)
            return;
        auto daemon = room->daemon;
        if (daemon) {
            daemon->OnGameEnded.RemoveListeners();
            daemon->OnUpdateBoard.RemoveListeners();
            while (daemon->Status() != SurakartaDaemon::ExecuteStatus::ENDED) {
                try {
                    if (daemon->Status() == SurakartaDaemon::ExecuteStatus::WAITING_FOR_BLACK_AGENT) {
                        (room->first_player_color == PieceColor::BLACK ? room->first_player_handler : room->second_player_handler)
                            ->CommitMoveRaw(SurakartaMove(0, 0, 0, 0, PieceColor::BLACK));
                    } else if (daemon->Status() == SurakartaDaemon::ExecuteStatus::WAITING_FOR_WHITE_AGENT) {
                        (room->first_player_color == PieceColor::WHITE ? room->first_player_handler : room->second_player_handler)
                            ->CommitMoveRaw(SurakartaMove(0, 0, 0, 0, PieceColor::WHITE));
                    }
                } catch (...) {
                    // ignore
                }
            }
        }
        if (room->daemon_thread && room->daemon_thread->joinable()) {
            room->daemon_thread->join();
        }
        if (room->status == RoomStatus::WAITING_SECOND_PLAYER) {
            room->StartFailed();
        }
        {
            std::lock_guard lock(mutex);
            // remove the room from the list
            for (int i = 0; i < (int)rooms.size(); i++) {
                if (rooms[i]->id == room->id) {
                    rooms.erase(rooms.begin() + i);
                    logger->Log("Room %d is closed.", room->id);
                    break;
                }
            }
            room->status = RoomStatus::REMOVED;
        }
    }

    std::optional<SurakartaNetworkMessageReady> WaitReadyMessage(
        std::shared_ptr<NetworkFramework::Socket> socket,
        std::optional<NetworkFramework::Message> message_remained_in_last_loop) {
        auto message_opt = message_remained_in_last_loop.has_value()
                               ? message_remained_in_last_loop
                               : socket->Receive();
        while (true) {
            if (message_opt.has_value() == false) {
                // disconnect
                return std::nullopt;
            } else {
                if (message_opt.value().opcode == OPCODE::READY_OP) {
                    return SurakartaNetworkMessageReady(message_opt.value());
                } else {
                    // invalid opcode; just ignore
                }
            }
        }
    }

    static std::optional<std::pair<PieceColor, PieceColor>> ResolveColor(std::pair<PieceColor, PieceColor> request) {
        if (request.first != PieceColor::NONE && request.second != PieceColor::NONE) {
            if (request.first == request.second) {
                return std::nullopt;
            }
            return request;
        } else if (request.first == PieceColor::NONE && request.second == PieceColor::NONE) {
            if (GlobalRandomGenerator::getInstance()() % 2 == 0) {
                return std::make_pair(PieceColor::WHITE, PieceColor::BLACK);
            } else {
                return std::make_pair(PieceColor::BLACK, PieceColor::WHITE);
            }
        } else if (request.first == PieceColor::NONE) {
            return std::make_pair(ReverseColor(request.second), request.second);
        } else {
            return std::make_pair(request.first, ReverseColor(request.first));
        }
    }

    void Execute(std::shared_ptr<NetworkFramework::Socket> socket) override {
        auto peer_address = socket->PeerAddress();
        auto peer_port = socket->PeerPort();
        auto logger = logger_->CreateSublogger(peer_address + ":" + std::to_string(peer_port));
        try {
            socket = std::make_shared<SurakartaNetworkSocketLogWrapper>(std::move(socket), logger);
            socket = std::make_shared<SurakartaExceptionAsEofWrapper>(std::move(socket));
            logger->Log("Connection established.");
            auto message_remained_in_last_loop = std::optional<NetworkFramework::Message>();
            while (true) {
                auto ready_message_opt = WaitReadyMessage(socket, message_remained_in_last_loop);
                if (ready_message_opt.has_value() == false) {
                    // disconnect
                    return;
                }
                auto ready_decoded = SurakartaNetworkMessageReady(ready_message_opt.value());
                std::shared_ptr<Room> room =
                    GetOrCreateRoom(ready_decoded, socket);
                auto room_logger = logger->CreateSublogger("room " + std::to_string(room->id));
                int result = room->WaitingIfIsFirst(room_logger);
                bool is_first_player;
                if (result == 0) {
                    // This thread is for the first player
                    is_first_player = true;
                } else if (result == 1) {
                    ShutdownAndRemoveRoom(room, room_logger);
                    continue;
                } else if (room->Status() == RoomStatus::WAITING_SECOND_PLAYER) {
                    // This thread is for the second player
                    // assign color
                    is_first_player = false;
                    room_logger->Log("Try to join room.");
                    PieceColor first_player_requested_color = room->first_player_message.Color();
                    PieceColor second_player_requested_color = ready_decoded.Color();
                    auto resolved_colors = ResolveColor(std::make_pair(first_player_requested_color, second_player_requested_color));
                    if (resolved_colors.has_value() == false) {
                        // color conflict
                        room_logger->Log("Color conflict.");
                        SurakartaNetworkMessageReject reject_message(ready_decoded.Username(), "Color conflict.");
                        socket->Send(reject_message);
                        room->first_player_socket->Send(reject_message);
                        room->StartFailed();
                        continue;
                    }
                    auto first_player_color = resolved_colors.value().first;
                    auto second_player_color = resolved_colors.value().second;
                    // create daemon
                    auto first_player_handler = std::make_shared<SurakartaAgentInteractiveHandler>();
                    auto second_player_handler = std::make_shared<SurakartaAgentInteractiveHandler>();
                    first_player_handler->BlockAgentCreation();
                    second_player_handler->BlockAgentCreation();
                    auto daemon = std::make_shared<SurakartaDaemon>(
                        BOARD_SIZE, MAX_NO_CAPTURE_ROUND,
                        first_player_color == PieceColor::BLACK ? first_player_handler->GetAgentFactory()
                                                                : second_player_handler->GetAgentFactory(),
                        first_player_color == PieceColor::WHITE ? first_player_handler->GetAgentFactory()
                                                                : second_player_handler->GetAgentFactory());
                    // start the daemon
                    auto daemon_thread = std::make_shared<std::thread>([daemon, logger] {
                        try {
                            daemon->Execute();
                        } catch (const std::exception& e) {
                            logger->Log("Daemon thread failed: %s", e.what());
                        } catch (...) {
                            logger->Log("Daemon thread failed: unknown error");
                        }
                    });
                    room->StartRoom(
                        socket, first_player_color, second_player_color,
                        room->first_player_message.Username(), ready_decoded.Username(),
                        first_player_handler, second_player_handler,
                        daemon, daemon_thread);
                    room_logger->Log("Room is ready.");
                } else {
                    // This room is not available
                    SurakartaNetworkMessageReject reject_message(ready_decoded.Username(), std::string("Room ") + std::to_string(room->id) + " is not creatable or joinable.");
                    socket->Send(reject_message);
                    continue;
                }
                const auto first_player_color = room->first_player_color;
                const auto second_player_color = room->second_player_color;
                const auto my_color = is_first_player ? first_player_color : second_player_color;
                auto my_handler = is_first_player ? room->first_player_handler : room->second_player_handler;
                auto peer_socket = is_first_player ? room->second_player_socket : room->first_player_socket;
                auto peer_username = is_first_player ? room->second_player_username : room->first_player_username;
                my_handler->OnMoveCommitted.AddListener([my_color, socket, logger](SurakartaMoveTrace trace) {
                    if (trace.color == ReverseColor(my_color)) {
                        auto from = trace.path[0].From();
                        auto to = trace.path[trace.path.size() - 1].To();
                        SurakartaNetworkMessageMove message(from, to);
                        socket->Send(message);
                    }
                });
                my_handler->OnGameEnded.AddListener([&](SurakartaMoveResponse response) {
                    std::lock_guard lock(room->mutex);
                    if (room->status == RoomStatus::PLAYING) {
                        auto message = SurakartaNetworkMessageEnd(response.GetMoveReason(), response.GetEndReason(), response.GetWinner());
                        socket->Send(message);
                        peer_socket->Send(message);
                        room->status = RoomStatus::ENDED;
                    }
                });
                // check room status
                {
                    std::lock_guard lock(room->mutex);
                    if (room->status == RoomStatus::ENDED) {
                        // game is ended and status is changed by daemon thread
                        //
                        // there must be one and only one thread that has flag is_exited_by_this_thread
                        // to clean the room
                        {
                            room->status = RoomStatus::CLOSED;
                        }
                        ShutdownAndRemoveRoom(room, room_logger);
                        continue;
                    }
                    if (room->status == RoomStatus::CLOSED) {
                        continue;
                    }
                }
                // send ready message
                SurakartaNetworkMessageReady ready_message(
                    peer_username, my_color, room->id);
                socket->Send(ready_message);
                // preparations have been done; allow agent creation
                my_handler->UnblockAgentCreation();

                auto Resign = [&](bool do_not_send_to_this_socket = false) {
                    // connection has been unexpectedly closed
                    SurakartaNetworkMessageEnd message(
                        std::nullopt,
                        SurakartaEndReason::RESIGN,
                        ReverseColor(my_color));
                    if (!do_not_send_to_this_socket) {
                        // socket->Send(message);
                    }
                    peer_socket->Send(message);
                    {
                        std::lock_guard lock(room->mutex);
                        room->status = RoomStatus::CLOSED;
                    }
                    ShutdownAndRemoveRoom(room, room_logger);
                };

                // now, the game is started
                try {
                    room_logger->Log("Listen loop is started.");
                    while (true) {
                        auto message_opt = socket->Receive();
                        {
                            std::lock_guard lock(room->mutex);
                            if (room->status != RoomStatus::PLAYING) {
                                if (room->status == RoomStatus::ENDED) {
                                    // game is ended and status is changed by daemon thread
                                    //
                                    // there must be one and only one thread that has flag is_exited_by_this_thread
                                    // to clean the room
                                    room->status = RoomStatus::CLOSED;
                                    ShutdownAndRemoveRoom(room, room_logger);
                                }
                                message_remained_in_last_loop = message_opt;
                                break;
                            }
                        }
                        if (message_opt.has_value() == false) {
                            Resign();
                            break;
                        }
                        auto message = message_opt.value();
                        if (message.opcode == OPCODE::MOVE_OP) {
                            // move piece
                            auto decoded = SurakartaNetworkMessageMove(message);
                            my_handler->CommitMoveRaw(
                                SurakartaMove(decoded.From(), decoded.To(), my_handler->MyColor()));
                        } else if (message.opcode == OPCODE::LEAVE_OP) {
                            // leave room
                            Resign();
                            break;
                        } else if (message.opcode == OPCODE::RESIGN_OP) {
                            // resign
                            Resign();
                            break;
                        } else if (message.opcode == OPCODE::CHAT_OP) {
                            // chat
                            auto decoded = SurakartaNetworkMessageChat(message);
                            peer_socket->Send(decoded);
                        } else {
                            // invalid opcode; just ignore
                        }
                    }
                } catch (...) {
                    room->status = RoomStatus::CLOSED;
                    ShutdownAndRemoveRoom(room, room_logger);
                }
            }
        } catch (const std::exception& e) {
            logger->Log("Oops! Service failed with exception: %s", e.what());
        } catch (...) {
            logger->Log("Oops! Service failed with unknown exception.");
        }
    }

    void ShutdownService() {
        while (rooms.empty() == false) {
            ShutdownAndRemoveRoom(rooms[0], logger_);
        }
    }

   private:
    std::shared_ptr<SurakartaLogger> logger_;
};

SurakartaNetworkService::SurakartaNetworkService(std::shared_ptr<SurakartaLogger> logger)
    : impl_(std::make_shared<SurakartaNetworkServiceImpl>(logger)) {}

void SurakartaNetworkService::Execute(std::shared_ptr<NetworkFramework::Socket> socket) {
    impl_->Execute(socket);
}

void SurakartaNetworkService::ShutdownService() {
    impl_->ShutdownService();
}
