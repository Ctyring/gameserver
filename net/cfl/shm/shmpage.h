/**
 * @file shmpage.h
 * @brief 跨平台共享内存管理器定义与实现
 *
 * @details
 * 本文件实现了一个跨平台的共享内存管理器 SharedMemoryManagerBase，
 * 支持在多进程/多模块间通过共享内存实现对象存储与管理。
 *
 * 功能点：
 * - 多页共享内存的创建、附加与释放
 * - 基于块的内存分配与回收
 * - 内存块元信息（使用状态、新建标记、时间戳）维护
 * - 跨平台支持（Windows / Linux）
 *
 * 使用场景：
 * - 游戏服务器进程间通信
 * - 数据共享与缓存
 * - 需要高性能共享内存对象池的场合
 */

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
#include "shmobj.h"

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>

#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/// @brief 在非 Windows 平台上用 int 模拟 HANDLE
using HANDLE = int;
/// @brief 在 Linux/Unix 上定义 INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (-1)
#endif

namespace cfl::shm {

/**
 * @brief 每个共享内存块的元信息
 */
    struct MemoryBlockHeader {
        std::size_t index = 0;        ///< 数据块编号
        bool in_use = false;          ///< 是否正在使用
        bool is_new = false;          ///< 是否是新创建的块
        std::time_t before_time = 0;  ///< DS 服务器写入前的时间戳
        std::time_t after_time = 0;   ///< DS 服务器写入后的时间戳
    };

/**
 * @brief 共享内存页
 */
    struct SharedMemoryPage {
        char *raw_data = nullptr;                  ///< 指向共享内存原始数据
        MemoryBlockHeader *block_headers = nullptr;///< 块头数组起始地址
        std::optional<ShmHandle> handle{};         ///< 平台相关的共享内存句柄

        SharedMemoryPage() = default;
        // 禁止拷贝构造/拷贝赋值
        SharedMemoryPage(const SharedMemoryPage&) = default;
        SharedMemoryPage& operator=(const SharedMemoryPage&) = default;
        // 允许移动（默认）
        SharedMemoryPage(SharedMemoryPage&&) noexcept = default;
        SharedMemoryPage& operator=(SharedMemoryPage&&) noexcept = default;
    };

/**
 * @brief 共享内存管理器
 *
 * @details
 * 管理多页共享内存，支持内存块的分配、释放与追踪。
 * 提供了基于索引的对象获取与共享内存扩展机制。
 */
    class SharedMemoryManagerBase {
    public:
        /**
         * @brief 构造函数：创建或附加到共享内存
         * @param module_id 模块编号
         * @param raw_block_size 每个原始块大小（字节）
         * @param blocks_per_page 每页的块数
         * @param attach_only 如果为 true 则附加到已有共享内存，否则新建
         */
        explicit SharedMemoryManagerBase(std::size_t module_id,
                                         std::size_t raw_block_size,
                                         std::size_t blocks_per_page,
                                         bool attach_only = false);

        /**
         * @brief 析构函数
         * @details 释放所有共享内存页，并关闭相关句柄
         */
        ~SharedMemoryManagerBase();

    protected:
        using PageList = std::vector<SharedMemoryPage>; ///< 页列表类型

        PageList pages_;                 ///< 内存页集合
        std::size_t blocks_per_page_;    ///< 每页容纳的块数
        std::size_t page_count_;         ///< 页数量
        std::size_t total_blocks_;       ///< 总块数
        std::size_t block_size_;         ///< 每块字节大小（含头部）
        std::size_t raw_block_size_;     ///< 原始块大小（未对齐）
        std::size_t module_id_;          ///< 模块编号
        bool empty_created_;             ///< 是否首次创建

        using BlockRef = std::reference_wrapper<MemoryBlockHeader>;

        /// 映射表：所有块头
        using BlockMap = std::unordered_map<std::size_t, MemoryBlockHeader*>;
        BlockMap block_map_;

        /// 映射表：已使用块
        using UsedBlockMap = std::unordered_map<void *, MemoryBlockHeader*>;
        UsedBlockMap used_blocks_;

        /// 映射表：空闲块
        using FreeBlockMap = std::unordered_map<std::size_t, MemoryBlockHeader*>;
        FreeBlockMap free_blocks_;

    private:
        /**
         * @brief 创建一个新页
         * @return 是否创建成功
         */
        bool create_new_page();

        /**
         * @brief 初始化页面数据区域
         * @details 将页面清零并初始化所有块头
         * @param page 待初始化的页面
         */
        void init_page(SharedMemoryPage &page);

    public:
        /**
         * @brief 初始化块映射表
         */
        void initialize_block_map();

        /**
         * @brief 是否是首次创建共享内存
         * @return true 表示首次创建
         */
        bool is_first_created() const noexcept { return empty_created_; }

        /**
         * @brief 从共享内存中恢复其他页信息
         */
        void import_existing_pages();

        /**
         * @brief 获取总块数
         */
        std::size_t total_count() const noexcept { return total_blocks_; }

        /**
         * @brief 获取空闲块数量
         */
        std::size_t free_count() const noexcept { return static_cast<std::size_t>(free_blocks_.size()); }

        /**
         * @brief 获取已使用块数量
         */
        std::size_t used_count() const noexcept { return static_cast<std::size_t>(used_blocks_.size()); }

        /**
         * @brief 通过索引获取块头
         * @param index 块索引
         * @return 指向块头的指针（可能为 nullptr）
         */
        MemoryBlockHeader *get_block_header(std::size_t index);

        /**
         * @brief 通过索引获取对象指针
         * @param index 块索引
         * @return 指向共享对象的指针（可能为 nullptr）
         */
        SharedObject *get_object(std::size_t index);

        /**
         * @brief 获取原始块大小
         */
        std::size_t raw_block_size() const noexcept { return raw_block_size_; }

        /**
         * @brief 获取对齐后的块大小
         */
        std::size_t aligned_block_size() const noexcept { return block_size_; }

        /**
         * @brief 清理已用区块中已释放的对象
         */
        void clean_dirty_blocks();

         /**
         * @brief 分配一个新对象
         * @param new_block 是否标记为新建
         * @return 分配的对象指针（可为空）
         */
         std::optional<SharedObject *> allocate_object(bool new_block = false);

        /**
         * @brief 释放一个对象
         * @param obj 待释放的对象指针
         * @return 是否释放成功
         */
        virtual bool destroy_object(SharedObject *obj);

        /**
         * @brief 获取已使用数据块映射
         */
        UsedBlockMap &used_blocks() noexcept { return used_blocks_; }
    };

    template<typename T>
    class SharedMemoryManager : public SharedMemoryManagerBase {
        public:
        SharedMemoryManager(std::size_t module_id,
                            std::size_t blocks_per_page,
                            bool attach_only = false) : SharedMemoryManagerBase(module_id, sizeof(T), blocks_per_page,
                                                                                attach_only) {}

/**
         * @brief 通过索引获取对象
         */
        T *get_object_by_index(std::int32_t index) {
            return static_cast<T *>(SharedMemoryManagerBase::get_object(static_cast<std::size_t>(index)));
        }

        /**
         * @brief 分配一个新对象
         */
        std::optional<T *> allocate_object(bool is_new_block) {
            auto obj_opt = SharedMemoryManagerBase::allocate_object(is_new_block);
//            auto obj_opt = std::optional<SharedObject *>(nullptr);
            if (!obj_opt.has_value() || obj_opt.value() == nullptr) {
                return std::nullopt;
            }
            T *obj = static_cast<T *>(obj_opt.value());
            // 在共享内存中原位构造对象
            if(is_new_block)
                new(obj) T();
            return std::make_optional(obj);
        }

        /**
         * @brief 获取块头信息
         */
        MemoryBlockHeader *get_block_header_by_index(std::int32_t index) {
            return SharedMemoryManagerBase::get_block_header(static_cast<std::size_t>(index));
        }

        /**
         * @brief 销毁对象
         */
        bool destroy_object(T *object) {
            if (object == nullptr) {
                return false;
            }
            object->~T(); // 显式调用析构
            return SharedMemoryManagerBase::destroy_object(reinterpret_cast<SharedObject *>(object));
        }

    };

} // namespace cfl::shm
