#pragma once

#include <string>
#include <array>
#include <cstdint>
#include <ranges>
#include "cfl/cfl.h"
#include "cfl/db/db.h"
#include "cfl/db/db_mysql.h"
namespace cfl::shm {

    struct RoleDataObject : public SharedObject {
        // 基础数据
        uint64_t roleId{0};
        uint64_t accountId{0};
        char name[64];                // 角色名
        int32_t carrerId{0};
        int32_t level{0};
        std::array<int64_t, cfl::ActionNum> action{};   // 体力
        std::array<int64_t, cfl::ActionNum> actime{};   // 体力恢复时间
        int64_t exp{0};
        int32_t langId{0};
        int64_t fightValue{0};
        int32_t vipLevel{0};
        int32_t vipExp{0};
        int32_t cityCopyId{0};
        int32_t channel{0};
        bool isDeleted{false};
        int64_t qq{0};
        uint64_t createTime{0};
        uint64_t logonTime{0};
        uint64_t logoffTime{0};
        uint64_t groupMailTime{0};
        uint64_t guildId{0};
        uint32_t onlineTime{0};

        // 签到数据
        int32_t signNum{0};
        uint32_t signDay{0};
        uint32_t recvAction{0};

        RoleDataObject() = default;

        RoleDataObject(const RoleDataObject &) = delete;

        RoleDataObject &operator=(const RoleDataObject &) = delete;

        RoleDataObject(RoleDataObject &&) noexcept = default;

        RoleDataObject &operator=(RoleDataObject &&) noexcept = default;

        ~RoleDataObject() = default;

        [[nodiscard]]
        bool Save() {
            // 用 execute_fmt 构造 SQL
            // 注意这里需要完整拼接所有字段
            // 由于 std::format 没有自动转义字符串，需要自己处理 name
            auto escaped_name = name;

            int ret = cfl::db::MySQLUtil::execute_prepared(
                    "test",
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

        [[nodiscard]]
        bool Delete() {
            int ret = cfl::db::MySQLUtil::execute_prepared(
                    "test",
                    "UPDATE role SET isdelete = 1 WHERE id = {}",
                    roleId
            );
            return ret >= 0;
        }

    };
}
