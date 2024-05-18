#pragma once

#include <exception>
#include "network_framework.h"

class SurakartaNetworkException : public std::exception {};

template <typename T>
class SurakartaNetworkMessageParsingException : public SurakartaNetworkException {
   public:
    char* what() const noexcept override {
        return "Failed to parse message to type " + typeid(T).name();
    }
};
