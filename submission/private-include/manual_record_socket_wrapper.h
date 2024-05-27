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
        : socket_(socket), only_manual_color_(only_manual_color) {
        manual_file.open(manual_path);
    }

    ~SurakartaManualRecordSocketWrapper() {
        manual_file.close();
    }

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
    const std::shared_ptr<NetworkFramework::Socket> socket_;
    const PieceColor only_manual_color_;
    bool is_manual_enabled_ = false;
    std::ofstream manual_file;

    void Handle(const NetworkFramework::Message& message) {
        std::lock_guard lock(mutex_);
        if (message.opcode == OPCODE::READY_OP) {
            if (only_manual_color_ == PieceColor::NONE ||
                SurakartaNetworkMessageReady(message).Color() == only_manual_color_) {
                is_manual_enabled_ = true;
            }
        } else if (is_manual_enabled_) {
            if (message.opcode == OPCODE::MOVE_OP) {
                manual_file << message.data1 << "-" << message.data2 << " ";
            } else if (message.opcode == OPCODE::END_OP) {
                auto decoded = SurakartaNetworkMessageEnd(message);
                manual_file << SurakartaToString(decoded.EndReason())[0] << "#";
                is_manual_enabled_ = false;
            }
        }
        manual_file.flush();
    }
};
