/**
 * @file Connection.h
 * @brief 定义网络连接类 (Connection) 和连接管理单例 (ConnectionMgr)
 *
 * 该文件基于 Asio 实现 TCP 连接的管理，包括读写、连接状态维护以及连接池管理。
 */

#pragma once

#include <asio.hpp>
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include "cfl.h"

namespace cfl {

/**
 * @enum NetStatus
 * @brief 网络连接状态枚举
 */
    enum class NetStatus {
        Init,        ///< 初始状态，尚未使用
        Connecting,  ///< 正在连接
        Connected,   ///< 已连接
        Closing,     ///< 正在关闭
        Closed       ///< 已关闭
    };

/**
 * @class Connection
 * @brief 表示单个 TCP 连接对象，支持异步读写
 *
 * Connection 类包装了 Asio 的 socket，对数据读写进行封装，并通过 strand 保证线程安全。
 * 另外支持连接状态、连接 ID 和用户数据的管理。
 */
    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        using tcp = asio::ip::tcp;

        /**
         * @brief 构造函数
         * @param io_context Asio 的 io_context 引用
         */
        explicit Connection(asio::io_context &io_context);

        /**
         * @brief 析构函数
         */
        ~Connection();

        /**
         * @brief 获取底层 socket 引用
         * @return socket 引用
         */
        tcp::socket &socket() { return socket_; }

        /**
         * @brief 启动连接，设置状态为 Connected，并开始异步读取
         */
        void start() {
            status_ = NetStatus::Connected;
            do_read();
        }

        /**
         * @brief 从缓冲区提取完整数据包
         * @return 提取是否成功
         */
        bool extract_buffer();

        /**
         * @brief 关闭读写，调用 socket::shutdown
         * @return 成功返回 true，失败返回 false
         */
        bool shutdown() {
            asio::error_code ec;
            socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            if (ec) {
                spdlog::error("shutdown error: {}", ec.message());
                return false;
            }
            return true;
        }

        /**
         * @brief 异步发送数据
         * @param msg 要发送的字符串消息
         *
         * 消息将被投递到 strand，保证多线程安全；内部维护一个发送队列。
         */
        void send(const std::string &msg) {
            asio::post(strand_, [self = shared_from_this(), msg]() {
                bool writing = !self->send_queue_.empty();
                self->send_queue_.push(msg);
                if (!writing) {
                    self->do_write();
                }
            });
        }

        /**
         * @brief 主动关闭连接
         */
        void close();

        /**
         * @brief 校验数据包头
         * @param header 包头
         * @return 包头是否合法
         */
        bool check_header(const PacketHeader header);

        /**
         * @brief 设置当前连接状态
         * @param status 新的连接状态
         */
         void set_status(NetStatus status) { status_.store(status); }

        /**
         * @brief 获取当前连接状态
         * @return NetStatus
         */
        NetStatus status() const { return status_; }

        /**
         * @brief 设置连接 ID
         * @param id 连接 ID
         */
        void set_conn_id(std::uint64_t id) { conn_id_ = id; }

        /**
         * @brief 获取连接 ID
         * @return 连接 ID
         */
        std::uint64_t conn_id() const { return conn_id_; }

        /**
         * @brief 设置连接绑定的数据
         * @param data 用户自定义数据
         */
        void set_conn_data(std::uint64_t data) { conn_data_ = data; }

        /**
         * @brief 获取连接绑定的数据
         * @return 用户自定义数据
         */
        std::uint64_t conn_data() const { return conn_data_; }

        /**
         * @brief 重置连接对象，清空缓存与状态
         */
        void reset() {
            conn_id_ = 0;
            conn_data_ = 0;
            status_.store(NetStatus::Init);
            expected_size_ = 0;
            data_len_ = 0;
            data_buffer_.reset();
            while (!send_queue_.empty()) send_queue_.pop();
        }

        void set_data_handler(std::shared_ptr<DataHandler> handler) {
            data_handler_ = std::move(handler);
        }

    private:
        /**
         * @brief 内部实现：异步读取数据
         */
        void do_read();

        /**
         * @brief 内部实现：异步写入数据
         */
        void do_write();

    private:
        tcp::socket socket_;   ///< 底层 TCP socket
        asio::strand<asio::io_context::executor_type> strand_; ///< strand，保证并发安全
        std::array<char, 8192> read_buf_; ///< 读缓冲区
        std::queue<std::string> send_queue_; ///< 待发送的消息队列
        std::atomic<NetStatus> status_{NetStatus::Init}; ///< 连接状态 todo: 优化内存序
//        std::vector<std::byte> cur_packet_; ///< 半包缓存
        std::size_t expected_size_{0}; ///< 当前包期望大小
        std::uint64_t conn_id_{0}; ///< 连接 ID
        std::uint64_t conn_data_{0}; ///< 连接绑定的数据
        std::size_t data_len_{0}; ///< 当前缓冲区有效数据长度
        bool packet_number_check{true}; ///< 是否启用包序号检查
        std::int32_t check_number{0}; ///< 包序号
        std::shared_ptr<DataHandler> data_handler_; ///< 数据处理监听
        std::shared_ptr<SimpleDataBuffer> data_buffer_;///< 数据缓冲
    };

/**
 * @class ConnectionMgr
 * @brief 连接管理器（单例模式）
 *
 * 提供连接池功能，支持初始化、分配、查找和回收连接。
 */
    class ConnectionMgr {
    public:
        using ConnectionPtr = std::shared_ptr<Connection>;

        /**
         * @brief 获取单例实例
         * @return ConnectionMgr 引用
         */
        static ConnectionMgr &instance() {
            static ConnectionMgr inst;
            return inst;
        }

        /**
         * @brief 初始化连接池
         * @param io_context Asio io_context
         * @param max_connections 最大连接数，默认 10000
         */
        void init(asio::io_context &io_context,
                  std::uint64_t max_connections = 10000) {
            std::scoped_lock lock(mutex_);
            io_context_ = &io_context;
            max_connections_ = max_connections;
            free_connections_ = std::vector<ConnectionPtr>(max_connections_);
            for (auto &c: free_connections_) {
                c = std::make_shared<Connection>(*io_context_);
            }
            spdlog::info("Init {}", free_connections_.size());
        }

        /**
         * @brief 获取一个新连接（从池中分配）
         * @return ConnectionPtr，新连接指针；如果池空则返回 nullptr
         */
        ConnectionPtr get_new_connection() {
            std::scoped_lock lock(mutex_);
            if (free_connections_.empty()) {
                spdlog::error("no free connection");
                return nullptr;
            }
            auto conn = free_connections_.back();
            free_connections_.pop_back();
            use_connections_.emplace(cur_conn_id_, conn);
            conn->set_conn_id(cur_conn_id_++);
            return conn;
        }

        /**
         * @brief 根据连接 ID 获取连接
         * @param conn_id 连接 ID
         * @return ConnectionPtr，如果未找到返回 nullptr
         */
        ConnectionPtr get_connection(std::uint64_t conn_id) {
            std::scoped_lock lock(mutex_);
            auto it = use_connections_.find(conn_id);
            if (it == use_connections_.end()) {
                for(auto &c: use_connections_){
                    spdlog::info("conn_id: {}, conn_id: {}", c.first, c.second->conn_id());
                }
                spdlog::error("no connection found");
                return nullptr;
            }
            return it->second;
        }

        /**
         * @brief 删除指定连接并回收到连接池
         * @param conn_id 连接 ID
         * @return 是否成功删除
         */
        bool delete_connection(std::uint64_t conn_id) {
            std::scoped_lock lock(mutex_);
            auto it = use_connections_.find(conn_id);
            if (it == use_connections_.end()) {
                spdlog::error("no connection found");
                return false;
            }
            it->second->reset();
            free_connections_.push_back(it->second);
            use_connections_.erase(it);
            return true;
        }

        bool close_all_connection() {
            std::scoped_lock lock(mutex_);
            for (auto &c: use_connections_) {
                c.second->close();
                c.second->reset();
            }
            return true;
        }

    public:
        ConnectionMgr() = default; ///< 默认构造函数

        std::vector<ConnectionPtr> free_connections_; ///< 空闲连接池
        std::unordered_map<std::uint64_t, ConnectionPtr> use_connections_; ///< 使用中的连接
        std::mutex mutex_; ///< 互斥锁，保护连接池
        std::uint64_t max_connections_{10000}; ///< 最大连接数
        std::uint64_t cur_conn_id_{1}; ///< 当前分配的连接 ID
        asio::io_context *io_context_{nullptr}; ///< io_context 指针
    };

}  // namespace cfl
