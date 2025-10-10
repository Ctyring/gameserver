#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <string_view>
#include "db/db_mysql.h"
#include "server_define.h"

namespace cfl{
class StaticData;
using DataFunc = bool (StaticData::*)(db::SqlData&);

struct DataFuncNode {
    std::string table_name;
    DataFunc func;

    DataFuncNode(std::string name, DataFunc f)
            : table_name(std::move(name)), func(f) {}
};

class StaticData {
public:
    StaticData();
    ~StaticData() = default;

    StaticData(const StaticData&) = delete;
    StaticData& operator=(const StaticData&) = delete;

    [[nodiscard]] static StaticData& instance() noexcept {
        static StaticData instance;
        return instance;
    }

    bool init_data_reader();
    [[nodiscard]] bool load_config_data();
    [[nodiscard]] bool reload_config_data(std::string_view table_name);

    // ---------------- 基础配置 ----------------
    std::map<std::string, int32_t> constant_values;
    [[nodiscard]] bool read_constant_data(db::SqlData& query);
    [[nodiscard]] int32_t get_constant_value(std::string &name) const noexcept;

    // ---------------- 体力 ----------------
    std::unordered_map<uint32_t, StActionInfo> action_map;
    [[nodiscard]] bool read_action_config(db::SqlData& query);
    [[nodiscard]] int64_t get_action_max_value(uint32_t action_id) const noexcept;
    [[nodiscard]] uint32_t get_action_unit_time(uint32_t action_id) const noexcept;

    // ---------------- 职业 ----------------
    std::unordered_map<uint32_t, StCarrerInfo> carrer_map;
    [[nodiscard]] bool read_carrer(db::SqlData& query);
    [[nodiscard]] const StCarrerInfo* get_carrer_info(uint32_t carrer_id) const noexcept;

    // ---------------- 职业等级 ----------------
    std::vector<std::vector<StLevelInfo>> carrer_levels;
    [[nodiscard]] bool read_carrer_level(db::SqlData& query);
    [[nodiscard]] const StLevelInfo* get_carrer_level_info(uint32_t carrer_id, uint32_t level) const noexcept;

    // ---------------- 角色 ----------------
    std::unordered_map<uint32_t, StActorInfo> actor_map;
    [[nodiscard]] bool read_actor(db::SqlData& query);
    [[nodiscard]] const StActorInfo* get_actor_info(uint32_t actor_id) const noexcept;

    // ---------------- 技能关系 ----------------
    std::unordered_map<uint32_t, StActorSkillInfo> actor_skill_map;
    [[nodiscard]] bool read_actor_skill_info(db::SqlData& query);
    [[nodiscard]] const StActorSkillInfo* get_actor_skill_info(uint32_t actor_id) const noexcept;

    // ---------------- 副本 ----------------
    std::unordered_map<uint32_t, StCopyInfo> copy_info_map;
    [[nodiscard]] bool read_copy_info(db::SqlData& query);
    [[nodiscard]] const StCopyInfo* get_copy_info(uint32_t copy_id) const noexcept;
    [[nodiscard]] uint32_t get_copy_type(uint32_t copy_id) const noexcept;

    // ---------------- 语言配置 ----------------
    std::unordered_map<uint32_t, StLocalString> language_map;
    [[nodiscard]] bool read_language(db::SqlData& query);
    [[nodiscard]] std::string get_language_text(uint32_t id, uint32_t lang) const;
//    [[nodiscard]] std::string get_language_text(std::string_view id, uint32_t lang) const;

    // ---------------- 掉落 ----------------
    std::unordered_map<uint32_t, std::vector<StAwardItem>> award_items;
    [[nodiscard]] bool read_award_data(db::SqlData& query);
    [[nodiscard]] bool parse_drop_item(std::string_view drop, StDropItem& item) const;
    [[nodiscard]] bool get_award_item(int32_t award_id, int32_t carrer, StAwardItem& item) const;
    [[nodiscard]] bool get_award_item_by_index(int32_t award_id, int32_t carrer, int32_t index, StItemData& item) const;
    [[nodiscard]] bool get_items_from_award_id(int32_t award_id, int32_t carrer, std::vector<StItemData>& items, int32_t times) const;
//    [[nodiscard]] bool get_items_award_id_times(int32_t award_id, int32_t carrer, int32_t times, std::vector<StItemData>& items) const;

    // ---------------- 物品 ----------------
    std::unordered_map<uint32_t, StItemInfo> item_map;
    [[nodiscard]] bool read_item_data(db::SqlData& query);
    [[nodiscard]] const StItemInfo* get_item_info(uint32_t item_id) const noexcept;

    // ---------------- 功能表 ----------------
    std::unordered_map<uint32_t, StFuncInfo> func_info_map;
    [[nodiscard]] bool read_func_info(db::SqlData& query);
    [[nodiscard]] const StFuncInfo* get_func_info(uint32_t func_id) const noexcept;
    [[nodiscard]] bool is_func_open(uint32_t func_id, int32_t level, int32_t vip_level) const noexcept;

    // ---------------- 功能VIP ----------------
    std::unordered_map<uint32_t, StFuncVipInfo> func_vip_map;
    [[nodiscard]] bool read_func_vip_info(db::SqlData& query);
    [[nodiscard]] const StFuncVipInfo* get_func_vip_info(uint32_t func_id) const noexcept;

    // ---------------- 功能花费 ----------------
    std::unordered_map<uint32_t, std::vector<int32_t>> func_cost_map;
    [[nodiscard]] bool read_func_cost_info(db::SqlData& query);
    [[nodiscard]] int32_t get_func_cost_info(uint32_t func_id, int32_t times) const noexcept;

    // ---------------- 装备 ----------------
    std::unordered_map<uint32_t, StEquipInfo> equip_map;
    [[nodiscard]] bool read_equip_info(db::SqlData& query);
    [[nodiscard]] const StEquipInfo* get_equip_info(uint32_t equip_id) const noexcept;

    // ---------------- 宝石 ----------------
    std::unordered_map<uint32_t, StGemInfo> gem_map;
    [[nodiscard]] bool read_gem_info(db::SqlData& query);
    [[nodiscard]] const StGemInfo* get_gem_info(uint32_t gem_id) const noexcept;

    // ---------------- 宠物 ----------------
    std::unordered_map<uint32_t, StPetInfo> pet_map;
    [[nodiscard]] bool read_pet_info(db::SqlData& query);
    [[nodiscard]] const StPetInfo* get_pet_info(uint32_t pet_id) const noexcept;

    // ---------------- 伙伴 ----------------
    std::unordered_map<uint32_t, StPartnerInfo> partner_map;
    [[nodiscard]] bool read_partner_info(db::SqlData& query);
    [[nodiscard]] const StPartnerInfo* get_partner_info(uint32_t partner_id) const noexcept;

    // ---------------- 坐骑 ----------------
    std::unordered_map<uint32_t, StMountInfo> mount_map;
    [[nodiscard]] bool read_mount_info(db::SqlData& query);
    [[nodiscard]] const StMountInfo* get_mount_info(uint32_t mount_id) const noexcept;

    // ---------------- 任务 ----------------
    std::unordered_map<uint32_t, StTaskInfo> task_map;
    [[nodiscard]] bool read_task_info(db::SqlData& query);
    [[nodiscard]] const StTaskInfo* get_task_info(uint32_t task_id) const noexcept;

    // ---------------- 商店 ----------------
    std::unordered_map<uint32_t, StStoreItemInfo> store_map;
    [[nodiscard]] bool read_store_info(db::SqlData& query);
    [[nodiscard]] const StStoreItemInfo* get_store_item_info(uint32_t store_id) const noexcept;

    // ---------------- 活动 ----------------
    std::unordered_map<uint32_t, StActivityInfo> activity_map;
    [[nodiscard]] bool read_activity_info(db::SqlData& query);
    [[nodiscard]] const StActivityInfo* get_activity_info(uint32_t activity_type) const noexcept;

    // ---------------- 技能 ----------------
    std::unordered_map<uint32_t, StSkillInfo> skill_map;
    [[nodiscard]] bool read_skill_info(db::SqlData& query);
    [[nodiscard]] const StSkillInfo* get_skill_info(uint32_t skill_id, uint32_t level) const noexcept;

    // ---------------- 技能事件 ----------------
    std::unordered_map<uint32_t, StSkillEventInfo> skill_event_map;
    [[nodiscard]] bool read_skill_event();
    [[nodiscard]] const StSkillEventInfo* get_skill_event_info(uint32_t skill_id) const noexcept;

    // ---------------- 连击技能 ----------------
    std::unordered_map<uint32_t, StComboSkillInfo> combo_skill_map;
    [[nodiscard]] bool read_combo_skill_info(db::SqlData& query);
    [[nodiscard]] const StComboSkillInfo* get_combo_skill_info(uint32_t skill_id) const noexcept;

    // ---------------- Buff ----------------
    std::unordered_map<uint32_t, StBuffInfo> buff_map;
    [[nodiscard]] bool read_buff_info(db::SqlData& query);
    [[nodiscard]] const StBuffInfo* get_buff_info(uint32_t buff_id) const noexcept;

    // ---------------- 子弹 ----------------
    std::unordered_map<uint32_t, StBulletInfo> bullet_map;
    [[nodiscard]] bool read_bullet_info(db::SqlData& query);
    [[nodiscard]] const StBulletInfo* get_bullet_info(uint32_t bullet_id) const noexcept;

    // ---------------- 充值 ----------------
    std::unordered_map<uint32_t, StChargeInfo> charge_map;
    [[nodiscard]] bool read_charge_info(db::SqlData& query);
    [[nodiscard]] const StChargeInfo* get_charge_info(uint32_t charge_id) const noexcept;

    // ---------------- 函数注册 ----------------
    std::vector<DataFuncNode> data_func_list;
};
}