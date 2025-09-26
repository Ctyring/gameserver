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
// 连接状态
enum class NetStatus { Init, Connecting, Connected, Closing, Closed };

// 单个连接对象
class Connection : public std::enable_shared_from_this<Connection> {
   public:
    using tcp = asio::ip::tcp;

    explicit Connection(asio::io_context& io_context);
    ~Connection();

    tcp::socket& socket() { return socket_; }

    void start() {
        //            status_ = NetStatus::Connected;
        //            do_read();
    }

    bool extract_buffer();

    bool shutdown() {
        asio::error_code ec;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) {
            spdlog::error("shutdown error: {}", ec.message());
            return false;
        }
        return true;
    }

    void send(const std::string& msg) {
        // 投递到 strand，避免多线程并发写
        asio::post(strand_, [self = shared_from_this(), msg]() {
            bool writing = !self->send_queue_.empty();
            self->send_queue_.push(msg);
            if (!writing) {
                self->do_write();
            }
        });
    }
    void close();
    bool check_header(const PacketHeader header);
    NetStatus status() const { return status_; }
    void set_conn_id(std::uint64_t id) { conn_id_ = id; }
    std::uint64_t conn_id() const { return conn_id_; }
    void set_conn_data(std::uint64_t data) { conn_data_ = data; }
    std::uint64_t conn_data() const { return conn_data_; }

   private:
    void do_read();
    void do_write();

   private:
    tcp::socket socket_;
    asio::strand<asio::io_context::executor_type> strand_;
    std::array<char, 8192> read_buf_;
    std::queue<std::string> send_queue_;
    std::atomic<NetStatus> status_{NetStatus::Init};
    // 半包缓存
    std::vector<std::byte> cur_packet_;
    std::size_t expected_size_{0};
    std::uint64_t conn_id_{0};
    std::uint64_t conn_data_{0};
    std::size_t data_len_{0};
    bool packet_number_check{true};
    std::int32_t check_number{0};
};

// 管理连接的单例
class ConnectionMgr {
   public:
    using ConnectionPtr = std::shared_ptr<Connection>;

    static ConnectionMgr& instance() {
        static ConnectionMgr inst;
        return inst;
    }

    void init(asio::io_context& io_context,
              std::uint64_t max_connections = 10000) {
        std::scoped_lock lock(mutex_);
        io_context_ = &io_context;
        max_connections_ = max_connections;
        free_connections_ = std::vector<ConnectionPtr>(max_connections_);
        for (auto& c : free_connections_) {
            c = std::make_shared<Connection>(*io_context_);
        }
        spdlog::info("Init {}", free_connections_.size());
    }

    ConnectionPtr get_new_connection() {
        std::scoped_lock lock(mutex_);
        if(free_connections_.empty()){
            spdlog::error("no free connection");
            return nullptr;
        }
        auto conn = free_connections_.back();
        free_connections_.pop_back();
        use_connections_.emplace(cur_conn_id_++, conn);
        return conn;
    }

    ConnectionPtr get_connection(std::uint64_t conn_id) {
        std::scoped_lock lock(mutex_);
        auto it = use_connections_.find(conn_id);
        if (it == use_connections_.end()) {
            spdlog::error("no connection found");
            return nullptr;
        }
        return it->second;
    }

    bool delete_connection(std::uint64_t conn_id) {
        std::scoped_lock lock(mutex_);
        auto it = use_connections_.find(conn_id);
        if (it == use_connections_.end()) {
            spdlog::error("no connection found");
            return false;
        }
        free_connections_.push_back(it->second);
        use_connections_.erase(it);
        return true;
    }

public:
    ConnectionMgr() = default;

    std::vector<ConnectionPtr> free_connections_;
    std::unordered_map<std::uint64_t, ConnectionPtr> use_connections_;
    std::mutex mutex_;
    std::uint64_t max_connections_{10000};
    std::uint64_t cur_conn_id_{1};
    asio::io_context* io_context_{nullptr};
};
}  // namespace cfl