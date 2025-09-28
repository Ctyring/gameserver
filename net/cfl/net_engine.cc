#include "net_engine.h"
#include "connection.h"
#include "buffer.h"
namespace cfl {
    bool NetEngine::start(uint16_t port,
                          int32_t maxConn,
                          std::shared_ptr<DataHandler> bufferHandler,
                          std::string &listenIp) {
        if (bufferHandler == nullptr) {
            return false;
        }
        buffer_handler_ = bufferHandler;

        ConnectionMgr::instance().init(io_context_, maxConn);
        if (listenIp.empty() || listenIp.length() < 4) {
            listenIp = "0.0.0.0";
        }
        acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(io_context_,
                                                              asio::ip::tcp::endpoint(asio::ip::make_address(listenIp),
                                                                                      port));

        wait_for_connect();
        // todo: 改成service
        worker_thread_ = std::jthread([this]() {
            io_context_.run();
        });
        return true;
    }

    bool NetEngine::stop() {
        // todo: 改成service
        io_context_.stop();
        acceptor_->close();
        acceptor_.reset();
        worker_thread_ = {};

        ConnectionMgr::instance().close_all_connection();
        return true;
    }

    bool NetEngine::wait_for_connect() {
        auto conn = ConnectionMgr::instance().get_new_connection();
        if (conn == nullptr) {
            return false;
        }

        conn->set_data_handler(buffer_handler_);
        acceptor_->async_accept(conn->socket(),
                                std::bind(&NetEngine::handle_accept,
                                          this,
                                          conn,
                                          std::placeholders::_1));
    }

    std::shared_ptr<Connection> NetEngine::connect_sync(const std::string &ip, uint16_t port){
        // todo
        return nullptr;
    }

    std::shared_ptr<Connection> NetEngine::connect_async(const std::string &ip, uint16_t port){
        auto conn = ConnectionMgr::instance().get_new_connection();
        if (conn == nullptr) {
            return nullptr;
        }

        conn->set_data_handler(buffer_handler_);

        // 解析器
        // todo: 改成 service
        auto resolver = asio::ip::tcp::resolver(io_context_);
        // 异步解析地址
        asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(ip, std::to_string(port));
        // 异步连接
        asio::async_connect(
                conn->socket(),
                endpoints,
                [this, conn](const asio::error_code& ec, const asio::ip::tcp::endpoint&) {
                    this->handle_connect(conn, ec);
                }
        );

        return conn;
    }

    void NetEngine::handle_connect(std::shared_ptr<Connection> connection, const asio::error_code &ec) {
        if(!ec){
            connection->set_status(NetStatus::Connected);
            buffer_handler_->on_new_connect(connection->conn_id());
            connection->start();
        }
        else{
            connection->close();
        }
    }

    void NetEngine::handle_accept(std::shared_ptr<Connection> connection,
                                 const asio::error_code &ec) {
        if (ec) {
            return;
        }
        buffer_handler_->on_new_connect(connection->conn_id());
        connection->start();

        wait_for_connect();
    }

    bool NetEngine::send_buffer(int32_t connId, std::shared_ptr<DataBuffer> buffer) {
        auto conn = ConnectionMgr::instance().get_connection(connId);
        if(conn == nullptr){
            return false;
        }

        if(conn->status() != NetStatus::Connected){
            spdlog::error("connId:{} is not connected", connId);
            return false;
        }

        conn->send(buffer->data().data());
        return true;
    }

    bool NetEngine::send_message(uint64_t connId, uint32_t msgId, uint64_t targetId, uint32_t userData,
                                 std::span<const char> data) {
        auto conn = ConnectionMgr::instance().get_connection(connId);
        if(conn == nullptr){
            spdlog::error("connId:{} is not exist", connId);
            return false;
        }

        if(conn->status() != NetStatus::Connected){
            spdlog::error("connId:{} is not connected", connId);
            return false;
        }

        auto buffer = BufferAllocator::instance().allocate_buffer(data.size() + sizeof(PacketHeader));
        if(buffer == nullptr){
            spdlog::error("allocate buffer failed");
            return false;
        }

        auto* header = reinterpret_cast<PacketHeader*>(buffer->buffer().data());
        header->check_code = CODE_VALUE;
        header->user_data  = userData;
        header->target_id  = targetId;
        header->size       = static_cast<uint32_t>(data.size() + sizeof(PacketHeader));
        header->msg_id     = msgId;
        header->packet_id  = 1;

        // 拷贝数据到 header 之后
        std::memcpy(buffer->buffer().data() + sizeof(PacketHeader), data.data(), data.size());

        buffer->set_total_length(header->size);

        conn->send(buffer->data().data());
        return true;
    }
}