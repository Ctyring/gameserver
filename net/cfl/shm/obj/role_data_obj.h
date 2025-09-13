#pragma once

#include <string>
#include <array>
#include <cstdint>
#include <ranges>
#include "cfl/cfl.h"
#include "cfl/db/db.h"
#include "cfl/db/db_mysql.h"
#include "cfl/db/db_sqlite.h"

namespace cfl::shm {

    /**
     * @brief 角色数据对象，用于共享内存存储与数据库交互
     *
     * @details
     * RoleDataObject 是游戏中角色的核心数据结构，既用于服务器共享内存保存，
     * 也支持与 MySQL 数据库交互（保存、更新、删除）。
     * 包含了角色的基础信息、体力、经验、VIP、时间戳等关键数据。
     */
    struct RoleDataObject : public SharedObject {
        // ================= 基础数据 =================
        uint64_t roleId{0};                 ///< 角色ID（主键）
        uint64_t accountId{0};              ///< 账号ID
        char name[64];                      ///< 角色名
        int32_t carrerId{0};                ///< 职业ID
        int32_t level{0};                   ///< 等级
        std::array<int64_t, cfl::ActionNum> action{};   ///< 体力
        std::array<int64_t, cfl::ActionNum> actime{};   ///< 体力恢复时间
        int64_t exp{0};                     ///< 经验值
        int32_t langId{0};                  ///< 语言ID
        int64_t fightValue{0};              ///< 战斗力
        int32_t vipLevel{0};                ///< VIP等级
        int32_t vipExp{0};                  ///< VIP经验
        int32_t cityCopyId{0};              ///< 副本ID
        int32_t channel{0};                 ///< 渠道号
        bool isDeleted{false};              ///< 是否已删除标志位
        int64_t qq{0};                      ///< QQ号
        uint64_t createTime{0};             ///< 创建时间
        uint64_t logonTime{0};              ///< 上线时间
        uint64_t logoffTime{0};             ///< 下线时间
        uint64_t groupMailTime{0};          ///< 群邮件时间戳
        uint64_t guildId{0};                ///< 公会ID
        uint32_t onlineTime{0};             ///< 在线时长（秒）

        // ================= 签到数据 =================
        int32_t signNum{0};                 ///< 累计签到次数
        uint32_t signDay{0};                ///< 签到日期
        uint32_t recvAction{0};             ///< 已领取体力奖励

        // ================= 构造/析构函数 =================
        RoleDataObject() = default;
        RoleDataObject(const RoleDataObject &) = delete;
        RoleDataObject &operator=(const RoleDataObject &) = delete;
        RoleDataObject(RoleDataObject &&) noexcept = default;
        RoleDataObject &operator=(RoleDataObject &&) noexcept = default;
        ~RoleDataObject() = default;

        // ================= 数据库接口 =================

        /**
         * @brief 保存角色数据（插入或替换）
         *
         * @details 使用 MySQL 的 `REPLACE INTO`，如果记录已存在则替换，否则插入新记录。
         *
         * @return true 保存成功
         * @return false 保存失败
         */
        [[nodiscard]]
        bool Save() {
            int ret = cfl::db::MySQLUtil::execute_prepared(
                    "gameserver",
                    "REPLACE INTO role "
                    "(id, accountid, name, carrerid, level, citycopyid, exp, langid, viplevel, vipexp, "
                    "action1, action2, action3, action4, actime1, actime2, actime3, actime4, "
                    "createtime, logontime, logofftime, grouptime, fightvalue, guildid) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                    "?, ?, ?, ?, ?, ?, ?, ?, "
                    "?, ?, ?, ?, ?, ?)",
                    roleId,
                    accountId,
                    std::string_view(name),
                    carrerId,
                    level,
                    cityCopyId,
                    exp,
                    langId,
                    vipLevel,
                    vipExp,
                    action[0], action[1], action[2], action[3],
                    actime[0], actime[1], actime[2], actime[3],
                    createTime,
                    logonTime,
                    logoffTime,
                    groupMailTime,
                    fightValue,
                    guildId
            );
            return ret >= 0;
        }

        /**
         * @brief 保存角色数据到SQLite（插入或替换）
         *
         * @details 使用 SQLite 的 `INSERT OR REPLACE`，如果记录已存在则替换，否则插入新记录。
         *
         * @return true 保存成功
         * @return false 保存失败
         */
        [[nodiscard]]
        bool SaveSQLite() {
            int ret = cfl::db::SQLiteUtil::execute_prepared(
                    "gameserver",
                    "INSERT OR REPLACE INTO role "
                    "(id, accountid, name, carrerid, level, citycopyid, exp, langid, viplevel, vipexp, "
                    "action1, action2, action3, action4, actime1, actime2, actime3, actime4, "
                    "createtime, logontime, logofftime, grouptime, fightvalue, guildid) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                    "?, ?, ?, ?, ?, ?, ?, ?, "
                    "?, ?, ?, ?, ?, ?)",
                    roleId,
                    accountId,
                    std::string_view(name),
                    carrerId,
                    level,
                    cityCopyId,
                    exp,
                    langId,
                    vipLevel,
                    vipExp,
                    action[0], action[1], action[2], action[3],
                    actime[0], actime[1], actime[2], actime[3],
                    createTime,
                    logonTime,
                    logoffTime,
                    groupMailTime,
                    fightValue,
                    guildId
            );
            return ret >= 0;
        }

        /**
         * @brief 更新角色数据（只修改已有记录，不插入新记录）
         *
         * @details 使用 MySQL 的 `UPDATE` 语句，根据 `id` 更新已有记录。
         * 与 Save 的区别在于：Save 使用 REPLACE INTO，Update 使用 UPDATE。
         *
         * @return true 更新成功
         * @return false 更新失败
         */
        [[nodiscard]]
        bool Update() {
            int ret = cfl::db::MySQLUtil::execute_prepared(
                    "gameserver",
                    "UPDATE role SET "
                    "accountid=?, name=?, carrerid=?, level=?, citycopyid=?, exp=?, langid=?, viplevel=?, vipexp=?, "
                    "action1=?, action2=?, action3=?, action4=?, "
                    "actime1=?, actime2=?, actime3=?, actime4=?, "
                    "createtime=?, logontime=?, logofftime=?, grouptime=?, fightvalue=?, guildid=? "
                    "WHERE id=?",
                    accountId,
                    std::string_view(name),
                    carrerId,
                    level,
                    cityCopyId,
                    exp,
                    langId,
                    vipLevel,
                    vipExp,
                    action[0], action[1], action[2], action[3],
                    actime[0], actime[1], actime[2], actime[3],
                    createTime,
                    logonTime,
                    logoffTime,
                    groupMailTime,
                    fightValue,
                    guildId,
                    roleId  ///< WHERE 条件
            );
            return ret >= 0;
        }

        /**
         * @brief 更新角色数据到SQLite（只修改已有记录，不插入新记录）
         *
         * @details 使用 SQLite 的 `UPDATE` 语句，根据 [id](file://D:\cProjet\gameserver\net\cmake-build-debug\_deps\mysqlcpp-src\jdbc\examples\examples.h#L77-L77) 更新已有记录。
         * 与 SaveSQLite 的区别在于：SaveSQLite 使用 INSERT OR REPLACE，UpdateSQLite 使用 UPDATE。
         *
         * @return true 更新成功
         * @return false 更新失败
         */
        [[nodiscard]]
        bool UpdateSQLite() {
            int ret = cfl::db::SQLiteUtil::execute_prepared(
                    "gameserver",
                    "UPDATE role SET "
                    "accountid=?, name=?, carrerid=?, level=?, citycopyid=?, exp=?, langid=?, viplevel=?, vipexp=?, "
                    "action1=?, action2=?, action3=?, action4=?, "
                    "actime1=?, actime2=?, actime3=?, actime4=?, "
                    "createtime=?, logontime=?, logofftime=?, grouptime=?, fightvalue=?, guildid=? "
                    "WHERE id=?",
                    accountId,
                    std::string_view(name),
                    carrerId,
                    level,
                    cityCopyId,
                    exp,
                    langId,
                    vipLevel,
                    vipExp,
                    action[0], action[1], action[2], action[3],
                    actime[0], actime[1], actime[2], actime[3],
                    createTime,
                    logonTime,
                    logoffTime,
                    groupMailTime,
                    fightValue,
                    guildId,
                    roleId  ///< WHERE 条件
            );
            return ret >= 0;
        }

        /**
         * @brief 删除角色数据（逻辑删除）
         *
         * @details 通过更新 `isdelete=1` 标志位来实现逻辑删除，而不是物理删除。
         *
         * @return true 删除成功
         * @return false 删除失败
         */
        [[nodiscard]]
        bool Delete() {
            int ret = cfl::db::MySQLUtil::execute_prepared(
                    "gameserver",
                    "UPDATE role SET isdelete = 1 WHERE id = ?",
                    roleId
            );
            return ret >= 0;
        }

        /**
         * @brief 删除角色数据到SQLite（逻辑删除）
         *
         * @details 通过更新 `isdelete=1` 标志位来实现逻辑删除，而不是物理删除。
         *
         * @return true 删除成功
         * @return false 删除失败
         */
        [[nodiscard]]
        bool DeleteSQLite() {
            int ret = cfl::db::SQLiteUtil::execute_prepared(
                    "gameserver",
                    "UPDATE role SET isdelete = 1 WHERE id = ?",
                    roleId
            );
            return ret >= 0;
        }
    };
}
