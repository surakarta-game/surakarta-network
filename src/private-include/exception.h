#pragma once

#include <exception>
#include "network_framework.h"

class SurakartaNetworkException : public std::exception {};

template <typename T>
class SurakartaNetworkMessageParsingException : public SurakartaNetworkException {
   public:
    SurakartaNetworkMessageParsingException()
        : message_(std::string("Failed to parse message to type ") + typeid(T).name()) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

   private:
    std::string message_;
};

class SurakartaNetworkUnexpectedEofException : public SurakartaNetworkException {
   public:
    SurakartaNetworkUnexpectedEofException()
        : message_("Unexpected EOF while reading message") {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

   private:
    std::string message_;
};

class SurakartaNetworkUnexpectedMessageException : public SurakartaNetworkException {
   public:
    SurakartaNetworkUnexpectedMessageException(NetworkFramework::Message message)
        : message_(std::string("Unexpected message: Opcode = ") + std::to_string(message.opcode) +
                   ", data1 = " + message.data1 +
                   ", data2 = " + message.data2 +
                   ", data3 = " + message.data3) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

   private:
    std::string message_;
};

class SurakartaNetworkRejectedException : public SurakartaNetworkException {
   public:
    SurakartaNetworkRejectedException(const std::string& username, const std::string& reason)
        : message_(std::string("User ") + username + " is rejected by the server. Reason: " + reason),
          username_(username),
          reason_(reason) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    const std::string& Username() const {
        return username_;
    }

    const std::string& Reason() const {
        return reason_;
    }

   private:
    std::string message_;
    std::string username_;
    std::string reason_;
};

class SurakartaNetworkAgentColorMismatchException : public SurakartaNetworkException {
   public:
    SurakartaNetworkAgentColorMismatchException(PieceColor network_color, PieceColor agent_color)
        : message_(std::string("Network color ") +
                   (network_color == PieceColor::BLACK   ? "BLACK"
                    : network_color == PieceColor::WHITE ? "WHITE"
                    : network_color == PieceColor::NONE  ? "NONE"
                                                         : "UNKNOWN") +
                   " does not match agent color " +
                   (agent_color == PieceColor::BLACK   ? "BLACK"
                    : agent_color == PieceColor::WHITE ? "WHITE"
                    : agent_color == PieceColor::NONE  ? "NONE"
                                                       : "UNKNOWN")),
          network_color_(network_color),
          agent_color_(agent_color) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    PieceColor NetworkColor() const {
        return network_color_;
    }

    PieceColor AgentColor() const {
        return agent_color_;
    }

   private:
    std::string message_;
    PieceColor network_color_;
    PieceColor agent_color_;
};
