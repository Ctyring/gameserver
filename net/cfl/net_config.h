#pragma once

#pragma pack(push, 1)

#include "cfl.h"
#include <memory>
#include <span>

namespace cfl {
    const std::uint8_t CODE_VALUE = 0x12;
    struct PacketHeader {
        std::uint8_t check_code;
        std::uint32_t msg_id;
        std::uint32_t size;
        std::uint32_t packet_id;
        std::uint64_t target_id;
        std::uint32_t user_data;
    };

    struct NetPacket;

    class DataBuffer {
    public:
        virtual ~DataBuffer() = default;

        // 数据访问
        virtual std::span<const char> data() const = 0;

        virtual std::int32_t total_length() const = 0;

        virtual void set_total_length(std::int32_t length) = 0;

        virtual std::int32_t body_length() const = 0;

        virtual std::span<char> buffer() = 0;

        virtual std::int32_t buffer_size() const = 0;

        // 数据拷贝
        virtual std::int32_t copy_from(DataBuffer &src) = 0;

        virtual std::int32_t copy_to(char *dest, std::int32_t dest_len) = 0;
    };

// 网络数据处理回调接口
    struct DataHandler {
        virtual ~DataHandler() = default;

        virtual bool on_data_handle(std::shared_ptr<DataBuffer> data_buffer, std::int32_t conn_id) = 0;

        virtual bool on_close_connect(std::int32_t conn_id) = 0;

        virtual bool on_new_connect(std::int32_t conn_id) = 0;
    };

// 网络包分发接口
    struct PacketDispatcher {
        virtual ~PacketDispatcher() = default;

        virtual bool dispatch_packet(const NetPacket &packet) = 0;

        virtual bool on_close_connect(std::int32_t conn_id) = 0;

        virtual bool on_new_connect(std::int32_t conn_id) = 0;

        virtual bool on_second_timer() = 0;
    };

// 网络包数据结构
    struct NetPacket {
        std::int32_t msg_id{0};
        std::int32_t conn_id{0};
        std::shared_ptr<DataHandler> data_buffer{nullptr};

        explicit NetPacket(std::int32_t msg_id_ = 0,
                   std::shared_ptr<DataHandler> data_buffer_ = nullptr,
                   std::int32_t conn_id_ = 0)
                : msg_id(msg_id_), data_buffer(std::move(data_buffer_)), conn_id(conn_id_) {}
    };

// 一个asio友好的i_data_buffer实现
    class SimpleDataBuffer : public DataBuffer {
    public:
        explicit SimpleDataBuffer(std::int32_t size = 1024)
                : buffer_(size), total_length_(0), body_length_(0) {}

        [[nodiscard]] std::span<const char> data() const override {
            return {buffer_.data(), static_cast<size_t>(total_length_)};
        }


        [[nodiscard]] std::int32_t total_length() const override { return total_length_; }

        void set_total_length(std::int32_t length) override { total_length_ = length; }

        [[nodiscard]] std::int32_t body_length() const override { return body_length_; }

        std::span<char> buffer() override { return buffer_; }

        [[nodiscard]] std::int32_t buffer_size() const override { return static_cast<std::int32_t>(buffer_.size()); }

        std::int32_t copy_from(DataBuffer &src) override {
            std::int32_t len = std::min(buffer_size(), src.total_length());
            std::copy(src.data().begin(), src.data().begin() + len, buffer_.begin());
            total_length_ = len;
            return len;
        }

        std::int32_t copy_to(char *dest, std::int32_t dest_len) override {
            std::int32_t len = std::min(dest_len, total_length_);
            std::copy(buffer_.begin(), buffer_.begin() + len, dest);
            return len;
        }

    private:
        std::vector<char> buffer_;
        std::int32_t total_length_;
        std::int32_t body_length_;
    };
}
#pragma pack(pop)