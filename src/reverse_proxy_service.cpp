#include "reverse_proxy_service.h"

void ReverseProxyService::Execute(std::shared_ptr<NetworkFramework::Socket> socket) {
    const auto server_socket = NetworkFramework::ConnectToServer(server_address_, server_port_);
    socket = middle_ware_(socket);
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