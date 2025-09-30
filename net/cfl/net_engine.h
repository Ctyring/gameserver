#pragma once

#include "connection.h"
#include <asio.hpp>
#include <string>
#include <memory>
#include <thread>

namespace cfl {

/**
 * @brief 网络引擎类，基于 Asio 实现 TCP 网络通信。
 *
 * NetEngine 提供启动/停止服务器、发送消息、连接远程服务器等功能，
 * 并使用单例模式管理实例。线程安全的操作通过 strand 实现。
 */
    class NetEngine {
    public:
        /**
         * @brief 构造函数
         *
         * 初始化 io_context 和 strand。
         */
        NetEngine()
                : io_context_(),
                  strand_(asio::make_strand(io_context_)) {}

        /**
         * @brief 默认析构函数
         */
        ~NetEngine() = default;

        /**
         * @brief 删除拷贝构造函数
         */
        NetEngine(const NetEngine &) = delete;

        /**
         * @brief 删除拷贝赋值运算符
         */
        NetEngine &operator=(const NetEngine &) = delete;

        /**
         * @brief 获取 NetEngine 单例实例
         * @return NetEngine& 单例实例引用
         */
        static NetEngine &instance() {
            static NetEngine NetEngine;
            return NetEngine;
        }

        /**
         * @brief 启动网络引擎（服务器模式）
         * @param port 监听端口
         * @param maxConn 最大连接数
         * @param bufferHandler 数据处理回调
         * @param listenIp 监听的 IP 地址
         * @return true 启动成功
         * @return false 启动失败
         */
        bool start(uint16_t port,
                   int32_t maxConn,
                   std::shared_ptr<DataHandler> bufferHandler, std::string &listenIp);

        /**
         * @brief 停止网络引擎
         * @return true 停止成功
         * @return false 停止失败
         */
        bool stop();

        /**
         * @brief 发送消息给指定连接
         * @param connId 连接 ID
         * @param msgId 消息 ID
         * @param targetId 目标对象 ID
         * @param userData 用户自定义数据
         * @param data 消息内容
         * @return true 发送成功
         * @return false 发送失败
         */
        bool send_message(uint64_t connId,
                          uint32_t msgId,
                          uint64_t targetId,
                          uint32_t userData,
                          std::span<const char> data);

        /**
         * @brief 发送缓冲区数据
         * @param connId 连接 ID
         * @param buffer 数据缓冲区
         * @return true 发送成功
         * @return false 发送失败
         */
        bool send_buffer(int32_t connId,
                         std::shared_ptr<DataBuffer> buffer);

        /**
         * @brief 同步连接到远程服务器
         * @param ip 服务器 IP
         * @param port 服务器端口
         * @return std::shared_ptr<Connection> 成功返回 Connection 对象，否则返回 nullptr
         */
        std::shared_ptr<Connection> connect_sync(const std::string &ip, uint16_t port);

        /**
         * @brief 异步连接到远程服务器
         * @param ip 服务器 IP
         * @param port 服务器端口
         * @return std::shared_ptr<Connection> 返回 Connection 对象
         */
        std::shared_ptr<Connection> connect_async(const std::string &ip, uint16_t port);

        /**
         * @brief 启用或禁用数据包检查
         * @param enable 是否启用
         * @return true 操作成功
         * @return false 操作失败
         */
        bool enable_packet_check(bool enable);

        /**
         * @brief 等待连接完成（阻塞）
         * @return true 连接成功
         * @return false 连接失败
         */
        bool wait_for_connect();

        /**
         * @brief 设置是否启用数据包检查
         * @param enable 是否启用（默认 true）
         */
        void set_check_enable(bool enable = true) {
            packet_check_enabled_ = enable;
        }

    private:
        /**
         * @brief 连接回调处理
         * @param connection 连接对象
         * @param ec Asio 错误码
         */
        void handle_connect(std::shared_ptr<Connection> connection,
                            const asio::error_code &ec);

        /**
         * @brief 接受连接回调处理
         * @param connection 连接对象
         * @param ec Asio 错误码
         */
        void handle_accept(std::shared_ptr<Connection> connection,
                           const asio::error_code &ec);

    private:
        /// TCP 监听器
        std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;

        /// IO 上下文
        asio::io_context io_context_;

        /// 工作线程（自动 join）
        std::jthread worker_thread_;

        /// 是否启用数据包检查
        bool packet_check_enabled_ = false;

        /// Asio strand 保证线程安全
        asio::strand<asio::io_context::executor_type> strand_;

        /// 数据处理回调
        std::shared_ptr<DataHandler> buffer_handler_ = nullptr;
    };

} // namespace cfl
