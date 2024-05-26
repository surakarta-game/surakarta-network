#pragma once

#include "opcode.h"
#include "socket.h"
#include "surakarta_logger.h"

class SurakartaExceptionAsEofWrapper : public NetworkFramework::Socket {
   public:
    SurakartaExceptionAsEofWrapper(
        std::shared_ptr<NetworkFramework::Socket> socket)
        : socket_(std::move(socket)) {}

    void Send(NetworkFramework::Message message) override {
        socket_->Send(std::move(message));
    }
    std::optional<NetworkFramework::Message> Receive() override {
        try {
            return socket_->Receive();
        } catch (...) {
            return std::nullopt;
        }
    }
    void Close() override { socket_->Close(); }
    std::string PeerAddress() const override { return socket_->PeerAddress(); }
    int PeerPort() const override { return socket_->PeerPort(); }

   private:
    std::shared_ptr<NetworkFramework::Socket> socket_;
};
