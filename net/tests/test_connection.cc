// test_connection.cpp
#include "cfl/connection.h"
#include <asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace cfl;

// 简单的测试服务器
class TestServer {
public:
    TestServer(asio::io_context& io_context, short port)
            : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                std::cout << "Server: 新连接建立" << std::endl;
                // 简单回显服务器逻辑
                auto buffer = std::make_shared<std::array<char, 1024>>();
                std::cout << "Server: 等待接收数据..." << std::endl;

                socket.async_read_some(asio::buffer(*buffer),
                                       [buffer, socket = std::move(socket)](std::error_code ec, std::size_t length) mutable {
                                           if (!ec) {
                                               std::string received(buffer->data(), length);
                                               std::cout << "Server 接收到: " << received << std::endl;

                                               // 回传数据
                                               std::string response = "Echo: " + received;
                                               asio::write(socket, asio::buffer(response));
                                               std::cout << "Server 发送回执: " << response << std::endl;
                                           }
                                       });
            }

            // 继续接受下一个连接
            do_accept();
        });
    }

    asio::ip::tcp::acceptor acceptor_;
};

int main() {
    // 设置控制台为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    try {
        // 创建IO上下文
        asio::io_context io_context;

        // 初始化连接管理器
        ConnectionMgr::instance().init(io_context, 100);
        // 启动测试服务器
        TestServer server(io_context, 8080);
        std::cout << "测试服务器启动在端口 8080" << std::endl;

        // 在另一个线程运行io_context
        std::thread io_thread([&io_context]() {
            io_context.run();
        });

        // 等待服务器启动
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 测试连接功能
        std::cout << "\n=== 开始测试连接 ===" << std::endl;

        // 获取新连接
        auto connection = ConnectionMgr::instance().get_new_connection();
        if (!connection) {
            std::cerr << "无法获取新连接" << std::endl;
            return 1;
        }

        std::cout << "成功获取连接对象" << std::endl;

        // 设置连接ID
        connection->set_conn_id(1);
        std::cout << "设置连接ID为: " << connection->conn_id() << std::endl;

        // 连接到服务器
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "8080");

        asio::connect(connection->socket(), endpoints);
        std::cout << "成功连接到服务器" << std::endl;

        // 发送测试消息
        std::string test_message = "Hello, Connection Test!";
        connection->send(test_message);
        std::cout << "发送消息: " << test_message << std::endl;

        // 等待一段时间让消息传输完成
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 测试关闭连接
        connection->close();
        std::cout << "连接已关闭" << std::endl;

        // 将连接返回到空闲连接池
        ConnectionMgr::instance().delete_connection(1);
        std::cout << "连接已返回到连接池" << std::endl;

        // 停止io_context
        io_context.stop();
        if (io_thread.joinable()) {
            io_thread.join();
        }

        std::cout << "\n=== 测试完成 ===" << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
