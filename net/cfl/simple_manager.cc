#include "simple_manager.h"
#include "cfl.h"
#include "db/db_mysql.h"
namespace cfl {
    SimpleManager &SimpleManager::instance() noexcept {
        static SimpleManager instance;
        return instance;
    }

    bool SimpleManager::loadData() {
        auto query = db::MySQLUtil::query("db_game", "SELECT * FROM player");;
        while (query->next()) {
            auto info = std::make_unique<SimpleInfo>();
            info->role_id = query->get_int64("id");
            info->account_id = query->get_int64("accountid");
            info->name = query->get_string("name");
            info->career_id = query->get_int32("carrerid");
            info->create_time = query->get_int64("createtime");
            info->logon_time = query->get_int64("logontime");
            info->logoff_time = query->get_int64("logofftime");
            info->guild_id = query->get_int64("guildid");
            info->level = query->get_int32("level");
            info->vip_level = query->get_int32("viplevel");

            addSimpleInfo(std::move(info));
        }
        return true;
    }

    SimpleInfo *SimpleManager::getSimpleInfoById(std::uint64_t id) noexcept {
        auto it = id_to_info_.find(id);
        return (it != id_to_info_.end()) ? it->second.get() : nullptr;
    }

    std::uint64_t SimpleManager::getRoleIdByName(std::string_view name) const noexcept {
        auto it = name_to_id_.find(std::string(name));
        return (it != name_to_id_.end()) ? it->second : 0;
    }

    std::uint64_t SimpleManager::getCreateTime(std::uint64_t id) const noexcept {
        if (auto info = instance().getSimpleInfoById(id))
            return info->create_time;
        return 0;
    }

    std::uint64_t SimpleManager::getLogonTime(std::uint64_t id) const noexcept {
        if (auto info = instance().getSimpleInfoById(id))
            return info->logon_time;
        return 0;
    }

    std::uint64_t SimpleManager::getLogoffTime(std::uint64_t id) const noexcept {
        if (auto info = instance().getSimpleInfoById(id))
            return info->logoff_time;
        return 0;
    }

    std::uint64_t SimpleManager::getFightValue(std::uint64_t id) const noexcept {
        if (auto info = instance().getSimpleInfoById(id))
            return info->fight_value;
        return 0;
    }

    bool SimpleManager::setFightValue(std::uint64_t id, std::uint64_t value, std::uint32_t level) noexcept {
        if (auto info = getSimpleInfoById(id)) {
            info->fight_value = value;
            info->level = level;
            return true;
        }
        return false;
    }

    bool SimpleManager::setName(std::uint64_t id, std::string_view name) {
        auto *info = getSimpleInfoById(id);
        if (!info) return false;

        // 移除旧名称映射
        name_to_id_.erase(info->name);

        info->name = name;
        auto [it, inserted] = name_to_id_.insert({std::string(name), id});
        if (!inserted) {
//            std::cerr << "[SimpleManager] Duplicate role name: " << name << '\n';
            return false;
        }
        return true;
    }

    bool SimpleManager::setCreateTime(std::uint64_t id, std::uint64_t time) noexcept {
        if (auto info = getSimpleInfoById(id)) {
            info->create_time = time;
            return true;
        }
        return false;
    }

    bool SimpleManager::setLogonTime(std::uint64_t id, std::uint64_t time) noexcept {
        if (auto info = getSimpleInfoById(id)) {
            info->logon_time = time;
            return true;
        }
        return false;
    }

    bool SimpleManager::setLogoffTime(std::uint64_t id, std::uint64_t time) noexcept {
        if (auto info = getSimpleInfoById(id)) {
            info->logoff_time = time;
            return true;
        }
        return false;
    }

    bool SimpleManager::setVipLevel(std::uint64_t id, std::uint32_t vipLevel) noexcept {
        if (auto info = getSimpleInfoById(id)) {
            info->vip_level = vipLevel;
            return true;
        }
        return false;
    }

    bool SimpleManager::setGuildId(std::uint64_t id, std::uint64_t guildId) noexcept {
        if (auto info = getSimpleInfoById(id)) {
            info->guild_id = guildId;
            return true;
        }
        return false;
    }

    bool SimpleManager::setRoleDeleted(std::uint64_t id, bool deleted) noexcept {
        if (auto info = getSimpleInfoById(id)) {
            info->is_deleted = deleted;
            return true;
        }
        return false;
    }

    bool SimpleManager::checkNameExist(std::string_view name) const noexcept {
        return name_to_id_.contains(std::string(name));
    }

    bool SimpleManager::checkNameFormat(std::string_view name) const noexcept {
        if (name.size() < 4 || name.size() > 20)
            return false;
        constexpr std::string_view invalid_chars = ",;'\" \\%%\r\n";
        return name.find_first_of(invalid_chars) == std::string_view::npos;
    }

    std::uint64_t SimpleManager::getGuildId(std::uint64_t id) const noexcept {
        if (auto info = instance().getSimpleInfoById(id))
            return info->guild_id;
        return 0;
    }

    std::uint32_t SimpleManager::getTotalCount() const noexcept {
        return static_cast<std::uint32_t>(id_to_info_.size());
    }

    bool SimpleManager::getRoleIdsByAccountId(std::uint64_t accountId, std::vector<std::uint64_t> &roleIds) const {
        roleIds.clear();
        for (const auto &[id, info]: id_to_info_) {
            if (info && info->account_id == accountId)
                roleIds.push_back(id);
        }
        return true;
    }

    SimpleInfo *SimpleManager::createSimpleInfo(std::uint64_t roleId,
                                                std::uint64_t accountId,
                                                std::string_view name,
                                                std::uint32_t careerId) {
        auto info = std::make_unique<SimpleInfo>();
        info->role_id = roleId;
        info->account_id = accountId;
        info->name = name;
        info->career_id = careerId;
        info->create_time = get_timestamp();
        info->vip_level = 0;
        info->level = 0;
        info->fight_value = 0;

        auto raw_ptr = info.get();
        id_to_info_.emplace(roleId, std::move(info));
        name_to_id_[std::string(name)] = roleId;
        return raw_ptr;
    }

    bool SimpleManager::addSimpleInfo(std::unique_ptr<SimpleInfo> info) {
        if (!info) return false;

        auto roleId = info->role_id;
        const auto &name = info->name;

        id_to_info_.emplace(roleId, std::move(info));

        auto [_, inserted] = name_to_id_.insert({name, roleId});
        if (!inserted) {
//            std::cerr << "[SimpleManager] Duplicate role name detected: " << name << '\n';
        }

        return true;
    }
}