#include "surakarta_network_service.h"

void SurakartaNetworkService::Execute(std::shared_ptr<NetworkFramework::Socket> socket) {
}

std::unique_ptr<NetworkFramework::Service> SurakartaNetworkServiceFactory::Create() {
    return std::unique_ptr<SurakartaNetworkService>();
}
