#include <asio.hpp>
#include <iostream>
#include <thread>

using asio::ip::udp;

void udp_server() {
    try {
        asio::io_context io;
        udp::socket socket(io, udp::endpoint(udp::v4(), 12346));

        std::cout << "[UDP Server] Listening on port 12346..." << std::endl;

//        for (;;) {
            char data[1024];
            udp::endpoint sender;
            size_t len = socket.receive_from(asio::buffer(data), sender);
            std::cout << "[UDP Server] Received: " << std::string(data, len) << std::endl;

            socket.send_to(asio::buffer("Hello from UDP server\n"), sender);
//        }
    } catch (std::exception &e) {
        std::cerr << "[UDP Server] Exception: " << e.what() << std::endl;
    }
}

void udp_client() {
    try {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        asio::io_context io;
        udp::socket socket(io, udp::endpoint(udp::v4(), 0));

        udp::endpoint server(asio::ip::make_address("127.0.0.1"), 12346);

        socket.send_to(asio::buffer("Hello from UDP client\n"), server);

        char reply[1024];
        udp::endpoint sender;
        size_t len = socket.receive_from(asio::buffer(reply), sender);
        std::cout << "[UDP Client] Received: " << std::string(reply, len) << std::endl;
    } catch (std::exception &e) {
        std::cerr << "[UDP Client] Exception: " << e.what() << std::endl;
    }
}

int main() {
    std::thread server(udp_server);
    std::thread client(udp_client);
    server.join();
    client.join();
    return 0;
}
