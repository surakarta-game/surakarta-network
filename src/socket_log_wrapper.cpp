#include "socket_log_wrapper.h"
#include "message.h"

static void LogMessage(std::shared_ptr<SurakartaLogger> logger,
                       NetworkFramework::Message message,
                       bool sending) {
    if (sending)
        logger = logger->CreateSublogger("send");
    else
        logger = logger->CreateSublogger("recv");
    if (message.opcode == OPCODE::READY_OP) {
        auto decoded = SurakartaNetworkMessageReady(message);
        auto username = decoded.Username();
        auto color = SurakartaToString(decoded.Color());
        auto room_id = decoded.RoomId();
        logger->Log("Ready message: username: \"%s\", color: %s, room id: %d",
                    username.c_str(), color.c_str(), room_id);
    } else if (message.opcode == OPCODE::REJECT_OP) {
        auto decoded = SurakartaNetworkMessageReject(message);
        auto username = decoded.Username();
        auto reason = decoded.Reason();
        logger->Log("Reject message: username: \"%s\", reason: \"%s\"",
                    username.c_str(), reason.c_str());
    } else if (message.opcode == OPCODE::MOVE_OP) {
        auto decoded = SurakartaNetworkMessageMove(message);
        auto from = decoded.data1;
        auto to = decoded.data2;
        logger->Log("Move message: from: %s, to: %s",
                    from.c_str(), to.c_str());
    } else if (message.opcode == OPCODE::CHAT_OP) {
        auto decoded = SurakartaNetworkMessageChat(message);
        auto username = decoded.Username();
        auto chat_message = decoded.ChatMessage();
        logger->Log("Chat message: username: \"%s\", message: \"%s\"",
                    username.c_str(), chat_message.c_str());
    } else if (message.opcode == OPCODE::END_OP) {
        auto decoded = SurakartaNetworkMessageEnd(message);
        auto move_reason = decoded.IllegalMoveReason().has_value()
                               ? SurakartaToString(decoded.IllegalMoveReason().value())
                               : "EMPTY";
        auto end_reason = SurakartaToString(decoded.EndReason());
        auto winner = SurakartaToString(decoded.Winner());
        logger->Log("End message: move reason: %s, end reason: %s, winner: %s",
                    move_reason.c_str(), end_reason.c_str(), winner.c_str());
    } else if (message.opcode == OPCODE::LEAVE_OP) {
        auto decoded = SurakartaNetworkMessageLeave(message);
        auto username = decoded.Username();
        auto leave_reason = decoded.LeaveReason();
        logger->Log("Leave message: username: \"%s\", leave reason: \"%s\"",
                    username.c_str(), leave_reason.c_str());
    } else if (message.opcode == OPCODE::RESIGN_OP) {
        auto decoded = SurakartaNetworkMessageResign(message);
        logger->Log("Resign message");
    } else {
        logger->Log("Unknown message: %d \"%s\" \"%s\" \"%s\"",
                    message.opcode, message.data1.c_str(), message.data2.c_str(), message.data3.c_str());
    }
}

void SurakartaNetworkSocketLogWrapper::Send(NetworkFramework::Message message) {
    LogMessage(logger_, message, true);
    socket_->Send(message);
}

std::optional<NetworkFramework::Message> SurakartaNetworkSocketLogWrapper::Receive() {
    auto message = socket_->Receive();
    if (message.has_value()) {
        LogMessage(logger_, message.value(), false);
    }
    return message;
}
