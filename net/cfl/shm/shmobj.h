#pragma once
#include <chrono>
#include <cstdint>
#include <atomic>
#include <string_view>

namespace cfl::shm {

    /**
     * @enum ObjectState
     * @brief 表示共享对象的生命周期状态。
     */
    enum class ObjectState : std::uint8_t {
        Idle,       /**< 空闲状态，未被占用 */
        Locked,     /**< 被占用状态，通常表示正在被写入/修改 */
        Released,   /**< 已释放状态，不再被使用 */
        Destroyed,  /**< 已销毁状态，不可再使用 */
        InUse       /**< 使用中状态，表示活跃的引用 */
    };

    /**
     * @class SharedObject
     * @brief 表示一个可放置于共享内存中的对象，具有生命周期状态管理和元信息。
     *
     * 本类提供了线程安全的状态管理 API，并记录最后一次状态更新的时间点。
     * 对象不可拷贝，但可以安全存储于共享内存区域。
     */
    class SharedObject {
    public:
        /**
         * @brief 构造函数。
         * @details 初始化对象为 Idle 状态，检查码为 0，记录当前时间。
         */
        SharedObject() noexcept
                : check_code_{0},
                  state_{ObjectState::Idle},
                  last_update_{std::chrono::system_clock::now()} {}

        /// @name 禁止拷贝
        /// @{
        SharedObject(const SharedObject&) = delete;                 ///< 禁止拷贝构造
        SharedObject& operator=(const SharedObject&) = delete;      ///< 禁止拷贝赋值
        /// @}

        // ---------- 状态变更 API ----------
        /**
         * @brief 将对象标记为被占用 (Locked)。
         */
        void lock() noexcept { set_state(ObjectState::Locked); }

        /**
         * @brief 将对象标记为空闲 (Idle)。
         */
        void unlock() noexcept { set_state(ObjectState::Idle); }

        /**
         * @brief 将对象标记为已释放 (Released)。
         */
        void release() noexcept { set_state(ObjectState::Released); }

        /**
         * @brief 将对象标记为已销毁 (Destroyed)。
         */
        void destroy() noexcept { set_state(ObjectState::Destroyed); }

        /**
         * @brief 将对象标记为使用中 (InUse)。
         */
        void use() noexcept { set_state(ObjectState::InUse); }

        /**
         * @brief 将对象重置为空闲 (Idle)。
         */
        void reset() noexcept { set_state(ObjectState::Idle); }

        // ---------- 状态查询 API ----------
        /**
         * @brief 判断对象是否被占用。
         * @return true 如果对象处于 Locked 状态，否则返回 false。
         */
        [[nodiscard]] bool is_locked() const noexcept { return state() == ObjectState::Locked; }

        /**
         * @brief 判断对象是否已销毁。
         * @return true 如果对象处于 Destroyed 状态，否则返回 false。
         */
        [[nodiscard]] bool is_destroyed() const noexcept { return state() == ObjectState::Destroyed; }

        /**
         * @brief 判断对象是否已释放。
         * @return true 如果对象处于 Released 状态，否则返回 false。
         */
        [[nodiscard]] bool is_released() const noexcept { return state() == ObjectState::Released; }

        /**
         * @brief 判断对象是否处于使用中。
         * @return true 如果对象处于 InUse 状态，否则返回 false。
         */
        [[nodiscard]] bool is_in_use() const noexcept { return state() == ObjectState::InUse; }

        // ---------- 元信息 ----------
        /**
         * @brief 获取对象最后一次更新状态的时间点。
         * @return 时间点 (std::chrono::system_clock::time_point)。
         */
        [[nodiscard]] std::chrono::system_clock::time_point last_update_time() const noexcept {
            return last_update_;
        }

        /**
         * @brief 获取对象当前的状态。
         * @return 当前状态 (ObjectState)。
         */
        [[nodiscard]] ObjectState state() const noexcept {
            return state_.load(std::memory_order_acquire);
        }

        /**
         * @brief 获取对象的检查码。
         * @return 检查码 (int32_t)。
         */
        [[nodiscard]] std::size_t check_code() const noexcept {
            return check_code_;
        }

        /**
         * @brief 设置对象的检查码。
         * @param code 新的检查码。
         */
        void set_check_code(std::size_t code) noexcept {
            check_code_ = code;
        }

    private:
        /**
         * @brief 设置对象状态，并更新最后修改时间。
         * @param new_state 新的对象状态。
         */
        void set_state(ObjectState new_state) noexcept {
            state_.store(new_state, std::memory_order_release);
            last_update_ = std::chrono::system_clock::now();
        }

        std::size_t check_code_;   ///< 对象的检查码，用于完整性校验
        std::atomic<ObjectState> state_;  ///< 对象的生命周期状态，使用原子类型保证线程安全
        std::chrono::system_clock::time_point last_update_; ///< 最近一次状态更新的时间点
    };

} // namespace shm
