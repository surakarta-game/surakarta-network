#pragma once

#include "socket.h"
#include "surakarta_logger.h"

class SurakartaNetworkSocketRawLogWrapper : public NetworkFramework::Socket {
   public:
    SurakartaNetworkSocketRawLogWrapper(
        std::shared_ptr<NetworkFramework::Socket> socket,
        std::shared_ptr<SurakartaLogger> logger)
        : socket_(std::move(socket)), logger_(logger) {}

    void Send(NetworkFramework::Message message) override {
        logger_->CreateSublogger("send raw")->Log("Send: %d \"%s\" \"%s\" \"%s\"", message.opcode, message.data1.c_str(), message.data2.c_str(), message.data3.c_str());
        socket_->Send(message);
    }

    std::optional<NetworkFramework::Message> Receive() override {
        auto message = socket_->Receive();
        if (message.has_value()) {
            logger_->CreateSublogger("recv raw")->Log("Receive: %d \"%s\" \"%s\" \"%s\"", message->opcode, message->data1.c_str(), message->data2.c_str(), message->data3.c_str());
        }
        return message;
    }

    void Close() override { socket_->Close(); }
    std::string PeerAddress() const override { return socket_->PeerAddress(); }
    int PeerPort() const override { return socket_->PeerPort(); }

   private:
    std::shared_ptr<NetworkFramework::Socket> socket_;
    std::shared_ptr<SurakartaLogger> logger_;
};
