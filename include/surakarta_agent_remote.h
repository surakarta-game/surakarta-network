#pragma once

#include "surakarta_agent_base.h"

class SurakartaAgentRemote : public SurakartaAgentBase {
   public:
    SurakartaMove CalculateMove() override;
};
