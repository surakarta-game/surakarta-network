#pragma once

#include "service.h"

class SurakartaNetworkServiceFactoryImpl;

class SurakartaNetworkServiceFactory : public NetworkFramework::ServiceFactory {
   public:
    SurakartaNetworkServiceFactory();
    std::unique_ptr<NetworkFramework::Service> Create() override;

   private:
    std::shared_ptr<SurakartaNetworkServiceFactoryImpl> impl_;
};
