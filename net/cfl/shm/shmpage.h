#pragma once

#include <cstdint>
#include <ctime>
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include <system_error>
#include <optional>
#include <format>
#include <unordered_map>
#include "shm.h"

#if defined(_WIN32)

#include <windows.h>

#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
using HANDLE = int; // 非 Windows 下用 int 模拟
#endif
namespace cfl::shm {

/// @brief 每个共享内存块的元信息
    struct MemoryBlockHeader {
        std::int32_t index = 0;        ///< 数据块编号
        bool in_use = false;           ///< 是否正在使用
        bool is_new = false;           ///< 是否是新创建的块
        std::time_t before_time = 0;   ///< DS 服务器写入前的时间戳
        std::time_t after_time = 0;    ///< DS 服务器写入后的时间戳
    };

/// @brief 共享内存页
    struct SharedMemoryPage {
        char *raw_data = nullptr;   ///< 指向共享内存原始数据
        MemoryBlockHeader *block_headers = nullptr; ///< 块头数组起始地址
        std::optional<ShmHandle> handle{};             ///< 平台相关的共享内存句柄
    };

/// @brief 共享内存管理器
/// @details 负责多页共享内存的创建、分配、释放与追踪
    class SharedMemoryManager {
    public:
        /// @brief 构造函数：创建或附加到共享内存
        /// @param module_id 模块编号
        /// @param raw_block_size 每个原始块大小
        /// @param blocks_per_page 每页块数
        /// @param attach_only 如果为 true 则附加到已有共享内存，否则新建
        explicit SharedMemoryManager(std::int32_t module_id,
                            std::int32_t raw_block_size,
                            std::int32_t blocks_per_page,
                            bool attach_only = false);

        /// @brief 构造函数：直接从已有数据区域管理
//    SharedMemoryManager(std::int32_t raw_block_size,
//                        char* base_addr,
//                        std::int32_t length);

        virtual ~SharedMemoryManager();

    protected:
        using PageList = std::vector<SharedMemoryPage>;

        PageList pages_;            ///< 内存页集合
        std::int32_t blocks_per_page_;  ///< 每页容纳的块数
        std::int32_t page_count_;       ///< 页数量
        std::int32_t total_blocks_;     ///< 总块数
        std::int32_t block_size_;       ///< 每块字节大小
        std::int32_t raw_block_size_;   ///< 原始块大小（未对齐）
        std::int32_t module_id_;        ///< 模块编号
        bool empty_created_;    ///< 是否首次创建

        /// 映射表：所有块头
        using BlockMap = std::unordered_map<std::int32_t, MemoryBlockHeader *>;
        BlockMap block_map_;

        /// 映射表：已使用块
        using UsedBlockMap = std::unordered_map<void *, MemoryBlockHeader *>;
        UsedBlockMap used_blocks_;

        /// 映射表：空闲块
        using FreeBlockMap = std::unordered_map<std::int32_t, MemoryBlockHeader *>;
        FreeBlockMap free_blocks_;

    private:
        /// @brief 创建一个新页
        bool create_new_page();

        /// @brief 初始化页面数据区域（置零 + 初始化块头）
        void init_page(SharedMemoryPage &page);

    public:
        /// @brief 初始化映射表（逻辑服务器调用）
        void initialize_block_map();

        /// @brief 是否是首次创建共享内存
        bool is_first_created() const noexcept { return empty_created_; }

        /// @brief 从共享内存中恢复其他页信息
        void import_existing_pages();

        /// @brief 获取总块数
        std::int32_t total_count() const noexcept { return total_blocks_; }

        /// @brief 获取空闲块数量
        std::int32_t free_count() const noexcept { return static_cast<std::int32_t>(free_blocks_.size()); }

        /// @brief 获取已使用块数量
        std::int32_t used_count() const noexcept { return static_cast<std::int32_t>(used_blocks_.size()); }

        /// @brief 通过索引获取块头
        virtual MemoryBlockHeader *get_block_header(std::int32_t index);

        /// @brief 通过索引获取对象指针
        virtual class SharedObject *get_object(std::int32_t index);

        /// @brief 获取原始块大小
        std::int32_t raw_block_size() const noexcept { return raw_block_size_; }

        /// @brief 获取对齐后的块大小
        std::int32_t aligned_block_size() const noexcept { return block_size_; }

        /// @brief 处理已用区块中被释放的数据
        void clean_dirty_blocks();

        /// @brief 分配一个新对象
        /// @param new_block 是否标记为新建
        virtual SharedObject *allocate_object(bool new_block = false);

        /// @brief 释放一个对象
        virtual bool destroy_object(SharedObject *obj);

        /// @brief 获取已使用数据块映射
        UsedBlockMap &used_blocks() noexcept { return used_blocks_; }
    };

/// @brief 共享内存中的对象基类
/// @details 所有存入共享内存的对象必须继承该类
    class SharedObject {
    public:
        virtual ~SharedObject() = default;

        /// @brief 当分配时调用（相当于构造逻辑）
        virtual void on_create() {}

        /// @brief 当释放时调用（相当于析构逻辑）
        virtual void on_destroy() {}
    };
}