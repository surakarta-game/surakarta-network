#pragma once

#include "opcode.h"
#include "socket.h"
#include "surakarta_logger.h"

class SurakartaNetworkSocketLogWrapper : public NetworkFramework::Socket {
   public:
    SurakartaNetworkSocketLogWrapper(
        std::unique_ptr<NetworkFramework::Socket> socket,
        std::shared_ptr<SurakartaLogger> logger)
        : socket_(std::move(socket)), logger_(logger) {}

    void Send(NetworkFramework::Message message) override;
    std::optional<NetworkFramework::Message> Receive() override;
    void Close() override { socket_->Close(); }
    std::string PeerAddress() const override { return socket_->PeerAddress(); }
    int PeerPort() const override { return socket_->PeerPort(); }

   private:
    std::unique_ptr<NetworkFramework::Socket> socket_;
    std::shared_ptr<SurakartaLogger> logger_;
};
