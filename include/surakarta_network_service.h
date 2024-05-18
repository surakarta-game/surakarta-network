#pragma once

#include "service.h"

class SurakartaNetworkService : public NetworkFramework::Service {
   public:
    void Execute(std::shared_ptr<NetworkFramework::Socket> socket) override;
};

class SurakartaNetworkServiceFactory : public NetworkFramework::ServiceFactory {
   public:
    std::unique_ptr<NetworkFramework::Service> Create() override;
};
