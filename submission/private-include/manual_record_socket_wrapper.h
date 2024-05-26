#pragma once

#include <memory>
#include "../../src/private-include/message.h"
#include "socket.h"

class SurakartaManualRecordSocketWrapper : public NetworkFramework::Socket {
   public:
    SurakartaManualRecordSocketWrapper(
        std::shared_ptr<NetworkFramework::Socket> socket,
        std::string manual_path,
        PieceColor only_manual_color = PieceColor::NONE)
        : manual_path_(manual_path), socket_(socket), only_manual_color_(only_manual_color) {}

    void Send(NetworkFramework::Message message) override {
        Handle(message);
        socket_->Send(message);
    }

    std::optional<NetworkFramework::Message> Receive() override {
        auto message = socket_->Receive();
        if (message.has_value()) {
            Handle(message.value());
        }
        return message;
    }

    void Close() override { socket_->Close(); }
    std::string PeerAddress() const override { return socket_->PeerAddress(); }
    int PeerPort() const override { return socket_->PeerPort(); }

   private:
    std::mutex mutex_;
    std::string manual_;
    std::string manual_path_;
    const std::shared_ptr<NetworkFramework::Socket> socket_;
    const PieceColor only_manual_color_;
    bool is_manual_enabled_ = false;

    void Handle(const NetworkFramework::Message& message) {
        std::lock_guard lock(mutex_);
        if (message.opcode == OPCODE::READY_OP) {
            if (only_manual_color_ == PieceColor::NONE ||
                SurakartaNetworkMessageReady(message).Color() == only_manual_color_) {
                is_manual_enabled_ = true;
            }
        } else if (is_manual_enabled_) {
            if (message.opcode == OPCODE::MOVE_OP) {
                manual_ += message.data1 + "-" + message.data2 + " ";
            } else if (message.opcode == OPCODE::END_OP) {
                auto decoded = SurakartaNetworkMessageEnd(message);
                manual_ += SurakartaToString(decoded.EndReason())[0];
                manual_ += "#";
                is_manual_enabled_ = false;
            }
        }
        std::ofstream manual_file(manual_path_, std::ios::out | std::ios::ate);
        manual_file << manual_;
        manual_file.close();
    }
};
