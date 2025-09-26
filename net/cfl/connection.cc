#include "connection.h"

namespace cfl {
Connection::Connection(asio::io_context& io_context)
    : socket_(io_context), strand_(asio::make_strand(io_context)) {
    // todo: 初始化一些参数
}

Connection::~Connection() {}

void Connection::do_read() {
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(read_buf_),
        asio::bind_executor(
            strand_, [self](std::error_code ec, std::size_t length) {
                if (!self->extract_buffer()) {
                    self->close();
                    return;
                }
                if (!ec && length > 0) {
                    std::string data(self->read_buf_.data(), length);
                    std::cout << "[Recv] " << data << std::endl;
                    self->do_read();
                } else {
                    self->close();
                }
            }));
}

void Connection::do_write() {
    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(send_queue_.front()),
        asio::bind_executor(strand_,
                            [self](std::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    self->send_queue_.pop();
                                    if (!self->send_queue_.empty()) {
                                        self->do_write();
                                    }
                                } else {
                                    self->close();
                                }
                            }));
}

void Connection::close() {
    asio::post(strand_, [self = shared_from_this()]() {
        if (self->status_ != NetStatus::Closed) {
            self->status_ = NetStatus::Closing;
            asio::error_code ec;
            self->socket_.close(ec);
            self->status_ = NetStatus::Closed;

            // todo: 通知service
            // todo: 通知ConnectionMgr
        }
    });
}

bool Connection::check_header(const cfl::PacketHeader header) {
    if (header.check_code != CODE_VALUE) [[unlikely]] {
        spdlog::info("验证Header失败! check_code error ConnID:{} TargetID:{}",
                     conn_id_, header.target_id);
        return false;
    }

    constexpr int32_t kMaxPacketSize = 1024 * 1024;
    if (header.size <= 0 || header.size > kMaxPacketSize) [[unlikely]] {
        spdlog::info("验证Header失败! size error ConnID:{} TargetID:{}",
                     conn_id_, header.target_id);
        return false;
    }

    if (header.msg_id <= 0 || header.msg_id > 399999) [[unlikely]] {
        spdlog::info("验证Header失败! msg_id error ConnID:{} TargetID:{}",
                     conn_id_, header.target_id);
        return false;
    }

    if (packet_number_check) [[likely]] {
        const int32_t packetCheckNo =
            header.packet_id - (header.msg_id ^ header.size);

        if (packetCheckNo <= 0) [[unlikely]] {
            spdlog::info("验证Header失败! checkNo error ConnID:{} TargetID:{}",
                         conn_id_, header.target_id);
            return false;
        }

        if (check_number == 0 || check_number == packetCheckNo) [[likely]] {
            return true;
        }

        spdlog::info("验证Header失败! checkNo error ConnID:{} TargetID:{}",
                     conn_id_, header.target_id);
        return false;
    }

    return true;
}

bool Connection::extract_buffer() {
    if (data_len_ == 0) {
        return true;
    }

    auto buf_ptr = read_buf_.data();
    auto remain = data_len_;

    while (true) {
        // 1. 补齐半包
        if (!cur_packet_.empty()) {
            auto already = cur_packet_.size();
            auto need = expected_size_ - already;
            if (remain < need) {
                // 数据不够，先攒起来
                cur_packet_.insert(
                    cur_packet_.end(), reinterpret_cast<std::byte*>(buf_ptr),
                    reinterpret_cast<std::byte*>(buf_ptr + remain));
                data_len_ = 0;
                return true;
            } else {
                // 可以补齐完整包
                cur_packet_.insert(
                    cur_packet_.end(), reinterpret_cast<std::byte*>(buf_ptr),
                    reinterpret_cast<std::byte*>(buf_ptr + need));
                buf_ptr += need;
                remain -= need;
                data_len_ = remain;

                // todo: 处理完整包 告诉service
                //                    on_message(cur_packet_);
                cur_packet_.clear();
                expected_size_ = 0;
            }
        }

        // 2. 不够一个包头
        if (remain < sizeof(PacketHeader)) {
            if (remain > 0 && std::byte(buf_ptr[0]) != std::byte{CODE_VALUE}) {
                // 首字节校验失败 → 返回 false 触发关闭
                return false;
            }
            // 等待更多数据
            break;
        }

        auto* header = reinterpret_cast<const PacketHeader*>(buf_ptr);

        if (!check_header(*header)) {
            spdlog::error("验证Header失败! ConnID:{} TargetID:{}", conn_id_,
                          header->target_id);
            return false;
        }

        auto packet_size = static_cast<std::size_t>(header->size);

        // 4. 数据不足一个完整包，先缓存
        if (packet_size > remain && packet_size < read_buf_.size()) {
            cur_packet_.assign(reinterpret_cast<std::byte*>(buf_ptr),
                               reinterpret_cast<std::byte*>(buf_ptr + remain));
            expected_size_ = packet_size;
            data_len_ = 0;
            return true;
        }

        // 5. 有完整包
        if (packet_size <= remain) {
            std::vector<std::byte> packet(
                reinterpret_cast<std::byte*>(buf_ptr),
                reinterpret_cast<std::byte*>(buf_ptr + packet_size));
            buf_ptr += packet_size;
            remain -= packet_size;
            data_len_ = remain;

            //                on_message(packet); // todo: 告诉service
        } else {
            // 包太大或非法
            return false;
        }
    }

    // 6. 收尾：把剩余数据前移
    if (remain > 0) {
        std::memmove(read_buf_.data(), buf_ptr, remain);
    }
    data_len_ = remain;

    return true;
}

}  // namespace cfl