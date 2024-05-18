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
