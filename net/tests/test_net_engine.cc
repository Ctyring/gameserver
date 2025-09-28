#include "cfl/net_engine.h"
#include "cfl/connection.h"
#include "cfl/buffer.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

using namespace cfl;

// 一个简单的 DataHandler 测试实现
class TestHandler : public DataHandler {
public:
    bool on_new_connect(int32_t connId) override {
        spdlog::info("New connection established: {}", connId);
        return true;
    }

    bool on_close_connect(int32_t connId) override {
        spdlog::info("Connection closed: {}", connId);
        return true;
    }

    bool on_data_handle(std::shared_ptr<DataBuffer> data_buffer, std::int32_t conn_id){
        spdlog::info("Received data from connection: {}", conn_id);
        return true;
    }
};

int main() {
    // 设置控制台为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    spdlog::set_level(spdlog::level::debug);

    auto handler = std::make_shared<TestHandler>();

    // 启动服务端
    std::string ip = "127.0.0.1";
    uint16_t port = 5555;
    NetEngine::instance().start(port, 100, handler, ip);

    // 客户端异步连接
    auto clientConn = NetEngine::instance().connect_async("127.0.0.1", port);

    if (!clientConn) {
        spdlog::error("Failed to create client connection");
        return -1;
    }

    // 等待连接建立
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 发送一条消息
    std::string msg = "Hello NetEngine!";
    NetEngine::instance().send_message(
            clientConn->conn_id(),
            1001,       // msgId
            42,         // targetId
            0,          // userData
            std::span<const char>(msg.data(), msg.size())
    );

    // 让程序跑一会儿
    std::this_thread::sleep_for(std::chrono::seconds(3));

    NetEngine::instance().stop();
    return 0;
}
