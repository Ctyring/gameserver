#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include "spdlog/spdlog.h"
#include "shmpage.h"
#include "cfl/config.h"
#include "cfl/cfl.h"
/**
 * @brief 共享内存对象池管理器
 *
 * 该单例类用于管理共享内存对象池，提供初始化、释放、
 * 恢复以及获取共享内存对象池的功能。
 */
namespace cfl::shm {
    enum class SHMTYPE : size_t {
        RoleData,
        End,
    };
    using SharedMemoryManagerBasePtr = std::shared_ptr<SharedMemoryManagerBase>;
//    using SharedMemoryManagerBasePtr = SharedMemoryManagerBase*;

    class DataPoolManager {
    public:
        /**
         * @brief 获取 DataPoolManager 的单例实例
         * @return 指向 DataPoolManager 实例的指针
         */
        static DataPoolManager& instance();

        /**
         * @brief 初始化共享内存对象池
         * @return true 表示初始化成功，false 表示失败
         */
        bool init();

        /**
         * @brief 释放共享内存对象池
         * @return true 表示释放成功，false 表示失败
         */
        bool release();

        /**
         * @brief 从共享内存恢复对象池状态
         * @return true 表示恢复成功，false 表示失败
         */
        bool restore_from_shared_memory();

        /**
         * @brief 根据索引获取共享内存对象池
         * @param index 共享内存池索引
         * @return 指向共享内存基类的指针，若失败则返回 nullptr
         */
        SharedMemoryManagerBasePtr get_shared_pool(SHMTYPE index);

    private:
        /// @brief 私有构造函数，确保单例模式
        DataPoolManager() = default;

        /// @brief 私有析构函数
        ~DataPoolManager() = default;

        /// @brief 禁止拷贝构造
        DataPoolManager(const DataPoolManager &) = delete;

        /// @brief 禁止赋值操作
        DataPoolManager &operator=(const DataPoolManager &) = delete;

    private:
        /// @brief 存放共享内存对象池的数组
        std::vector<SharedMemoryManagerBasePtr> data_object_pools_;

        /// @brief 每页共享内存大小（字节）
        std::uint32_t shared_page_size_ = 0;
    };

    /**
     * @brief 在共享内存池中创建一个对象
     *
     * @tparam T 要创建的对象类型
     * @param index 共享内存池索引
     * @param allocate_new 是否分配新的内存块（默认为 true）
     * @return 指向创建对象的指针，若失败则返回 nullptr
     */
    template<class T>
    std::shared_ptr<T> create_object(SHMTYPE index, bool allocate_new = true) {
        SharedMemoryManagerBasePtr ssm = DataPoolManager::instance().get_shared_pool(index);
        if (!ssm) {
            spdlog::error("CreateObject 错误: SharedMemoryBase 为空");
            return nullptr;
        }
        auto x = static_cast<T*>(ssm->allocate_object(allocate_new).value());
        std::shared_ptr<T> object(x,
                [](T* p) {
                    if (p) {
                        p->~T(); // 调用析构函数，但不释放内存，因为共享内存的分配由 pool 管理
                    }
                }
        );

        if (!object) {
            spdlog::get("sys")->error("CreateObject 错误, 原因: %s", cfl::shm::get_last_error_str(cfl::shm::get_last_error()).c_str());
        }
        return object;
    }
}
