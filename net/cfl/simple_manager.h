#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>
#include "db/db_mysql.h"
namespace cfl {
    struct SimpleInfo {
        std::uint64_t role_id = 0;
        std::uint64_t account_id = 0;
        std::uint64_t guild_id = 0;
        std::uint32_t career_id = 0;
        std::uint32_t level = 0;
        std::uint32_t vip_level = 0;
        std::uint64_t fight_value = 0;
        std::uint64_t logoff_time = 0;
        std::uint64_t logon_time = 0;
        std::uint64_t create_time = 0;
        std::string name;
        bool is_deleted = false;
        std::uint32_t logon_status = 0;  // 0: normal, 1: loading from DB

        SimpleInfo() = default;
    };

    class SimpleManager {
    public:
        [[nodiscard]] static SimpleManager &instance() noexcept;

        [[nodiscard]] SimpleInfo *getSimpleInfoById(std::uint64_t id) noexcept;

        [[nodiscard]] SimpleInfo *createSimpleInfo(std::uint64_t roleId,
                                                   std::uint64_t accountId,
                                                   std::string_view name,
                                                   std::uint32_t careerId);

        bool addSimpleInfo(std::unique_ptr<SimpleInfo> info);

        bool loadData();

        [[nodiscard]] std::uint64_t getRoleIdByName(std::string_view name) const noexcept;

        [[nodiscard]] std::uint64_t getCreateTime(std::uint64_t id) const noexcept;

        [[nodiscard]] std::uint64_t getLogonTime(std::uint64_t id) const noexcept;

        [[nodiscard]] std::uint64_t getLogoffTime(std::uint64_t id) const noexcept;

        bool setCreateTime(std::uint64_t id, std::uint64_t time) noexcept;

        bool setLogonTime(std::uint64_t id, std::uint64_t time) noexcept;

        bool setLogoffTime(std::uint64_t id, std::uint64_t time) noexcept;

        [[nodiscard]] std::uint64_t getFightValue(std::uint64_t id) const noexcept;

        bool setFightValue(std::uint64_t id, std::uint64_t value, std::uint32_t level) noexcept;

        bool setName(std::uint64_t id, std::string_view name);

        bool setVipLevel(std::uint64_t id, std::uint32_t vipLevel) noexcept;

        bool setGuildId(std::uint64_t id, std::uint64_t guildId) noexcept;

        bool setRoleDeleted(std::uint64_t id, bool deleted) noexcept;

        [[nodiscard]] bool checkNameExist(std::string_view name) const noexcept;

        [[nodiscard]] bool checkNameFormat(std::string_view name) const noexcept;

        [[nodiscard]] std::uint64_t getGuildId(std::uint64_t id) const noexcept;

        [[nodiscard]] std::uint32_t getTotalCount() const noexcept;

        bool getRoleIdsByAccountId(std::uint64_t accountId, std::vector<std::uint64_t> &roleIds) const;

    private:
        SimpleManager() = default;

        ~SimpleManager() = default;

        SimpleManager(const SimpleManager &) = delete;

        SimpleManager &operator=(const SimpleManager &) = delete;

        std::unordered_map<std::uint64_t, std::unique_ptr<SimpleInfo>> id_to_info_;
        std::unordered_map<std::string, std::uint64_t> name_to_id_;
    };
}