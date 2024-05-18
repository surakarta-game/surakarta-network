#include "surakarta_network_service.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include "message.h"
#include "opcode.h"
#include "surakarta.h"

struct SurakartaNetworkServiceSharedData {
    enum class RoomStatus {
        EMPTY,
        WAITING_SECOND_PLAYER,
        PLAYING,
        ENDED,
        CLOSED,
    };

    struct Room {
        int id;  // This field can be access without lock, since it is only written once
        std::mutex mutex;
        std::condition_variable when_game_started_or_initialization_failed;
        RoomStatus status = RoomStatus::EMPTY;
        std::shared_ptr<NetworkFramework::Socket> first_player_socket;
        std::shared_ptr<NetworkFramework::Socket> second_player_socket;
        SurakartaNetworkMessageReady first_player_message;
        std::shared_ptr<SurakartaDaemon> daemon;
        std::shared_ptr<std::thread> daemon_thread;
        std::shared_ptr<SurakartaAgentInteractiveHandler> first_player_handler;
        std::shared_ptr<SurakartaAgentInteractiveHandler> second_player_handler;

        Room(int id,
             std::shared_ptr<NetworkFramework::Socket> first_player_socket,
             SurakartaNetworkMessageReady first_player_message)
            : id(id), first_player_socket(first_player_socket), first_player_message(first_player_message) {}
    };

    std::mutex mutex;
    std::vector<std::shared_ptr<Room>> rooms;
};

class SurakartaNetworkServiceImpl : public NetworkFramework::Service {
   public:
    SurakartaNetworkServiceImpl(std::shared_ptr<SurakartaNetworkServiceSharedData> shared_data)
        : shared_data_(shared_data) {}

    void Execute(std::shared_ptr<NetworkFramework::Socket> socket) override {
    wait_ready_message:
        auto message_opt = socket->Receive();
        if (message_opt.has_value() == false) {
            // disconnect
            return;
        }
        if (message_opt.value().opcode != OPCODE::READY_OP) {
            // invalid opcode; just ignore
            goto wait_ready_message;
        }
        auto decoded = SurakartaNetworkMessageReady(message_opt.value());
        const int room_id = decoded.RoomId();
        std::shared_ptr<SurakartaNetworkServiceSharedData::Room> room = nullptr;
        {
            std::lock_guard<std::mutex> lock(shared_data_->mutex);
            for (int i = 0; i < shared_data_->rooms.size(); i++) {
                if (shared_data_->rooms[i]->id == room_id) {
                    room = shared_data_->rooms[i];
                    break;
                }
            }
            if (room == nullptr) {
                room = std::make_shared<SurakartaNetworkServiceSharedData::Room>(
                    room_id, socket, decoded);
                room->id = room_id;
                shared_data_->rooms.push_back(room);
            }
        }
        SurakartaNetworkServiceSharedData::RoomStatus room_status;
        {
            std::unique_lock lock(room->mutex);
            room_status = room->status;
        }
        bool is_first_player = false;
        PieceColor first_player_color, second_player_color;
        if (room_status == SurakartaNetworkServiceSharedData::RoomStatus::EMPTY) {
            // This thread is for the first player
            is_first_player = true;
            std::unique_lock lock(room->mutex);
            room->status = SurakartaNetworkServiceSharedData::RoomStatus::WAITING_SECOND_PLAYER;
            room->first_player_socket = socket;
            room->first_player_message = decoded;
            // now nothing to do, just wait
            room->when_game_started_or_initialization_failed.wait(lock, [&] {
                return room->status != SurakartaNetworkServiceSharedData::RoomStatus::WAITING_SECOND_PLAYER;
            });
            if (room->status == SurakartaNetworkServiceSharedData::RoomStatus::EMPTY) {
                goto wait_ready_message;
            }
        } else if (room_status == SurakartaNetworkServiceSharedData::RoomStatus::WAITING_SECOND_PLAYER) {
            // This thread is for the second player
            std::lock_guard lock(room->mutex);
            room->second_player_socket = socket;
            // assign color
            PieceColor first_player_requested_color = room->first_player_message.Color();
            PieceColor second_player_requested_color = decoded.Color();
            if (first_player_requested_color == PieceColor::WHITE && second_player_requested_color == PieceColor::BLACK) {
                first_player_color = PieceColor::WHITE;
                second_player_color = PieceColor::BLACK;
            } else if (first_player_requested_color == PieceColor::BLACK && second_player_requested_color == PieceColor::WHITE) {
                first_player_color = PieceColor::BLACK;
                second_player_color = PieceColor::WHITE;
            } else if (first_player_requested_color == PieceColor::NONE && second_player_requested_color == PieceColor::NONE) {
                // randomly assign color
                if (GlobalRandomGenerator::getInstance()() % 2 == 0) {
                    first_player_color = PieceColor::WHITE;
                    second_player_color = PieceColor::BLACK;
                } else {
                    first_player_color = PieceColor::BLACK;
                    second_player_color = PieceColor::WHITE;
                }
            } else if (first_player_requested_color == PieceColor::NONE && second_player_requested_color != PieceColor::NONE) {
                // assign color
                first_player_color = ReverseColor(second_player_requested_color);
                second_player_color = second_player_requested_color;
            } else if (first_player_requested_color != PieceColor::NONE && second_player_requested_color == PieceColor::NONE) {
                // assign color
                first_player_color = first_player_requested_color;
                second_player_color = ReverseColor(first_player_requested_color);
            } else {
                // color conflict
                SurakartaNetworkMessageReject reject_message(decoded.Username(), "Color conflict.");
                socket->Send(reject_message);
                room->first_player_socket->Send(reject_message);
                room->status = SurakartaNetworkServiceSharedData::RoomStatus::EMPTY;
                room->when_game_started_or_initialization_failed.notify_all();
                goto wait_ready_message;
            }
            // create daemon
            room->first_player_handler = std::make_shared<SurakartaAgentInteractiveHandler>();
            room->second_player_handler = std::make_shared<SurakartaAgentInteractiveHandler>();
            room->first_player_handler->BlockAgentCreation();
            room->second_player_handler->BlockAgentCreation();
            room->daemon = std::make_shared<SurakartaDaemon>(
                BOARD_SIZE, MAX_NO_CAPTURE_ROUND,
                first_player_color == PieceColor::BLACK ? room->first_player_handler->GetAgentFactory()
                                                        : room->second_player_handler->GetAgentFactory(),
                first_player_color == PieceColor::WHITE ? room->first_player_handler->GetAgentFactory()
                                                        : room->second_player_handler->GetAgentFactory());
            // send ready message
            SurakartaNetworkMessageReady first_player_ready_message(
                decoded.Username(), first_player_color, room_id);
            SurakartaNetworkMessageReady second_player_ready_message(
                room->first_player_message.Username(), second_player_color, room_id);
            room->first_player_socket->Send(first_player_ready_message);
            socket->Send(second_player_ready_message);
            // notify the thread of the first player
            room->status = SurakartaNetworkServiceSharedData::RoomStatus::PLAYING;
            room->when_game_started_or_initialization_failed.notify_all();
            // start the daemon
            if (!is_first_player) {
                std::lock_guard lock(room->mutex);
                room->daemon_thread = std::make_shared<std::thread>([&] {
                    room->daemon->Execute();
                });
            }
        } else {
            // This room is not available
            SurakartaNetworkMessageReject reject_message(decoded.Username(), std::string("Room ") + std::to_string(room_id) + " is not creatable or joinable.");
            socket->Send(reject_message);
            goto wait_ready_message;
        }

        auto my_color = is_first_player ? first_player_color : second_player_color;
        std::shared_ptr<SurakartaAgentInteractiveHandler> my_handler;
        std::shared_ptr<NetworkFramework::Socket> peer_socket;
        {
            std::lock_guard lock(room->mutex);
            if (is_first_player) {
                my_handler = room->first_player_handler;
                peer_socket = room->second_player_socket;
            } else {
                my_handler = room->second_player_handler;
                peer_socket = room->first_player_socket;
            }
        }
        my_handler->OnMoveCommitted.AddListener([&](SurakartaMoveTrace trace) {
            if (trace.color == my_color) {
                auto from = trace.path[0].From();
                auto to = trace.path[trace.path.size() - 1].To();
                SurakartaNetworkMessageMove message(from, to);
                socket->Send(message);
            }
        });
        my_handler->OnGameEnded.AddListener([&](SurakartaMoveResponse response) {
            std::lock_guard lock(room->mutex);
            auto message = SurakartaNetworkMessageEnd(response.GetMoveReason(), response.GetEndReason(), response.GetWinner());
            socket->Send(message);
            peer_socket->Send(message);
            room->status = SurakartaNetworkServiceSharedData::RoomStatus::ENDED;
        });
        // preparations have been done; allow agent creation
        my_handler->UnblockAgentCreation();

        bool is_exited_by_this_thread = false;
        // now, the game is started
        try {
            while (true) {
                auto message_opt = socket->Receive();
                {
                    std::lock_guard lock(room->mutex);
                    if (room->status != SurakartaNetworkServiceSharedData::RoomStatus::PLAYING) {
                        if (room->status == SurakartaNetworkServiceSharedData::RoomStatus::ENDED) {
                            // game is ended and status is changed by daemon thread
                            //
                            // there must be one and only one thread that has flag is_exited_by_this_thread
                            // to clean the room
                            is_exited_by_this_thread = true;
                            room->status = SurakartaNetworkServiceSharedData::RoomStatus::CLOSED;
                            break;
                        } else {
                            break;
                        }
                    }
                }
                if (message_opt.has_value() == false) {
                resign:
                    // connection has been unexpectedly closed
                    SurakartaNetworkMessageEnd message(
                        std::nullopt,
                        SurakartaEndReason::RESIGN,
                        ReverseColor(my_color));
                    peer_socket->Send(message);
                    is_exited_by_this_thread = true;
                    room->status = SurakartaNetworkServiceSharedData::RoomStatus::CLOSED;
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
                    goto resign;
                } else if (message.opcode == OPCODE::RESIGN_OP) {
                    // resign
                    goto resign;
                } else if (message.opcode == OPCODE::CHAT_OP) {
                    // chat
                    auto decoded = SurakartaNetworkMessageChat(message);
                    peer_socket->Send(decoded);
                } else {
                    // invalid opcode; just ignore
                }
            }
        } catch (...) {
            is_exited_by_this_thread = true;
            room->status = SurakartaNetworkServiceSharedData::RoomStatus::CLOSED;
        }

        // now, the game is ended
        if (is_exited_by_this_thread) {
            // shutdown SurakartaDaemon
            auto daemon = room->daemon;
            if (daemon->Status() != SurakartaDaemon::ExecuteStatus::ENDED) {
                if (daemon->Status() == SurakartaDaemon::ExecuteStatus::WAITING_FOR_BLACK_AGENT) {
                    (first_player_color == PieceColor::BLACK ? room->first_player_handler : room->second_player_handler)
                        ->CommitMoveRaw(SurakartaMove(0, 0, 0, 0, PieceColor::BLACK));
                } else if (daemon->Status() == SurakartaDaemon::ExecuteStatus::WAITING_FOR_WHITE_AGENT) {
                    (first_player_color == PieceColor::WHITE ? room->first_player_handler : room->second_player_handler)
                        ->CommitMoveRaw(SurakartaMove(0, 0, 0, 0, PieceColor::WHITE));
                }
            }
            room->daemon_thread->join();
            // remove the room from the list
            for (int i = 0; i < shared_data_->rooms.size(); i++) {
                if (shared_data_->rooms[i]->id == room_id) {
                    shared_data_->rooms.erase(shared_data_->rooms.begin() + i);
                    break;
                }
            }
        }
        goto wait_ready_message;
    }

   private:
    std::shared_ptr<SurakartaNetworkServiceSharedData> shared_data_;
};

class SurakartaNetworkServiceFactoryImpl : public NetworkFramework::ServiceFactory {
   public:
    SurakartaNetworkServiceFactoryImpl() {
        shared_data_ = std::make_shared<SurakartaNetworkServiceSharedData>();
    }

    std::unique_ptr<NetworkFramework::Service> Create() override {
        return std::make_unique<SurakartaNetworkServiceImpl>(shared_data_);
    }

   private:
    std::shared_ptr<SurakartaNetworkServiceSharedData> shared_data_;
};

SurakartaNetworkServiceFactory::SurakartaNetworkServiceFactory() {
    impl_ = std::make_unique<SurakartaNetworkServiceFactoryImpl>();
}

std::unique_ptr<NetworkFramework::Service> SurakartaNetworkServiceFactory::Create() {
    return impl_->Create();
}
