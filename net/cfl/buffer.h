#pragma once
#include "cfl.h"
#include <cassert>
#include <cstring>
#include <mutex>
#include <memory>
#include <span>

namespace cfl {

    /// 固定的协议头部长度
    constexpr std::size_t HEADER_LEN = 28;

    template <std::size_t Size>
    class BufferManager;

    /**
     * @brief 固定大小的数据缓冲区实现
     *
     * 该类继承自 DataBuffer，内部封装了一个大小为 `Size` 的原始缓冲区，
     * 并通过 `BufferManager` 管理对象的分配与释放。
     *
     * 特点：
     * - 支持引用计数（通过 `std::shared_ptr` 管理）。
     * - 内部维护双向链表指针（prev_/next_），由 BufferManager 管理。
     * - 提供 buffer() / data() / copy_from() / copy_to() 等接口。
     *
     * @tparam Size 缓冲区的大小（字节数）
     */
    template <std::size_t Size>
    class CFLDataBuffer : public DataBuffer,
                          public std::enable_shared_from_this<CFLDataBuffer<Size>> {
    public:
        /**
         * @brief 构造函数
         */
        CFLDataBuffer()
                : data_len_(0), buffer_size_(Size) {}

        /**
         * @brief 析构函数
         *
         * 会清理缓冲区数据长度，恢复默认 buffer_size_
         */
        ~CFLDataBuffer() override {
            data_len_ = 0;
            buffer_size_ = Size;
        }

        /**
         * @brief 释放缓冲区回到 BufferManager
         * @return true 释放成功，false 管理器不存在
         */
        bool release() {
            auto mgr = manager_.lock();
            if (!mgr) return false;
            // 使用 shared_from_this 确保传递的是 shared_ptr
            mgr->release_buffer(this->shared_from_this());
            return true;
        }

        /**
         * @brief 获取数据区的只读视图（去除协议头部）
         * @return std::span<const char> 指向数据区的只读 span
         */
        std::span<const char> data() const override {
            return std::span<const char>(buffer_ + HEADER_LEN, data_len_ - HEADER_LEN);
        }

        /**
         * @brief 获取总数据长度（包含头部）
         * @return 总长度
         */
        int total_length() const override { return data_len_; }

        /**
         * @brief 获取有效负载长度（不含头部）
         * @return 有效数据长度
         */
        int body_length() const override { return data_len_ - static_cast<int>(HEADER_LEN); }

        /**
         * @brief 设置总数据长度
         * @param length 总长度
         */
        void set_total_length(std::uint32_t length) override { data_len_ = length; }

        /**
         * @brief 获取整个缓冲区的可写 span
         * @return std::span<char>
         */
        std::span<char> buffer() override {
            return std::span<char>(buffer_, buffer_size_);
        }

        /**
         * @brief 获取缓冲区大小
         * @return 缓冲区大小
         */
        int buffer_size() const override {
            return static_cast<int>(buffer_size_);
        }

        /**
         * @brief 从另一个 DataBuffer 拷贝数据到当前缓冲区
         * @param src 数据源
         * @return 实际拷贝字节数
         */
        int copy_from(DataBuffer &src) override {
            std::memcpy(buffer_, src.buffer().data(), src.total_length());
            data_len_ = src.total_length();
            return data_len_;
        }

        /**
         * @brief 将当前缓冲区拷贝到外部内存
         * @param dest 目标指针
         * @param dest_len 目标缓冲区长度
         * @return 成功拷贝的字节数，若目标不足返回 0
         */
        int copy_to(char* dest, std::int32_t dest_len) override {
            if (dest_len < total_length()) {
                return 0;
            }
            std::memcpy(dest, buffer().data(), total_length());
            return total_length();
        }

        /// 链表指针（用于 BufferManager 管理）
        std::weak_ptr<CFLDataBuffer<Size>> prev_;   ///< 前向节点
        std::weak_ptr<CFLDataBuffer<Size>> next_;   ///< 后向节点
        std::weak_ptr<BufferManager<Size>> manager_;///< 所属 BufferManager

        int buffer_size_; ///< 缓冲区大小
        char buffer_[Size]; ///< 实际存储的缓冲区
        int data_len_; ///< 当前数据长度
    };

    /**
     * @brief 固定大小缓冲区的池化管理器
     *
     * - 内部维护 free_list_ 与 used_list_ 双向链表。
     * - 支持池化模式（复用内存），可通过 set_enable_pool 控制。
     * - 提供 allocate_buffer() / release_buffer() / release_all() 等接口。
     *
     * @tparam Size 缓冲区大小
     */
    template <std::size_t Size>
    class BufferManager : public std::enable_shared_from_this<BufferManager<Size>> {
    public:
        /**
         * @brief 构造函数
         */
        BufferManager()
                : buffer_count_(0), enable_pool_(true) {}

        /**
         * @brief 析构函数
         *
         * 会释放所有缓冲区引用。
         */
        ~BufferManager() {
            release_all();
        }

        /**
         * @brief 分配缓冲区
         * @return std::shared_ptr<DataBuffer> 智能指针包装的缓冲区
         */
        std::shared_ptr<DataBuffer> allocate_buffer() {
            std::scoped_lock lock(buffer_mutex_);
            std::shared_ptr<CFLDataBuffer<Size>> buf;

            if (!free_list_) {
                spdlog::info("[BufferManager] allocate buffer no free make new");
                // 没有空闲对象，新建
                buf = std::make_shared<CFLDataBuffer<Size>>();
                spdlog::info("[BufferManager] allocate buffer set manager");
                buf->manager_ = this->weak_from_this();
                spdlog::info("[BufferManager] allocate buffer p1 ok");
            } else {
                // 从空闲链表取出
                buf = free_list_;
                free_list_ = free_list_->next_.lock();
                if (free_list_) {
                    free_list_->prev_.reset();
                }
                buf->next_.reset();
                buf->prev_.reset();
            }

            // 加入已用链表
            if (!used_list_) {
                used_list_ = buf;
            } else {
                buf->next_ = used_list_;
                used_list_->prev_ = buf;
                used_list_ = buf;
            }

            ++buffer_count_;
            return buf;
        }

        /**
         * @brief 释放缓冲区
         * @param buf 待释放的缓冲区
         * @return true 释放成功，false buf 为空
         */
        bool release_buffer(std::shared_ptr<CFLDataBuffer<Size>> buf) {
            if (!buf) return false;

            std::scoped_lock lock(buffer_mutex_);
            buf->data_len_ = 0;

            // 从已用链表移除
            if (used_list_ == buf) {
                used_list_ = buf->next_.lock();
                if (used_list_) {
                    used_list_->prev_.reset();
                }
            } else {
                auto prev = buf->prev_.lock();
                if (prev) {
                    prev->next_ = buf->next_;
                }
                if (auto nxt = buf->next_.lock()) {
                    nxt->prev_ = prev;
                }
            }

            if (enable_pool_) {
                // 放回空闲链表
                buf->next_ = free_list_;
                buf->prev_.reset();
                free_list_ = buf;
                if (auto nxt = buf->next_.lock()) {
                    nxt->prev_ = buf;
                }
            }

            --buffer_count_;
            return true;
        }

        /**
         * @brief 释放所有缓冲区引用（清空链表）
         */
        void release_all() {
            free_list_.reset();
            used_list_.reset();
        }

        /**
         * @brief 设置是否启用对象池
         * @param enable true 启用，false 关闭（直接释放内存）
         */
        void set_enable_pool(bool enable) {
            enable_pool_ = enable;
        }

        std::mutex buffer_mutex_; ///< 缓冲区互斥锁

    private:
        std::shared_ptr<CFLDataBuffer<Size>> free_list_; ///< 空闲链表头
        std::shared_ptr<CFLDataBuffer<Size>> used_list_; ///< 已用链表头
        int buffer_count_; ///< 当前分配的缓冲区数量
        bool enable_pool_; ///< 是否启用池化
    };

    /**
     * @brief 通用缓冲区分配器（单例）
     *
     * 管理不同大小的 BufferManager，根据请求大小分配合适的缓冲区。
     * - 小缓冲区来自内存池
     * - 大缓冲区（>64KB）直接 new/delete
     */
    class BufferAllocator {
    public:
        /**
         * @brief 构造函数
         *
         * 默认禁用大内存池化。
         */
        BufferAllocator() {
            buffer_manager_any.set_enable_pool(false);
        }

        ~BufferAllocator() = default;

        /**
         * @brief 获取单例
         * @return BufferAllocator&
         */
        static BufferAllocator& instance() {
            static BufferAllocator BufferAllocator;
            return BufferAllocator;
        }

        /**
         * @brief 根据大小分配缓冲区
         * @param size 请求的大小
         * @return std::shared_ptr<DataBuffer>
         */
        std::shared_ptr<DataBuffer> allocate_buffer(int size);

        // 不同大小的缓冲区管理器
        BufferManager<64>      buffer_manager_64b;
        BufferManager<128>     buffer_manager_128b;
        BufferManager<256>     buffer_manager_256b;
        BufferManager<512>     buffer_manager_512b;
        BufferManager<1024>    buffer_manager_1k;
        BufferManager<2048>    buffer_manager_2k;
        BufferManager<4096>    buffer_manager_4k;
        BufferManager<8192>    buffer_manager_8k;
        BufferManager<16384>   buffer_manager_16k;
        BufferManager<32768>   buffer_manager_32k;
        BufferManager<65536>   buffer_manager_64k;

        /// 大内存直接 new/delete，不用池化
        BufferManager<10 * 1024 * 1024> buffer_manager_any;
    };

}
