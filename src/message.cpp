#include "message.h"
#include "exception.h"

SurakartaNetworkMessageReady::SurakartaNetworkMessageReady(const std::string& username, PieceColor color, int room_id)
    : NetworkFramework::Message(
          OPCODE::READY_OP,
          username,
          color == PieceColor::BLACK ? "BLACK" : color == PieceColor::WHITE ? "WHITE"
                                                                            : "",
          std::to_string(room_id)),
      username_(username),
      color_(color),
      room_id_(room_id) {}

SurakartaNetworkMessageReady::SurakartaNetworkMessageReady(const NetworkFramework::Message& message)
    : NetworkFramework::Message(message) {
    if (message.opcode != OPCODE::READY_OP) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageReady>();
    }
    username_ = data1;
    if (data2 != "BLACK" && data2 != "WHITE" && data3.empty() == false) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageReady>();
    }
    color_ = data2.empty() ? PieceColor::NONE : data2 == "BLACK" ? PieceColor::BLACK
                                                                 : PieceColor::WHITE;
    try {
        room_id_ = std::stoi(data3);
    } catch (std::invalid_argument& e) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageReady>();
    }
}

SurakartaNetworkMessageReject::SurakartaNetworkMessageReject(const std::string& username, const std::string& reason)
    : NetworkFramework::Message(OPCODE::REJECT_OP, username, reason), username_(username), reason_(reason) {}

SurakartaNetworkMessageReject::SurakartaNetworkMessageReject(const NetworkFramework::Message& message)
    : NetworkFramework::Message(message) {
    if (message.opcode != OPCODE::REJECT_OP) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageReject>();
    }
    username_ = data1;
    reason_ = data2;
}

static std::string ToString(const SurakartaPosition& pos) {
    char buffer[3];
    buffer[0] = 'A' + pos.x;
    buffer[1] = '1' + pos.y;
    buffer[2] = '\0';
    return std::string(buffer);
}

static SurakartaPosition ToPosition(const std::string& str) {
    if (str.size() != 2) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageMove>();
    }
    int x = str[0] - 'A';
    int y = str[1] - '1';
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageMove>();
    }
    return SurakartaPosition(x, y);
}

SurakartaNetworkMessageMove::SurakartaNetworkMessageMove(const SurakartaPosition& from, const SurakartaPosition& to)
    : NetworkFramework::Message(OPCODE::MOVE_OP, ToString(from), ToString(to)), from_(from), to_(to) {}

SurakartaNetworkMessageMove::SurakartaNetworkMessageMove(const NetworkFramework::Message& message)
    : NetworkFramework::Message(message) {
    if (message.opcode != OPCODE::MOVE_OP) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageMove>();
    }
    from_ = ToPosition(data1);
    to_ = ToPosition(data2);
}

SurakartaNetworkMessageResign::SurakartaNetworkMessageResign()
    : NetworkFramework::Message(OPCODE::RESIGN_OP) {}

SurakartaNetworkMessageResign::SurakartaNetworkMessageResign(const NetworkFramework::Message& message)
    : NetworkFramework::Message(message) {
    opcode = message.opcode;
    if (message.opcode != OPCODE::RESIGN_OP) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageResign>();
    }
}

SurakartaNetworkMessageEnd::SurakartaNetworkMessageEnd(
    std::optional<SurakartaIllegalMoveReason> illegal_move_reason,
    SurakartaEndReason end_reason,
    PieceColor winner)
    : NetworkFramework::Message(OPCODE::END_OP,
                                illegal_move_reason.has_value() ? std::to_string(static_cast<int>(illegal_move_reason.value())) : "",
                                std::to_string(static_cast<int>(end_reason)),
                                std::to_string(static_cast<int>(winner))),
      illegal_move_reason_(illegal_move_reason),
      end_reason_(end_reason),
      winner_(winner) {}

SurakartaNetworkMessageEnd::SurakartaNetworkMessageEnd(const NetworkFramework::Message& message)
    : NetworkFramework::Message(message) {
    if (message.opcode != OPCODE::END_OP) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageEnd>();
    }
    if (data1.empty()) {
        illegal_move_reason_ = std::nullopt;
    } else {
        try {
            illegal_move_reason_ = static_cast<SurakartaIllegalMoveReason>(std::stoi(data1));
        } catch (std::invalid_argument& e) {
            throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageEnd>();
        }
    }
    try {
        end_reason_ = static_cast<SurakartaEndReason>(std::stoi(data2));
    } catch (std::invalid_argument& e) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageEnd>();
    }
    try {
        winner_ = static_cast<PieceColor>(std::stoi(data3));
    } catch (std::invalid_argument& e) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageEnd>();
    }
}

SurakartaNetworkMessageLeave::SurakartaNetworkMessageLeave(const std::string& username, const std::string& leave_reason)
    : NetworkFramework::Message(OPCODE::LEAVE_OP, username, leave_reason), username_(username), leave_reason_(leave_reason) {}

SurakartaNetworkMessageLeave::SurakartaNetworkMessageLeave(const NetworkFramework::Message& message)
    : NetworkFramework::Message(message) {
    if (message.opcode != OPCODE::LEAVE_OP) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageLeave>();
    }
    username_ = data1;
    leave_reason_ = data2;
}

SurakartaNetworkMessageChat::SurakartaNetworkMessageChat(const std::string& username, const std::string& chat_message)
    : NetworkFramework::Message(OPCODE::CHAT_OP, username, chat_message), username_(username), chat_message_(chat_message) {}

SurakartaNetworkMessageChat::SurakartaNetworkMessageChat(const NetworkFramework::Message& message)
    : NetworkFramework::Message(message) {
    if (message.opcode != OPCODE::CHAT_OP) {
        throw SurakartaNetworkMessageParsingException<SurakartaNetworkMessageChat>();
    }
    username_ = data1;
    chat_message_ = data2;
}
