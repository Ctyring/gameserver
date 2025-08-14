#include <asio.hpp>
#include <iostream>
#include <thread>

using asio::ip::tcp;

void tcp_server() {
    try {
        asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));

        std::cout << "[TCP Server] Listening on port 12345..." << std::endl;

        tcp::socket socket(io);
        acceptor.accept(socket);
        std::cout << "[TCP Server] Client connected." << std::endl;

        for (;;) {
            char data[1024];
            asio::error_code ec;
            size_t len = socket.read_some(asio::buffer(data), ec);
            if (ec == asio::error::eof) {
                std::cout << "[TCP Server] Connection closed by client." << std::endl;
                break; // 正常断开
            } else if (ec) {
                std::cerr << "[TCP Server] Read error: " << ec.message() << std::endl;
                break;
            }
            if (len > 0) {
                std::cout << "[TCP Server] Received: " << std::string(data, len) << std::endl;
                asio::write(socket, asio::buffer("Hello from server\n"));
            }
        }
    } catch (std::exception &e) {
        std::cerr << "[TCP Server] Exception: " << e.what() << std::endl;
    }
}

void tcp_client() {
    try {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 等待服务器启动
        asio::io_context io;
        tcp::socket socket(io);
        socket.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"), 12345));

        asio::write(socket, asio::buffer("Hello from client\n"));

        char reply[1024];
        size_t len = socket.read_some(asio::buffer(reply));
        std::cout << "[TCP Client] Received: " << std::string(reply, len) << std::endl;
    } catch (std::exception &e) {
        std::cerr << "[TCP Client] Exception: " << e.what() << std::endl;
    }
}

int main() {
    std::thread server(tcp_server);
    std::thread client(tcp_client);
    server.join();
    client.join();
    return 0;
}
