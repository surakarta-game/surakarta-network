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

    void Execute(
        std::shared_ptr<NetworkFramework::Socket> socket) override {
        const auto server_socket = middle_ware_(NetworkFramework::ConnectToServer(server_address_, server_port_));
        std::thread client_to_server_thread([&]() {
            try {
                while (true) {
                    auto message = socket->Receive();
                    if (!message.has_value()) {
                        break;
                    }
                    server_socket->Send(message.value());
                }
                server_socket->Close();
            } catch (...) {
                // do nothing
            }
        });
        std::thread server_to_client_thread([&]() {
            try {
                while (true) {
                    auto message = server_socket->Receive();
                    if (!message.has_value()) {
                        break;
                    }
                    socket->Send(message.value());
                }
                socket->Close();
            } catch (...) {
                // do nothing
            }
        });
        client_to_server_thread.join();
        server_to_client_thread.join();
    }

   private:
    std::string server_address_;
    int server_port_;
    SocketMiddleWare middle_ware_;
};
