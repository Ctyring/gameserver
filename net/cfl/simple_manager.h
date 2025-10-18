#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>
#include "db/db_mysql.h"

namespace cfl {

    /**
     * @brief 存储玩家基础信息的数据结构。
     *
     * 用于保存角色（Role）的核心元数据，如 ID、账号、职业、等级、战斗力、登录/登出时间等。
     * 通常作为 SimpleManager 的管理对象。
     */
    struct SimpleInfo {
        std::uint64_t role_id = 0;       ///< 角色唯一ID
        std::uint64_t account_id = 0;    ///< 所属账号ID
        std::uint64_t guild_id = 0;      ///< 公会ID
        std::uint32_t career_id = 0;     ///< 职业ID
        std::uint32_t level = 0;         ///< 等级
        std::uint32_t vip_level = 0;     ///< VIP等级
        std::uint64_t fight_value = 0;   ///< 战斗力
        std::uint64_t logoff_time = 0;   ///< 最近登出时间（Unix时间戳）
        std::uint64_t logon_time = 0;    ///< 最近登录时间（Unix时间戳）
        std::uint64_t create_time = 0;   ///< 创建时间（Unix时间戳）
        std::string name;                ///< 角色名
        bool is_deleted = false;         ///< 删除标志（true 表示角色被删除）
        std::uint32_t logon_status = 0;  ///< 登录状态（0: 正常, 1: 从数据库加载中）

        SimpleInfo() = default;
    };

    /**
     * @brief 管理所有角色基础信息（SimpleInfo）的单例类。
     *
     * 该类负责：
     * - 从数据库加载玩家信息
     * - 根据 ID 或名称查询角色
     * - 创建和修改角色基础数据
     * - 检查角色名合法性、唯一性
     *
     * @note 使用单例模式访问：`SimpleManager::instance()`
     */
    class SimpleManager {
    public:
        /**
         * @brief 获取 SimpleManager 的全局唯一实例。
         * @return SimpleManager& 单例引用。
         */
        [[nodiscard]] static SimpleManager &instance() noexcept;

        /**
         * @brief 根据角色ID获取角色信息。
         * @param id 角色ID。
         * @return SimpleInfo* 指向角色信息的指针；若不存在则返回 nullptr。
         */
        [[nodiscard]] SimpleInfo *get_simple_info_by_id(std::uint64_t id) noexcept;

        /**
         * @brief 创建一个新的角色信息对象并插入管理器。
         * @param roleId 角色ID。
         * @param accountId 账号ID。
         * @param name 角色名。
         * @param careerId 职业ID。
         * @return SimpleInfo* 指向新建的 SimpleInfo 对象。
         */
        [[nodiscard]] SimpleInfo *create_simple_info(std::uint64_t roleId,
                                                   std::uint64_t accountId,
                                                   std::string_view name,
                                                   std::uint32_t careerId);

        /**
         * @brief 将一个外部构建的 SimpleInfo 对象加入管理器。
         * @param info 智能指针包装的 SimpleInfo 对象。
         * @return true 插入成功；false 插入失败（例如 info 为 nullptr）。
         */
        bool add_simple_info(std::unique_ptr<SimpleInfo> info);

        /**
         * @brief 从数据库加载所有玩家数据。
         * @return true 加载成功；false 加载失败。
         */
        bool load_data();

        /**
         * @brief 通过角色名获取角色ID。
         * @param name 角色名。
         * @return 角色ID；若未找到则返回 0。
         */
        [[nodiscard]] std::uint64_t get_role_id_by_name(std::string_view name) const noexcept;

        /**
         * @brief 获取角色创建时间。
         * @param id 角色ID。
         * @return 创建时间戳；若不存在则返回 0。
         */
        [[nodiscard]] std::uint64_t get_create_time(std::uint64_t id) const noexcept;

        /**
         * @brief 获取角色上次登录时间。
         * @param id 角色ID。
         * @return 登录时间戳；若不存在则返回 0。
         */
        [[nodiscard]] std::uint64_t get_logon_time(std::uint64_t id) const noexcept;

        /**
         * @brief 获取角色上次登出时间。
         * @param id 角色ID。
         * @return 登出时间戳；若不存在则返回 0。
         */
        [[nodiscard]] std::uint64_t get_logoff_time(std::uint64_t id) const noexcept;

        /**
         * @brief 设置角色创建时间。
         * @param id 角色ID。
         * @param time 创建时间戳。
         * @return true 设置成功；false 角色不存在。
         */
        bool set_create_time(std::uint64_t id, std::uint64_t time) noexcept;

        /**
         * @brief 设置角色登录时间。
         * @param id 角色ID。
         * @param time 登录时间戳。
         * @return true 设置成功；false 角色不存在。
         */
        bool set_logon_time(std::uint64_t id, std::uint64_t time) noexcept;

        /**
         * @brief 设置角色登出时间。
         * @param id 角色ID。
         * @param time 登出时间戳。
         * @return true 设置成功；false 角色不存在。
         */
        bool set_logoff_time(std::uint64_t id, std::uint64_t time) noexcept;

        /**
         * @brief 获取角色当前战斗力。
         * @param id 角色ID。
         * @return 战斗力数值；若不存在则返回 0。
         */
        [[nodiscard]] std::uint64_t get_fight_value(std::uint64_t id) const noexcept;

        /**
         * @brief 设置角色战斗力与等级。
         * @param id 角色ID。
         * @param value 新的战斗力。
         * @param level 新的等级。
         * @return true 设置成功；false 角色不存在。
         */
        bool set_fight_value(std::uint64_t id, std::uint64_t value, std::uint32_t level) noexcept;

        /**
         * @brief 修改角色名称。
         * @param id 角色ID。
         * @param name 新角色名。
         * @return true 设置成功；false 失败（例如重名）。
         */
        bool set_name(std::uint64_t id, std::string_view name);

        /**
         * @brief 设置角色VIP等级。
         * @param id 角色ID。
         * @param vipLevel 新VIP等级。
         * @return true 设置成功；false 角色不存在。
         */
        bool set_vip_level(std::uint64_t id, std::uint32_t vipLevel) noexcept;

        /**
         * @brief 设置角色所属公会ID。
         * @param id 角色ID。
         * @param guildId 公会ID。
         * @return true 设置成功；false 角色不存在。
         */
        bool set_guild_id(std::uint64_t id, std::uint64_t guildId) noexcept;

        /**
         * @brief 标记角色是否被删除。
         * @param id 角色ID。
         * @param deleted true 表示角色被删除。
         * @return true 设置成功；false 角色不存在。
         */
        bool set_role_deleted(std::uint64_t id, bool deleted) noexcept;

        /**
         * @brief 检查某角色名是否已存在。
         * @param name 角色名。
         * @return true 名称存在；false 名称未被使用。
         */
        [[nodiscard]] bool check_name_exist(std::string_view name) const noexcept;

        /**
         * @brief 检查角色名格式是否合法。
         * @param name 角色名。
         * @return true 合法；false 含非法字符或长度不符合要求。
         *
         * @details 要求：
         * - 长度在 [4, 20] 字符之间。
         * - 不包含 `,;'\" \\%%\r\n` 等非法字符。
         */
        [[nodiscard]] bool check_name_format(std::string_view name) const noexcept;

        /**
         * @brief 获取角色所属公会ID。
         * @param id 角色ID。
         * @return 公会ID；若不存在则返回 0。
         */
        [[nodiscard]] std::uint64_t get_guild_id(std::uint64_t id) const noexcept;

        /**
         * @brief 获取当前管理的角色总数量。
         * @return 角色总数。
         */
        [[nodiscard]] std::uint32_t get_total_count() const noexcept;

        /**
         * @brief 通过账号ID获取该账号下的所有角色ID。
         * @param accountId 账号ID。
         * @param roleIds 输出参数，用于存储角色ID列表。
         * @return true 查询成功。
         */
        bool get_role_ids_by_account_id(std::uint64_t accountId, std::vector<std::uint64_t> &roleIds) const;

    private:
        /**
         * @brief 构造函数（私有，单例模式）。
         */
        SimpleManager() = default;

        /**
         * @brief 析构函数（私有，单例模式）。
         */
        ~SimpleManager() = default;

        /// 禁止拷贝构造。
        SimpleManager(const SimpleManager &) = delete;

        /// 禁止赋值操作。
        SimpleManager &operator=(const SimpleManager &) = delete;

        /**
         * @brief 角色ID到角色信息的映射。
         */
        std::unordered_map<std::uint64_t, std::unique_ptr<SimpleInfo>> id_to_info_;

        /**
         * @brief 角色名到角色ID的映射。
         */
        std::unordered_map<std::string, std::uint64_t> name_to_id_;
    };

} // namespace cfl
