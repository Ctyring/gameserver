/**
 * @file net_config.h
 * @brief 定义网络数据缓冲区、数据处理接口以及网络包分发相关类与结构。
 *
 * 本文件提供了基础的数据缓冲区接口（DataBuffer）、数据处理接口（DataHandler）、
 * 包分发接口（PacketDispatcher），以及简单的缓冲区实现（SimpleDataBuffer）。
 * 同时定义了网络包头（PacketHeader）和网络包（NetPacket）的数据结构。
 */

#pragma once
#pragma pack(push, 1)

#include "cfl.h"
#include <memory>
#include <span>

namespace cfl {

    /// 固定校验码
    const std::uint8_t CODE_VALUE = 0x12;

    /**
     * @struct PacketHeader
     * @brief 网络数据包头部结构。
     *
     * 用于标识和解析网络传输的消息。
     */
    struct PacketHeader {
        std::uint8_t check_code;   ///< 校验码
        std::uint32_t msg_id;      ///< 消息ID
        std::uint32_t size;        ///< 数据包大小（包含包头）
        std::uint32_t packet_id;   ///< 包序号
        std::uint64_t target_id;   ///< 目标对象ID
        std::uint32_t user_data;   ///< 用户自定义数据
    };

    struct NetPacket;

    /**
     * @class DataBuffer
     * @brief 网络数据缓冲区抽象接口。
     *
     * 提供对网络数据的存取、拷贝等通用操作。
     */
    class DataBuffer {
    public:
        virtual ~DataBuffer() = default;

        /**
         * @brief 获取只读数据段。
         * @return 数据的只读 span 视图。
         */
        virtual std::span<const char> data() const = 0;

        /**
         * @brief 获取数据总长度。
         * @return 数据长度（字节数）。
         */
        virtual std::int32_t total_length() const = 0;

        /**
         * @brief 设置数据总长度。
         * @param length 新的数据长度。
         */
        virtual void set_total_length(std::uint32_t length) = 0;

        /**
         * @brief 获取包体长度（不含包头）。
         * @return 包体长度。
         */
        virtual std::int32_t body_length() const = 0;

        /**
         * @brief 获取可写缓冲区。
         * @return 数据缓冲区的 span 视图。
         */
        virtual std::span<char> buffer() = 0;

        /**
         * @brief 获取缓冲区总大小。
         * @return 缓冲区大小。
         */
        virtual std::int32_t buffer_size() const = 0;

        /**
         * @brief 从另一个 DataBuffer 中拷贝数据。
         * @param src 源数据缓冲区。
         * @return 实际拷贝的字节数。
         */
        virtual std::int32_t copy_from(DataBuffer &src) = 0;

        /**
         * @brief 拷贝数据到目标内存。
         * @param dest 目标内存指针。
         * @param dest_len 目标缓冲区大小。
         * @return 实际拷贝的字节数。
         */
        virtual std::int32_t copy_to(char *dest, std::int32_t dest_len) = 0;
    };

    /**
     * @struct DataHandler
     * @brief 网络数据处理回调接口。
     *
     * 用户实现该接口以处理接收到的数据、连接的关闭和新连接的建立。
     */
    struct DataHandler {
        virtual ~DataHandler() = default;

        /**
         * @brief 处理接收到的数据。
         * @param data_buffer 数据缓冲区。
         * @param conn_id 连接ID。
         * @return true 表示处理成功，false 表示处理失败。
         */
        virtual bool on_data_handle(std::shared_ptr<DataBuffer> data_buffer, std::int32_t conn_id) = 0;

        /**
         * @brief 处理连接关闭事件。
         * @param conn_id 连接ID。
         * @return true 表示处理成功。
         */
        virtual bool on_close_connect(std::int32_t conn_id) = 0;

        /**
         * @brief 处理新连接事件。
         * @param conn_id 连接ID。
         * @return true 表示处理成功。
         */
        virtual bool on_new_connect(std::int32_t conn_id) = 0;
    };

    /**
     * @struct PacketDispatcher
     * @brief 网络包分发接口。
     *
     * 负责将接收到的 NetPacket 分发给对应的逻辑处理模块。
     */
    struct PacketDispatcher {
        virtual ~PacketDispatcher() = default;

        /**
         * @brief 分发网络包。
         * @param packet 网络包。
         * @return true 表示分发成功。
         */
        virtual bool dispatch_packet(const NetPacket &packet) = 0;

        /**
         * @brief 处理连接关闭事件。
         * @param conn_id 连接ID。
         * @return true 表示处理成功。
         */
        virtual bool on_close_connect(std::int32_t conn_id) = 0;

        /**
         * @brief 处理新连接事件。
         * @param conn_id 连接ID。
         * @return true 表示处理成功。
         */
        virtual bool on_new_connect(std::int32_t conn_id) = 0;

        /**
         * @brief 定时器回调（每秒调用一次）。
         * @return true 表示处理成功。
         */
        virtual bool on_second_timer() = 0;
    };

    /**
     * @struct NetPacket
     * @brief 网络包数据结构。
     *
     * 表示一个完整的逻辑消息，包含消息ID、连接ID和数据处理对象。
     */
    struct NetPacket {
        std::int32_t msg_id{0};                         ///< 消息ID
        std::int32_t conn_id{0};                        ///< 连接ID
        std::shared_ptr<DataHandler> data_buffer{nullptr}; ///< 数据处理对象

        /**
         * @brief 构造函数。
         * @param msg_id_ 消息ID。
         * @param data_buffer_ 数据处理对象。
         * @param conn_id_ 连接ID。
         */
        explicit NetPacket(std::int32_t msg_id_ = 0,
                           std::shared_ptr<DataHandler> data_buffer_ = nullptr,
                           std::int32_t conn_id_ = 0)
                : msg_id(msg_id_), data_buffer(std::move(data_buffer_)), conn_id(conn_id_) {}
    };

    /**
     * @class SimpleDataBuffer
     * @brief 一个基于 std::vector 的简单数据缓冲区实现，适用于 asio 等网络框架。
     */
    class SimpleDataBuffer : public DataBuffer {
    public:
        /**
         * @brief 构造函数。
         * @param size 缓冲区大小，默认 1024 字节。
         */
        explicit SimpleDataBuffer(std::int32_t size = 1024)
                : buffer_(size), total_length_(0), body_length_(0) {}

        [[nodiscard]] std::span<const char> data() const override {
            return {buffer_.data(), static_cast<size_t>(total_length_)};
        }

        [[nodiscard]] std::int32_t total_length() const override { return total_length_; }

        void set_total_length(std::uint32_t length) override { total_length_ = length; }

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
        std::vector<char> buffer_;   ///< 数据存储缓冲区
        std::int32_t total_length_;  ///< 当前数据总长度
        std::int32_t body_length_;   ///< 当前包体长度
    };
}

#pragma pack(pop)
