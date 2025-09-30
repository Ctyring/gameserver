#include "connection.h"
#include "cfl.h"
#include "buffer.h"

namespace cfl {
    Connection::Connection(asio::io_context &io_context)
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
                
                // todo: 通知ConnectionMgr
                if(self->data_handler_)
                    self->data_handler_->on_close_connect(self->conn_id());
            }
        });
    }

    bool Connection::check_header(const cfl::PacketHeader& header) {
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

// 从接收缓冲区中提取完整的网络数据包并交给上层处理
// 返回值：true 表示解析正常；false 表示遇到错误（例如非法包头），需要关闭连接
    bool Connection::extract_buffer() {
        // 如果当前没有数据，直接返回
        if (data_len_ == 0) {
            return true;
        }

        // buf_ptr 指向当前未处理的数据起始地址
        auto buf_ptr = read_buf_.data();
        // remain 表示当前剩余的有效字节数
        auto remain = data_len_;

        // 循环尝试解析多个完整数据包
        while (true) {
            // ============================================================
            // 1. 处理半包情况：之前已经收到部分数据，等待拼接完整
            // ============================================================
            if (data_buffer_ && data_buffer_->total_length() > 0) {
                auto already = data_buffer_->total_length();     // 已经存储的字节数
                auto need = expected_size_ - already;            // 还需要的字节数

                if (remain < need) {
                    // 数据不足以拼接完整 → 把所有数据先拷贝进去，等待下次
                    std::memcpy(data_buffer_->buffer().data() + already, buf_ptr, remain);
                    data_buffer_->set_total_length(already + remain);

                    data_len_ = 0;   // 当前缓冲区的数据都用掉了
                    return true;     // 等待下次接收数据
                } else {
                    // 数据足够补齐完整包
                    std::memcpy(data_buffer_->buffer().data() + already, buf_ptr, need);
                    data_buffer_->set_total_length(already + need);

                    buf_ptr += need;     // 向后移动指针
                    remain -= need;      // 剩余数据减少
                    data_len_ = remain;  // 更新全局剩余长度

                    // 通知上层：完整包已就绪
                    data_handler_->on_data_handle(data_buffer_, conn_id());

                    // 释放缓存，准备处理下一个包
                    data_buffer_.reset();
                    expected_size_ = 0;
                }
            }

            // ============================================================
            // 2. 数据不足一个包头，无法判断 → 等待更多数据
            // ============================================================
            if (remain < sizeof(PacketHeader)) {
                if (remain > 0 && std::byte(buf_ptr[0]) != std::byte{CODE_VALUE}) {
                    // 如果剩余数据 > 0，但第一个字节不是合法校验码 → 认为是非法数据
                    // 返回 false 表示需要关闭连接
                    return false;
                }
                break; // 缓存的数据不足包头大小，跳出循环，等待更多数据
            }

            // ============================================================
            // 3. 校验包头
            // ============================================================
            auto *header = reinterpret_cast<const PacketHeader *>(buf_ptr);

            if (!check_header(*header)) {
                // 包头校验失败（如长度非法、校验码错误等）
                spdlog::error("验证Header失败! ConnID:{} TargetID:{}", conn_id_, header->target_id);
                return false;
            }

            auto packet_size = static_cast<std::size_t>(header->size);

            // ============================================================
            // 4. 数据不足一个完整包 → 启动半包缓存
            // ============================================================
            if (packet_size > remain && packet_size < read_buf_.size()) {
                // 分配一个缓存区，大小为目标包大小
                data_buffer_ = BufferAllocator::instance().allocate_buffer(packet_size);
                // 把当前剩余的数据拷贝进去
                std::memcpy(data_buffer_->buffer().data(), buf_ptr, remain);
                data_buffer_->set_total_length(static_cast<std::uint32_t>(remain));

                // 保存期望的完整包大小
                expected_size_ = static_cast<std::int32_t>(packet_size);

                data_len_ = 0;  // 当前缓冲区用尽
                return true;    // 等待后续数据来拼接
            }

            // ============================================================
            // 5. 已经收到了完整包
            // ============================================================
            if (packet_size <= remain) {
                // 分配一个新 buffer 存放完整包
                auto buffer = std::make_shared<SimpleDataBuffer>(static_cast<std::int32_t>(packet_size));

                // 把完整的数据拷贝进去
                std::memcpy(buffer->buffer().data(), buf_ptr, packet_size);
                buffer->set_total_length(static_cast<std::int32_t>(packet_size));

                // 移动指针，减少剩余数据
                buf_ptr += packet_size;
                remain -= packet_size;
                data_len_ = remain;

                // 通知上层：完整包已就绪
                data_handler_->on_data_handle(buffer, conn_id());
            } else {
                // 包声明的大小超过缓冲区容量 → 认为是非法数据
                return false;
            }
        }

        // ============================================================
        // 6. 收尾：把剩余数据前移，确保 read_buf_ 中数据是连续的
        // ============================================================
        if (remain > 0) {
            std::memmove(read_buf_.data(), buf_ptr, remain);
        }
        data_len_ = remain;

        return true; // 解析成功
    }

}  // namespace cfl