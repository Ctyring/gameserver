#pragma once

#include "db/db_mysql.h"
#include "shm/obj/global_data_obj.h"
#include <cstdint>
#include <memory>
#include <mutex>

namespace cfl {
    using namespace shm;

/**
 * @class GlobalDataManager
 * @brief 全局数据管理器
 *
 * 该类用于管理全局游戏或系统数据，包括：
 * - 从数据库加载全局数据；
 * - 生成全局唯一 GUID；
 * - 记录并维护最大在线人数；
 * - 维护额外的全局扩展数据。
 *
 * 采用线程安全的单例模式，保证多线程环境下全局数据一致性。
 *
 * @note
 * 典型使用方式：
 * @code
 * auto& mgr = GlobalDataManager::instance();
 * mgr.loadData();
 * auto guid = mgr.makeNewGuid();
 * mgr.setMaxOnline(500);
 * @endcode
 */
    class GlobalDataManager {
    public:
        /**
         * @brief 获取全局单例实例。
         *
         * @return GlobalDataManager 的唯一实例引用。
         *
         * @threadsafe
         * 采用 C++11 静态局部变量初始化特性，保证线程安全。
         */
        static GlobalDataManager &instance() noexcept {
            static GlobalDataManager instance;
            return instance;
        }

        /**
         * @brief 禁止拷贝构造。
         */
        GlobalDataManager(const GlobalDataManager &) = delete;

        /**
         * @brief 禁止拷贝赋值。
         */
        GlobalDataManager &operator=(const GlobalDataManager &) = delete;

        /**
         * @brief 禁止移动构造。
         */
        GlobalDataManager(GlobalDataManager &&) = delete;

        /**
         * @brief 禁止移动赋值。
         */
        GlobalDataManager &operator=(GlobalDataManager &&) = delete;

        /**
         * @brief 从数据库加载全局数据。
         *
         * 该函数通常在系统初始化阶段调用，从数据库表中加载全局配置信息或统计数据，
         * 并初始化内部的 `GlobalDataObject`。
         *
         * @return 加载成功返回 `true`，否则返回 `false`。
         */
        bool load_data();

        /**
         * @brief 生成新的全局唯一 GUID。
         *
         * 根据内部的全局计数器生成递增的唯一标识符，
         * 可用于角色、物品、任务等对象的唯一标识。
         *
         * @return 新生成的 64 位全局唯一 ID。
         *
         * @threadsafe
         */
        [[nodiscard]] std::uint64_t make_new_guid();

        /**
         * @brief 设置最大在线人数。
         *
         * 在玩家在线数量发生变化时更新该值，用于记录系统历史最大在线人数。
         *
         * @param num 当前在线人数。
         *
         * @threadsafe
         */
        void set_max_online(std::int32_t num) noexcept;

        /**
         * @brief 获取记录的最大在线人数。
         *
         * @return 最大在线人数。
         *
         * @threadsafe
         */
        [[nodiscard]] std::int32_t get_max_online() const noexcept;

        /**
         * @brief 设置扩展全局数据。
         *
         * 全局数据对象中包含多个备用整型槽位，可通过索引设置自定义扩展值。
         *
         * @param index 扩展数据索引（通常范围 0 ~ MAX_EXTRA_INDEX）。
         * @param value 要设置的整数值。
         *
         * @return 设置成功返回 `true`，若索引非法或对象未初始化则返回 `false`。
         *
         * @threadsafe
         */
        bool set_extra_data(std::int32_t index, std::int32_t value);

        /**
         * @brief 获取扩展全局数据。
         *
         * @param index 扩展数据索引。
         * @return 对应索引的整数值；若索引非法或未初始化则返回默认值（通常为 0）。
         *
         * @threadsafe
         */
        [[nodiscard]] std::int32_t get_extra_data(std::int32_t index) const;

    private:
        /**
         * @brief 构造函数（私有）。
         *
         * 初始化为空状态，需调用 loadData() 完成加载。
         */
        GlobalDataManager(){
            load_data();
        }

        /**
         * @brief 析构函数（私有）。
         *
         * 自动释放持有的全局数据对象。
         */
        ~GlobalDataManager() = default;

    private:
        /**
         * @brief 全局数据对象指针。
         *
         * 存储全局运行时数据，如 GUID 计数器、最大在线人数、扩展数据等。
         * 使用智能指针确保生命周期安全。
         */
        std::shared_ptr<GlobalDataObject> global_data_ = nullptr;

        /**
         * @brief 线程互斥锁。
         *
         * 保证在多线程读写全局数据时的同步性。
         */
        mutable std::mutex mutex_;
    };

} // namespace cfl
