#pragma once

//#include "IBufferHandler.h"
//#include "Connection.h"
#include "connection.h"
#include <asio.hpp>
#include <string>
#include <memory>
#include <thread>

namespace cfl {
    class NetEngine {
    public:
        NetEngine()
                : io_context_(),
                  strand_(asio::make_strand(io_context_)) {}

        ~NetEngine() = default;

        NetEngine(const NetEngine &) = delete;

        NetEngine &operator=(const NetEngine &) = delete;

        static NetEngine &instance() {
            static NetEngine NetEngine;
            return NetEngine;
        }

        bool start(uint16_t port,
                   int32_t maxConn,
                   std::shared_ptr<DataHandler> bufferHandler, std::string &listenIp);

        bool stop();

        CFL_API bool send_message(uint64_t connId,
                          uint32_t msgId,
                          uint64_t targetId,
                          uint32_t userData,
                          std::span<const char> data);

        bool send_buffer(int32_t connId,
                         std::shared_ptr<DataBuffer> buffer);

        std::shared_ptr<Connection> connect_sync(const std::string &ip, uint16_t port);

        std::shared_ptr<Connection> connect_async(const std::string &ip, uint16_t port);

        bool enable_packet_check(bool enable);

        bool wait_for_connect();

        void set_check_enable(bool enable=true){
            packet_check_enabled_ = enable;
        }

    private:
        void handle_connect(std::shared_ptr<Connection> connection,
                            const asio::error_code &ec);

        void handle_accept(std::shared_ptr<Connection> connection,
                           const asio::error_code &ec);

    private:
        std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
        asio::io_context io_context_;
        std::jthread worker_thread_;  // 自动 join
        bool packet_check_enabled_ = false;
        asio::strand<asio::io_context::executor_type> strand_;

        std::shared_ptr<DataHandler> buffer_handler_ = nullptr;  // 可以考虑改成智能指针
    };
}