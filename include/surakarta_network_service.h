#pragma once

#include "service.h"
#include "surakarta_logger.h"

class SurakartaNetworkServiceImpl;

class SurakartaNetworkService : public NetworkFramework::Service {
   public:
    SurakartaNetworkService(
        std::shared_ptr<SurakartaLogger> logger = std::make_shared<SurakartaLoggerNull>());

    void Execute(std::shared_ptr<NetworkFramework::Socket> socket) override;

    /// @brief This method should be called manually before server shutdown.
    void ShutdownService();

   private:
    std::shared_ptr<SurakartaNetworkServiceImpl> impl_;
};
