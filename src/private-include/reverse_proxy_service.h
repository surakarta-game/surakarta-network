#pragma once

#include <functional>
#include <thread>
#include "network_framework.h"

class ReverseProxyService : public NetworkFramework::Service {
   public:
    typedef std::function<std::shared_ptr<NetworkFramework::Socket>(std::shared_ptr<NetworkFramework::Socket>)> SocketMiddleWare;

    ReverseProxyService(
        std::string server_address,
        int server_port,
        SocketMiddleWare middle_ware = [](auto socket) { return socket; })
        : server_address_(server_address), server_port_(server_port), middle_ware_(middle_ware) {}

    void Execute(std::shared_ptr<NetworkFramework::Socket> socket) override;

   private:
    std::string server_address_;
    int server_port_;
    SocketMiddleWare middle_ware_;
};
