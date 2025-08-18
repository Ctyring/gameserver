#include <asio.hpp>
#include <iostream>
#include <thread>

using asio::ip::tcp;

// 异步 TCP 服务器
void tcp_server() {
    try {
        asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));

        std::cout << "[TCP Server] Listening on port 12345..." << std::endl;

        // 创建 socket
        auto socket = std::make_shared<tcp::socket>(io);

        // 开始异步接受连接
        acceptor.async_accept(*socket, [socket](std::error_code ec) {
            if (!ec) {
                std::cout << "[TCP Server] Client connected." << std::endl;

                auto buf = std::make_shared<std::array<char, 1024>>();

                // 开始异步读取
                socket->async_read_some(asio::buffer(*buf),
                                        [socket, buf](std::error_code ec, std::size_t len) {
                                            if (!ec) {
                                                std::cout << "[TCP Server] Received: "
                                                          << std::string(buf->data(), len) << std::endl;

                                                // 异步写回
                                                auto msg = std::make_shared<std::string>("Hello from server\n");
                                                asio::async_write(*socket, asio::buffer(*msg),
                                                                  [msg](std::error_code ec, std::size_t) {
                                                                      if (!ec)
                                                                          std::cout << "[TCP Server] Response sent."
                                                                                    << std::endl;
                                                                  });
                                            } else {
                                                std::cerr << "[TCP Server] Read error: " << ec.message() << std::endl;
                                            }
                                        });
            }
        });

        io.run();
    } catch (std::exception &e) {
        std::cerr << "[TCP Server] Exception: " << e.what() << std::endl;
    }
}

// 异步 TCP 客户端
void tcp_client() {
    try {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 等待服务器启动
        asio::io_context io;
        tcp::socket socket(io);

        socket.async_connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"), 12345),
                             [&socket](std::error_code ec) {
                                 if (!ec) {
                                     std::cout << "[TCP Client] Connected to server." << std::endl;

                                     auto msg = std::make_shared<std::string>("Hello from client\n");
                                     asio::async_write(socket, asio::buffer(*msg),
                                                       [msg](std::error_code ec, std::size_t) {
                                                           if (!ec)
                                                               std::cout << "[TCP Client] Sent message." << std::endl;
                                                       });

                                     auto buf = std::make_shared<std::array<char, 1024>>();
                                     socket.async_read_some(asio::buffer(*buf),
                                                            [buf](std::error_code ec, std::size_t len) {
                                                                if (!ec)
                                                                    std::cout << "[TCP Client] Received: "
                                                                              << std::string(buf->data(), len)
                                                                              << std::endl;
                                                            });
                                 }
                             });

        io.run();
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
