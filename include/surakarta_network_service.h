#pragma once

#include "service.h"
#include "surakarta_logger.h"

class SurakartaNetworkServiceFactoryImpl;

class SurakartaNetworkServiceFactory : public NetworkFramework::ServiceFactory {
   public:
    SurakartaNetworkServiceFactory(
        std::shared_ptr<SurakartaLogger> logger = std::make_shared<SurakartaLoggerNull>());
    std::unique_ptr<NetworkFramework::Service> Create() override;

   private:
    std::shared_ptr<SurakartaNetworkServiceFactoryImpl> impl_;
};
