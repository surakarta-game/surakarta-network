#pragma once

#include "surakarta_daemon.h"

class SurakartaAgentRemoteFactoryImpl;

class SurakartaAgentRemoteFactory : public SurakartaDaemon::AgentFactory {
   public:
    /// @brief Connect to a remote Surakarta server. This may take a while or throw an exception if the connection fails.
    /// @param address The address of the server.
    /// @param port The port of the server.
    /// @param username The username to use.
    /// @param room_id The room ID to join.
    /// @param requested_color The color to request.
    SurakartaAgentRemoteFactory(
        std::string address,
        int port,
        std::string username,
        int room_id,
        PieceColor requested_color = PieceColor::NONE);

    std::unique_ptr<SurakartaAgentBase> CreateAgent(
        std::shared_ptr<SurakartaGameInfo> game_info,
        std::shared_ptr<SurakartaBoard> board,
        std::shared_ptr<SurakartaRuleManager> rule_manager,
        SurakartaDaemon& daemon,
        PieceColor my_color) override;

    SurakartaEvent<std::optional<SurakartaIllegalMoveReason>, SurakartaEndReason, PieceColor> OnGameEnded;
    SurakartaEvent<std::string, std::string> OnChatMessageArrived;

   private:
    std::unique_ptr<SurakartaAgentRemoteFactoryImpl> impl_;
};
