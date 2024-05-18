#pragma once

#include "network_framework.h"
#include "opcode.h"
#include "surakarta.h"

class SurakartaNetworkMessageReady : public NetworkFramework::Message {
   public:
    SurakartaNetworkMessageReady(const std::string& username,
                                 PieceColor color,
                                 int room_id);

    SurakartaNetworkMessageReady(const NetworkFramework::Message& message);

    std::string Username() const { return username_; }
    PieceColor Color() const { return color_; }
    int RoomId() const { return room_id_; }

   private:
    std::string username_;
    PieceColor color_;
    int room_id_;
};

class SurakartaNetworkMessageReject : public NetworkFramework::Message {
   public:
    SurakartaNetworkMessageReject(const std::string& username,
                                  const std::string& reason);

    SurakartaNetworkMessageReject(const NetworkFramework::Message& message);

    std::string Username() const { return username_; }
    std::string Reason() const { return reason_; }

   private:
    std::string username_;
    std::string reason_;
};

class SurakartaNetworkMessageMove : public NetworkFramework::Message {
   public:
    SurakartaNetworkMessageMove(const SurakartaPosition& from, const SurakartaPosition& to);
    SurakartaNetworkMessageMove(const NetworkFramework::Message& message);

    SurakartaPosition From() const { return from_; }
    SurakartaPosition To() const { return to_; }

   private:
    SurakartaPosition from_;
    SurakartaPosition to_;
};

class SurakartaNetworkMessageResign : public NetworkFramework::Message {
   public:
    SurakartaNetworkMessageResign();
    SurakartaNetworkMessageResign(const NetworkFramework::Message& message);
};

class SurakartaNetworkMessageEnd : public NetworkFramework::Message {
   public:
    SurakartaNetworkMessageEnd(std::optional<SurakartaIllegalMoveReason> illegal_move_reason,
                               SurakartaEndReason end_reason,
                               PieceColor winner);
    SurakartaNetworkMessageEnd(const NetworkFramework::Message& message);

    std::optional<SurakartaIllegalMoveReason> IllegalMoveReason() const { return illegal_move_reason_; }
    SurakartaEndReason EndReason() const { return end_reason_; }
    PieceColor Winner() const { return winner_; }

   private:
    std::optional<SurakartaIllegalMoveReason> illegal_move_reason_;
    SurakartaEndReason end_reason_;
    PieceColor winner_;
};

class SurakartaNetworkMessageLeave : public NetworkFramework::Message {
   public:
    SurakartaNetworkMessageLeave(const std::string& username, const std::string& leave_reason);
    SurakartaNetworkMessageLeave(const NetworkFramework::Message& message);

    std::string Username() const { return username_; }
    std::string LeaveReason() const { return leave_reason_; }

   private:
    std::string username_;
    std::string leave_reason_;
};

class SurakartaNetworkMessageChat : public NetworkFramework::Message {
   public:
    SurakartaNetworkMessageChat(const std::string& username, const std::string& chat_message);
    SurakartaNetworkMessageChat(const NetworkFramework::Message& message);

    std::string Username() const { return username_; }
    std::string ChatMessage() const { return chat_message_; }

   private:
    std::string username_;
    std::string chat_message_;
};
