#pragma once
#include "cfl.h"
#include <cassert>
#include <cstring>
#include <mutex>

namespace cfl{

    constexpr std::size_t HEADER_LEN = 28;

    template <std::size_t Size>
    class BufferManager;

    template <std::size_t Size>
    class CFLDataBuffer : public DataBuffer {
    public:
        CFLDataBuffer()
                : data_len_(0), buffer_size_(Size), ref_count_(0),
                  prev_(nullptr), next_(nullptr), manager_(nullptr) {}

        ~CFLDataBuffer() override {
            data_len_ = 0;
            buffer_size_ = Size;
            ref_count_ = 0;
            prev_ = nullptr;
            next_ = nullptr;
            manager_ = nullptr;
        }

        bool add_ref() {
            std::scoped_lock lock(manager_->buffer_mutex_);
            ++ref_count_;
            return true;
        }

        bool release() {
            assert(manager_ != nullptr);
            manager_->release_buffer(this);
            return true;
        }

        std::span<const char> data() const override {
            return std::span<const char>(buffer_ + HEADER_LEN, data_len_ - HEADER_LEN);
        }

        int total_length() const override {
            return data_len_;
        }

        int body_length() const override {
            return data_len_ - static_cast<int>(HEADER_LEN);
        }

        void set_total_length(int length) override {
            data_len_ = length;
        }

        std::span<char> buffer() override {
            return std::span<char>(buffer_, buffer_size_);
        }


        int buffer_size() const override {
            return static_cast<int>(buffer_size_);
        }

        int copy_from(DataBuffer &src) override {
            std::memcpy(buffer_, src.buffer().data(), src.total_length());
            data_len_ = src.total_length();
            return data_len_;
        }

        int copy_to(char* dest, std::int32_t dest_len) override {
            if (dest_len < total_length()) {
                return 0;
            }
            std::memcpy(dest, buffer().data(), total_length());
            return total_length();
        }

        CFLDataBuffer<Size>* prev_;
        CFLDataBuffer<Size>* next_;
        BufferManager<Size>* manager_;

        int ref_count_;
        int buffer_size_;
        char buffer_[Size];
        int data_len_;
    };

    template <std::size_t Size>
    class BufferManager {
    public:
        BufferManager()
                : free_list_(nullptr), used_list_(nullptr),
                  buffer_count_(0), enable_pool_(true) {}

        ~BufferManager() {
            release_all();
        }

        DataBuffer* allocate_buffer() {
            std::scoped_lock lock(buffer_mutex_);
            CFLDataBuffer<Size>* buf = nullptr;
            if (!free_list_) {
                buf = new CFLDataBuffer<Size>();
                buf->manager_ = this;
            } else {
                buf = free_list_;
                free_list_ = free_list_->next_;
                if (free_list_) {
                    free_list_->prev_ = nullptr;
                }
                buf->next_ = nullptr;
                buf->prev_ = nullptr;
            }

            assert(buf->ref_count_ == 0);
            buf->ref_count_ = 1;

            if (!used_list_) {
                used_list_ = buf;
            } else {
                buf->next_ = used_list_;
                used_list_->prev_ = buf;
                buf->prev_ = nullptr;
                used_list_ = buf;
            }

            ++buffer_count_;
            return buf;
        }

        bool release_buffer(CFLDataBuffer<Size>* buf) {
            assert(buf != nullptr);
            if (!buf) return false;

            assert(buf->ref_count_ > 0);
            if (buf->ref_count_ <= 0) return false;

            std::scoped_lock lock(buffer_mutex_);
            --buf->ref_count_;

            if (buf->ref_count_ <= 0) {
                buf->data_len_ = 0;

                // 从已用链表移除
                if (used_list_ == buf) {
                    used_list_ = buf->next_;
                    if (used_list_) {
                        used_list_->prev_ = nullptr;
                    }
                } else {
                    assert(buf->prev_ != nullptr);
                    buf->prev_->next_ = buf->next_;
                    if (buf->next_) {
                        buf->next_->prev_ = buf->prev_;
                    }
                }

                if (enable_pool_) {
                    // 放回空闲链表
                    buf->next_ = free_list_;
                    buf->prev_ = nullptr;
                    free_list_ = buf;
                    if (buf->next_) {
                        buf->next_->prev_ = buf;
                    }
                } else {
                    delete buf;
                }
                --buffer_count_;
            }
            return true;
        }

        void release_all() {
            auto node = free_list_;
            while (node) {
                auto tmp = node;
                node = node->next_;
                delete tmp;
            }

            node = used_list_;
            while (node) {
                auto tmp = node;
                node = node->next_;
                delete tmp;
            }
        }

        void set_enable_pool(bool enable) {
            enable_pool_ = enable;
        }

    private:
        CFLDataBuffer<Size>* free_list_;
        CFLDataBuffer<Size>* used_list_;
        std::mutex buffer_mutex_;
        int buffer_count_;
        bool enable_pool_;
    };

    class BufferAllocator {
    public:
        BufferAllocator(){
            buffer_manager_any.set_enable_pool(false);
        }

        ~BufferAllocator() = default;
        static BufferAllocator& instance() {
            static BufferAllocator BufferAllocator;
            return BufferAllocator;
        }

        DataBuffer* allocate_buffer(int size);

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

        // 大内存直接 new/delete，不用池化
        BufferManager<10 * 1024 * 1024> buffer_manager_any;
    };

}
