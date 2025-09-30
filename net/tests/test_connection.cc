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
                auto sock_ptr = std::make_shared<asio::ip::tcp::socket>(std::move(socket));
                auto buffer = std::make_shared<std::array<char, 1024>>();

                sock_ptr->async_read_some(asio::buffer(*buffer),
                                          [sock_ptr, buffer](std::error_code ec, std::size_t length) {
                                              if (!ec) {
                                                  std::string received(buffer->data(), length);
                                                  std::cout << "Server 接收到: " << received << std::endl;

                                                  auto response = std::make_shared<std::string>("Echo: " + received);
                                                  asio::async_write(*sock_ptr, asio::buffer(*response),
                                                                    [sock_ptr, response](std::error_code ec, std::size_t) {
                                                                        if (!ec)
                                                                            std::cout << "Server 回执已发送" << std::endl;
                                                                    });
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
//        asio::io_context io_context2;

        // 初始化连接管理器
        ConnectionMgr::instance().init(io_context, 100);
        // 启动测试服务器
        TestServer server(io_context, 5000);
        std::cout << "测试服务器启动在端口 5000" << std::endl;

        // 在另一个线程运行io_context
        std::jthread io_thread([&io_context]() {
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

//        connection->start();
        std::cout << "成功获取连接对象" << std::endl;

        // 设置连接ID
//        connection->set_conn_id(1);
        std::cout << "设置连接ID为: " << connection->conn_id() << std::endl;

        // 连接到服务器
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "5000");

        asio::connect(connection->socket(), endpoints);
        std::cout << "成功连接到服务器" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        connection->start();
        // 发送测试消息
        std::string test_message = "Hello, Connection Test!";
        connection->send(test_message);
        std::cout << "发送消息: " << test_message << std::endl;

        // 等待一段时间让消息传输完成
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        // 测试关闭连接
        connection->close();
        std::cout << "连接已关闭" << std::endl;

        // 将连接返回到空闲连接池
        ConnectionMgr::instance().delete_connection(1);
        std::cout << "连接已返回到连接池" << std::endl;

        // 停止io_context
        io_context.stop();
//        if (io_thread.joinable()) {
//            io_thread.join();
//        }

        std::cout << "\n=== 测试完成 ===" << std::endl;

        // 让程序跑一会儿
//        std::this_thread::sleep_for(std::chrono::seconds(3));
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
