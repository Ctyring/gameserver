#include "static_data.h"
#include "cfl/db/db_mysql.h"
#include "cfl/db/db_sqlite.h"
#include "cfl/tools/common.h"
#include "pugixml.hpp"
#include <fstream>

namespace cfl {
    StaticData::StaticData() {
        carrer_levels.resize(4);
        for(int i = 0; i < 4; ++i){
            carrer_levels[i].resize(MaxRoleLevel);
        }
        init_data_reader();
        load_config_data();
    }

    bool StaticData::init_data_reader() {
        data_func_list.emplace_back("Data_Constant", &StaticData::read_constant_data);
        data_func_list.emplace_back("Data_Role", &StaticData::read_carrer);
        data_func_list.emplace_back("Data_RoleLevel", &StaticData::read_carrer_level);
        data_func_list.emplace_back("Data_Actor", &StaticData::read_actor);
        data_func_list.emplace_back("Data_Copy", &StaticData::read_copy_info);
        data_func_list.emplace_back("Data_Item", &StaticData::read_item_data);
        data_func_list.emplace_back("Data_Action", &StaticData::read_action_config);
        data_func_list.emplace_back("Data_Actor_Skill", &StaticData::read_actor_skill_info);
        data_func_list.emplace_back("Data_Equip", &StaticData::read_equip_info);
        data_func_list.emplace_back("Data_Gem", &StaticData::read_gem_info);
        data_func_list.emplace_back("Data_Partner", &StaticData::read_partner_info);
        data_func_list.emplace_back("Data_Mount", &StaticData::read_mount_info);
        data_func_list.emplace_back("Data_Store", &StaticData::read_store_info);
        data_func_list.emplace_back("Data_Combo_Skill", &StaticData::read_combo_skill_info);
        data_func_list.emplace_back("Data_Skill", &StaticData::read_skill_info);
        data_func_list.emplace_back("Data_FlyObject", &StaticData::read_bullet_info);
        data_func_list.emplace_back("Data_Pet", &StaticData::read_pet_info);
        data_func_list.emplace_back("Data_Language", &StaticData::read_language);
        data_func_list.emplace_back("Data_Award", &StaticData::read_award_data);
        data_func_list.emplace_back("Data_Func", &StaticData::read_func_info);
        data_func_list.emplace_back("Data_Func_Vip", &StaticData::read_func_vip_info);
        data_func_list.emplace_back("Data_Buff", &StaticData::read_buff_info);
        // todo
//        data_func_list.emplace_back("Data_Func_Cost", &StaticData::read_func_cost_info);
//        data_func_list.emplace_back("Data_Task", &StaticData::read_task_info);
        return true;
    }

    bool StaticData::load_config_data() {
        db::SQLiteMgr::instance()->register_sqlite("config_db", {{"dbname", "data/config.db"}});
        auto db = db::SQLiteMgr::instance()->get("config_db");
        if (!db) {
            spdlog::error("load config db error");
            return false;
        }

        for (auto &func: data_func_list) {
            auto name = func.table_name;
            auto result = db->query("select * from " + name);
            (this->*func.func)(*result);
        }
        return true;
    }

    bool StaticData::reload_config_data(std::string_view table_name) {
        auto db = db::SQLiteMgr::instance()->get("config_db");
        if (!db) {
            spdlog::error("reload config db error");
            return false;
        }
        for (auto &func: data_func_list)
            if (func.table_name == table_name) {
                auto name = func.table_name;
                auto result = db->query("select * from " + name);
                (this->*func.func)(*result);
                return true;
            }
        return false;
    }

    bool StaticData::read_constant_data(db::SqlData &query) {
        constant_values.clear();
        while (query.next()) {
            auto name = query.get_string("Name");
            auto value = query.get_int32("Value");
            constant_values.emplace(std::move(name), value);
        }

        return true;
    }

    int32_t StaticData::get_constant_value(std::string &name) const noexcept {
        auto iter = constant_values.find(name);
        if (iter == constant_values.end())
            spdlog::error("constant_values not find name:{}", name);
        return iter != constant_values.end() ? iter->second : 0;
    }

    bool StaticData::read_action_config(db::SqlData &query) {
        action_map.clear();
        while (query.next()) {
            StActionInfo info;
            info.action_id = query.get_int32("Id");
            info.max_value = query.get_int32("Max");
            info.unit_time = query.get_int32("UnitTime");
            action_map.emplace(info.action_id, info);
        }
        return true;
    }

    int64_t StaticData::get_action_max_value(uint32_t action_id) const noexcept {
        if (action_map.find(action_id) != action_map.end()) {
            return action_map.at(action_id).max_value;
        }
        spdlog::error("action_map not find action_id:{}", action_id);
        return 0;
    }

    uint32_t StaticData::get_action_unit_time(uint32_t action_id) const noexcept {
        if (action_map.find(action_id) != action_map.end()) {
            return action_map.at(action_id).unit_time;
        }
        spdlog::error("action_map not find action_id:{}", action_id);
        return 0;
    }

    bool StaticData::read_carrer(db::SqlData &query) {
        carrer_map.clear();
        while (query.next()) {
            StCarrerInfo info;
            info.id = query.get_int32("Carrer");
            info.actor_id = query.get_int32("ActorID");
            info.born_city = query.get_int32("BornCity");
            info.name = query.get_string("CarrerName");

            carrer_map.emplace(info.id, info);
        }

        return true;
    }

    const StCarrerInfo *StaticData::get_carrer_info(uint32_t carrer_id) const noexcept {
        if (carrer_map.find(carrer_id) != carrer_map.end())
            return &carrer_map.at(carrer_id);
        spdlog::error("carrer_map not find carrer_id:{}", carrer_id);
        return nullptr;
    }

    bool StaticData::read_carrer_level(db::SqlData &query) {
        while (query.next()) {
            auto carrer_id = query.get_int32("Carrer");
            auto level = query.get_int32("Level");
            carrer_levels[carrer_id - 1][level - 1].level = level;
            carrer_levels[carrer_id - 1][level - 1].fight_value = query.get_int32("FightValue");
            carrer_levels[carrer_id - 1][level - 1].need_exp = query.get_int32("RequireExp");

            int idx = query.column_index("P1");
            // todo: 优化判断 有些列可能不存在
            for (int i = 0; i < PropertyNum - 1; i++) {
                carrer_levels[carrer_id - 1][level - 1].properties[i] = query.get_int32(idx + i);
            }
        }
        return true;
    }

    const StLevelInfo *StaticData::get_carrer_level_info(uint32_t carrer_id, uint32_t level) const noexcept {
        if (level > carrer_levels[carrer_id - 1].size())
            return nullptr;
        return &carrer_levels[carrer_id - 1][level - 1];
    }

    bool StaticData::read_actor(db::SqlData &query) {
        actor_map.clear();
        while (query.next()) {
            StActorInfo info;
            info.id = query.get_int32("Id");
            info.init_level = query.get_int32("Level");
            info.default_speed = query.get_float("DefSpeed");
            info.radius = query.get_float("Radius");
            info.type = query.get_int32("Type");
            info.ai_id = query.get_int32("AiId");
            int idx = query.column_index("P1");
            for (int i = 0; i < PropertyNum; i++) {
                info.properties[i] = query.get_int32(idx + i);
            }

            actor_map.emplace(info.id, info);
        }
        return true;
    }

    const StActorInfo *StaticData::get_actor_info(uint32_t actor_id) const noexcept {
        if (actor_map.find(actor_id) != actor_map.end())
            return &actor_map.at(actor_id);
        spdlog::error("actor_map not find actor_id:{}", actor_id);
        return nullptr;
    }

    bool StaticData::read_actor_skill_info(db::SqlData &query) {
        actor_skill_map.clear();
        while (query.next()) {
            StActorSkillInfo info;
            info.actor_id = query.get_int32("Id");
            info.normal_id = query.get_int32("Normal1");
            info.specials[0] = query.get_int32("Special1");
            info.specials[1] = query.get_int32("Special2");
            info.specials[2] = query.get_int32("Special3");
            info.specials[3] = query.get_int32("Special4");
            info.specials[4] = query.get_int32("Special5");
            actor_skill_map.emplace(info.actor_id, info);
        }
        return true;
    }

    const StActorSkillInfo *StaticData::get_actor_skill_info(uint32_t actor_id) const noexcept {
        if (actor_skill_map.find(actor_id) != actor_skill_map.end())
            return &actor_skill_map.at(actor_id);
        spdlog::error("actor_skill_map not find actor_id:{}", actor_id);
        return nullptr;
    }

    bool StaticData::read_copy_info(db::SqlData &query) {
        copy_info_map.clear();
        while (query.next()) {
            StCopyInfo info;
            info.copy_id = query.get_int32("Id");
            info.copy_type = query.get_int32("CopyType");
            info.cost_act_id = query.get_int32("CostActionId");
            info.cost_act_num = query.get_int32("CostActionNum");
            info.get_money_id = query.get_int32("GetMoneyId");
            info.get_money_ratio = query.get_int32("GetMoneyRatio");
            copy_info_map.emplace(info.copy_id, info);
        }
        return true;
    }

    const StCopyInfo *StaticData::get_copy_info(uint32_t copy_id) const noexcept {
        if (copy_info_map.find(copy_id) != copy_info_map.end())
            return &copy_info_map.at(copy_id);
        spdlog::error("copy_info_map not find copy_id:{}", copy_id);
        return nullptr;
    }

    uint32_t StaticData::get_copy_type(uint32_t copy_id) const noexcept {
        if (copy_info_map.find(copy_id) != copy_info_map.end())
            return copy_info_map.at(copy_id).copy_type;
        spdlog::error("copy_info_map not find copy_id:{}", copy_id);
        return 0;
    }

    bool StaticData::read_language(db::SqlData &query) {
        while (query.next()) {
            StLocalString info;
            info.id = query.get_int32("Id");
            info.language[0] = query.get_string("Language_0");
            info.language[1] = query.get_string("Language_1");
            info.language[2] = query.get_string("Language_2");
            info.language[3] = query.get_string("Language_3");
            info.language[4] = query.get_string("Language_4");
            info.language[5] = query.get_string("Language_5");
            info.language[6] = query.get_string("Language_6");
            info.language[7] = query.get_string("Language_7");
            info.language[8] = query.get_string("Language_8");
            info.language[9] = query.get_string("Language_9");
            info.language[10] = query.get_string("Language_10");
            info.language[11] = query.get_string("Language_11");
            info.language[12] = query.get_string("Language_12");
            info.language[13] = query.get_string("Language_13");
            info.language[14] = query.get_string("Language_14");
            info.language[15] = query.get_string("Language_15");
            language_map.emplace(info.id, info);
        }
        return true;
    }

    std::string StaticData::get_language_text(uint32_t id, uint32_t lang) const {
        if (language_map.find(id) != language_map.end())
            return language_map.at(id).language[lang];
        spdlog::error("language_map not find id:{}", id);
        return "";
    }

    bool StaticData::read_award_data(db::SqlData &query) {
        award_items.clear();
        while (query.next()) {
            StAwardItem info;
            info.award_id = query.get_int32("Id");
            std::string fix_drop = query.get_string(3);
            std::string ratio_drop = query.get_string(5);
            info.distinct = query.get_int32(6);
            info.ratio_count = query.get_int32(4);

            // 固定掉落
            if (fix_drop != "NULL") {
                auto ret = split_string(fix_drop, ")(");
                for (auto &fix_item: ret) {
                    StDropItem drop_item;
                    if (parse_drop_item(fix_item, drop_item)) {
                        info.fix_items.emplace_back(drop_item);
                    } else {
                        spdlog::error("[read_award_data] fix parse_drop_item error:{}", fix_item);
                    }
                }
            }

            // 概率掉落
            if (ratio_drop != "NULL") {
                auto ret = split_string(ratio_drop, ")(");
                int32_t check_ratio = 0;
                for (auto &ratio_item: ret) {
                    StDropItem drop_item;
                    if (parse_drop_item(ratio_item, drop_item)) {
                        info.ratio_items.emplace_back(drop_item);
                        check_ratio += drop_item.ratio;
                    } else {
                        spdlog::error("parse_drop_item error:{}", ratio_item);
                    }
                }

                if (check_ratio != 10000) {
                    // todo: 找一个好的设计的概率掉落，现在的话就先不动
//                    spdlog::error("[read_award_data] ratio ratio_drop check_ratio != 10000:{}", check_ratio);
                }
            }

            auto it = award_items.find(info.award_id);
            if (it != award_items.end()) {
                it->second.emplace_back(info);
            } else {
                award_items.emplace(info.award_id, std::vector<StAwardItem>{info});
            }
        }

        return true;
    }

    bool StaticData::parse_drop_item(std::string_view drop, cfl::StDropItem &item) const {
        // 去掉drop开头和结尾的() 如果有的话
        if(drop.front() == '('){
            drop = drop.substr(1, drop.size());
        }
        if(drop.back() == ')'){
            drop = drop.substr(0, drop.size() - 1);
        }
        auto ret = split_string(drop, "|");
        if (ret.size() < 3) {
            return false;
        }
        item.item_id = std::stoi(std::string(ret[0]));
        item.ratio = std::stoi(std::string(ret[2]));
        ret = split_string(ret[1], "&");
        item.item_num[0] = std::stoi(std::string(ret[0]));
        item.item_num[1] = std::stoi(std::string(ret[1]));
        return true;
    }

    bool StaticData::get_award_item(int32_t award_id, int32_t carrer, cfl::StAwardItem &item) const {
        if (award_items.find(award_id) != award_items.end()) {
            auto &award_list = award_items.find(award_id)->second;
            for (auto &award_item: award_list) {
                if (award_item.carrer == carrer) {
                    item = award_item;
                    return true;
                }
            }
        }
        return false;
    }

    bool StaticData::get_award_item_by_index(int32_t award_id, int32_t carrer, int32_t index,
                                             cfl::StItemData &item) const {
        StAwardItem t_item;
        if (!get_award_item(award_id, carrer, t_item)) {
            spdlog::error("get_award_item error:{}", award_id);
            return false;
        }

        if (index >= t_item.fix_items.size()) {
            spdlog::error("index >= fix_items.size():{}", index);
            return false;
        }

        item.item_id = t_item.fix_items[index].item_id;
        item.item_num = t_item.fix_items[index].item_num[0];
        return true;
    }

// 根据奖励ID和职业，从奖励表中获取对应的物品列表
    bool StaticData::get_items_from_award_id(int32_t award_id, int32_t carrer, std::vector<StItemData> &items,
                                             int32_t times = 1) const {
        StAwardItem all_item;  // 用于存储指定奖励ID和职业对应的全部奖励信息
        // 尝试获取奖励信息，如果获取失败则打印错误日志并返回 false
        if (!get_award_item(award_id, carrer, all_item)) {
            spdlog::error("get_award_item error:{}", award_id);
            return false;
        }

        StItemData t_item;  // 临时存储单个物品信息

        // 处理固定奖励物品列表
        for (auto &item: all_item.fix_items) {
            t_item.item_id = item.item_id;  // 物品ID

            // 判断固定奖励物品数量是否是区间，如果是区间就随机生成数量
            if (item.item_num[0] == item.item_num[1]) {
                t_item.item_num = item.item_num[0];  // 数量固定
            } else {
                // 随机生成数量: [item.item_num[0], item.item_num[1]]
                t_item.item_num = item.item_num[0] + random_int() % (item.item_num[1] - item.item_num[0] + 1);
            }

            // 只有数量大于0的物品才加入结果列表
            if (t_item.item_num > 0) {
                items.emplace_back(t_item);
            }
        }

        // 处理概率奖励物品列表（ratio_items）
        // ratio_count 表示要进行几次概率奖励抽取
        for (int i = 0; i < all_item.ratio_count * times; i++) {
            auto random_value = random_int();  // 随机值用于判断落在哪个奖励区间

            for (auto &item: all_item.ratio_items) {
                if (random_value < item.ratio) {  // 随机值落入当前物品概率区间
                    t_item.item_id = item.item_id;

                    // 判断数量区间并随机生成数量
                    if (item.item_num[0] == item.item_num[1]) {
                        t_item.item_num = item.item_num[0];  // 数量固定
                    } else {
                        t_item.item_num = item.item_num[0] + random_int() % (item.item_num[1] - item.item_num[0] + 1);
                    }

                    // 数量大于0才加入结果列表
                    if (t_item.item_num > 0) {
                        items.emplace_back(t_item);
                    }
                } else {
                    // 如果随机值没有落在当前物品概率区间，则减去当前物品概率继续判断下一个
                    random_value -= item.ratio;
                }
            }
        }

        return true;  // 成功获取奖励物品
    }

    bool StaticData::read_item_data(db::SqlData &query) {
        item_map.clear();
        while (query.next()) {
            StItemInfo item;
            item.item_id = query.get_int32("Id");
            item.item_type = (ItemType) query.get_int32("ItemType");
            item.bag_type = query.get_int32("BagType");
            item.sell_id = query.get_int32("SellMoneyId");
            item.sell_price = query.get_int32("SellMoneyNum");
            item.quality = query.get_int32("Quality");
            item.stack_max = query.get_int32("StackMax");
            item.carrer_id = query.get_int32("Carrer");
            item.data1 = query.get_int32("Data1");
            item.data2 = query.get_int32("Data2");
            item_map.emplace(item.item_id, item);
        }
        return true;
    }

    const StItemInfo *StaticData::get_item_info(uint32_t item_id) const noexcept {
        if (item_map.find(item_id) != item_map.end()) {
            return &item_map.find(item_id)->second;
        }
        spdlog::error("get_item_info error:{}", item_id);
        return nullptr;
    }

    bool StaticData::read_func_info(db::SqlData &query) {
        func_info_map.clear();
        while (query.next()) {
            StFuncInfo info;
            info.func_id = query.get_int32("FuncId");
            info.name = query.get_string("Name");
            info.vip_level = query.get_int32("VipLevel");
            info.logic = query.get_int32("Logic");
            func_info_map.emplace(info.func_id, info);
        }
        return true;
    }

    const StFuncInfo *StaticData::get_func_info(uint32_t func_id) const noexcept {
        if (func_info_map.find(func_id) != func_info_map.end()) {
            return &func_info_map.find(func_id)->second;
        }
        spdlog::error("get_func_info error:{}", func_id);
        return nullptr;
    }

    bool StaticData::is_func_open(uint32_t func_id, int32_t level, int32_t vip_level) const noexcept {
        auto *info = get_func_info(func_id);
        if (info == nullptr || info->logic <= 0 || info->logic > 5) {
            spdlog::error("get_func_info error:{}", func_id);
            return false;
        }
        if (info->logic == 1) {
            if (level >= info->open_level || vip_level >= info->vip_level) {
                return true;
            }
        } else if (info->logic == 2) {
            if (level >= info->open_level && vip_level >= info->vip_level) {
                return true;
            }
        } else if (info->logic == 3) {
            if (vip_level >= info->vip_level) {
                return true;
            }
        } else if (info->logic == 4) {
            if (level >= info->open_level) {
                return true;
            }
        }
        return false;
    }

    bool StaticData::read_func_vip_info(db::SqlData &query) {
        func_vip_map.clear();
        while (query.next()) {
            StFuncVipInfo info;
            info.func_id = query.get_int32("FuncId");
            info.vip_value[0] = query.get_int32("Vip0");
            info.vip_value[1] = query.get_int32("Vip1");
            info.vip_value[2] = query.get_int32("Vip2");
            info.vip_value[3] = query.get_int32("Vip3");
            info.vip_value[4] = query.get_int32("Vip4");
            info.vip_value[5] = query.get_int32("Vip5");
            info.vip_value[6] = query.get_int32("Vip6");
            info.vip_value[7] = query.get_int32("Vip7");
            info.vip_value[8] = query.get_int32("Vip8");
            info.vip_value[9] = query.get_int32("Vip9");
            info.vip_value[10] = query.get_int32("Vip10");
            info.vip_value[11] = query.get_int32("Vip11");
            info.vip_value[12] = query.get_int32("Vip12");
            func_vip_map.emplace(info.func_id, info);
        }
        return true;
    }

    const StFuncVipInfo *StaticData::get_func_vip_info(uint32_t func_id) const noexcept {
        if (func_vip_map.find(func_id) != func_vip_map.end()) {
            return &func_vip_map.find(func_id)->second;
        }
        spdlog::error("get_func_vip_info error:{}", func_id);
        return nullptr;
    }

    bool StaticData::read_func_cost_info(db::SqlData &query) {
//        func_cost_map.clear();
//        while (query.next()) {

//        }
        // todo
        spdlog::error("read_func_cost_info not implement");
        return false;
    }

    int32_t StaticData::get_func_cost_info(uint32_t func_id, int32_t times) const noexcept {
        // todo
        spdlog::error("get_func_cost not implement");
        return 0;
    }

    bool StaticData::read_equip_info(db::SqlData &query) {
        equip_map.clear();

        while (query.next()) {
            StEquipInfo info;
            info.equip_id = query.get_int32("Id");
            info.suit_id = query.get_int32("Suit");
            info.pos = query.get_int32("Pos");
            equip_map.emplace(info.equip_id, info);
        }

        return true;
    }

    const StEquipInfo *StaticData::get_equip_info(uint32_t equip_id) const noexcept {
        if (equip_map.find(equip_id) != equip_map.end()) {
            return &equip_map.find(equip_id)->second;
        }
        spdlog::error("get_equip_info error:{}", equip_id);
        return nullptr;
    }

    bool StaticData::read_gem_info(db::SqlData &query) {
        gem_map.clear();
        while (query.next()) {
            StGemInfo info;
            info.gem_id = query.get_int32("Id");
            info.pos = query.get_int32("Pos");
            gem_map.emplace(info.gem_id, info);
        }

        return true;
    }

    const StGemInfo *StaticData::get_gem_info(uint32_t gem_id) const noexcept {
        if (gem_map.find(gem_id) != gem_map.end()) {
            return &gem_map.find(gem_id)->second;
        }
        spdlog::error("get_gem_info error:{}", gem_id);
        return nullptr;
    }

    bool StaticData::read_pet_info(db::SqlData &query) {
        pet_map.clear();
        while (query.next()) {
            StPetInfo info;
            info.pet_id = query.get_int32("Id");
            info.actor_id = query.get_int32("ActorId");
            pet_map.emplace(info.pet_id, info);
        }

        return true;
    }

    const StPetInfo *StaticData::get_pet_info(uint32_t pet_id) const noexcept {
        if (pet_map.find(pet_id) != pet_map.end()) {
            return &pet_map.find(pet_id)->second;
        }
        spdlog::error("get_pet_info error:{}", pet_id);
        return nullptr;
    }

    bool StaticData::read_partner_info(db::SqlData &query) {
        partner_map.clear();
        while (query.next()) {
            StPartnerInfo info;
            info.partner_id = query.get_int32("Id");
            info.actor_id = query.get_int32("ActorId");
            partner_map.emplace(info.partner_id, info);
        }
        return true;
    }

    const StPartnerInfo *StaticData::get_partner_info(uint32_t partner_id) const noexcept {
        if (partner_map.find(partner_id) != partner_map.end()) {
            return &partner_map.find(partner_id)->second;
        }
        spdlog::error("get_partner_info error:{}", partner_id);
        return nullptr;
    }

    bool StaticData::read_mount_info(db::SqlData &query) {
        mount_map.clear();
        while (query.next()) {
            StMountInfo info;
            info.mount_id = query.get_int32("Id");
            info.actor_id = query.get_int32("ActorId");
            mount_map.emplace(info.mount_id, info);
        }
        return true;
    }

    const StMountInfo *StaticData::get_mount_info(uint32_t mount_id) const noexcept {
        if (mount_map.find(mount_id) != mount_map.end()) {
            return &mount_map.find(mount_id)->second;
        }
        spdlog::error("get_mount_info error:{}", mount_id);
        return nullptr;
    }

    bool StaticData::read_task_info(db::SqlData &query) {
        // todo
        spdlog::error("read_task_info not implement");
        return false;
    }

    const StTaskInfo *StaticData::get_task_info(uint32_t task_id) const noexcept {
        // todo
        spdlog::error("get_task_info not implement");
        return nullptr;
    }

    bool StaticData::read_store_info(db::SqlData &query) {
        store_map.clear();
        while (query.next()) {
            StStoreItemInfo info;
            info.store_id = query.get_int32("Id");
            info.item_id = query.get_int32("ItemID");
            info.item_num = query.get_int32("ItemNum");
            info.cost_money_id = query.get_int32("CostMoneyID");
            info.cost_money_num = query.get_int32("CostMoneyNum");
            info.store_type = query.get_int32("StoreType");
            store_map.emplace(info.store_id, info);
        }

        return true;
    }

    const StStoreItemInfo *StaticData::get_store_item_info(uint32_t store_id) const noexcept {
        if (store_map.find(store_id) != store_map.end()) {
            return &store_map.find(store_id)->second;
        }
        spdlog::error("get_store_info error:{}", store_id);
        return nullptr;
    }

    bool StaticData::read_activity_info(db::SqlData &query) {
        activity_map.clear();
        while (query.next()) {
            StActivityInfo info;
            info.activity_id = query.get_int32("Id");
            activity_map.emplace(info.activity_id, info);
        }
        return true;
    }

    const StActivityInfo *StaticData::get_activity_info(uint32_t activity_id) const noexcept {
        if (activity_map.find(activity_id) != activity_map.end()) {
            return &activity_map.find(activity_id)->second;
        }
        spdlog::error("get_activity_info error:{}", activity_id);
        return nullptr;
    }

    bool StaticData::read_skill_info(db::SqlData &query) {
        skill_map.clear();
        while (query.next()) {
            StSkillInfo info;
            info.skill_id = query.get_int32("Id");
            info.level = query.get_int32("Level");
            info.cd = query.get_int32("CountDown");
            info.hurt_fix = query.get_int32("HurtFix");
            info.hurt_multi = query.get_int32("HurtMuti");
            info.skill_type = query.get_int32("SkillType");
            info.hit_ship_type = (HitShipType) query.get_int32("HitShipType");
            info.hit_myself = query.get_int32("HitMyself");
            int id = info.level << 20 | info.skill_id;
            skill_map.emplace(id, info);
        }
        return true;
    }

    const StSkillInfo *StaticData::get_skill_info(uint32_t skill_id, uint32_t level) const noexcept {
        if (level <= 0 || skill_id <= 0) {
            spdlog::error("get_skill_info error:{} {}", skill_id, level);
            return nullptr;
        }
        int id = level << 20 | skill_id;
        if (skill_map.find(id) != skill_map.end()) {
            return &skill_map.find(id)->second;
        }
        spdlog::error("get_skill_info error:{} {}", skill_id, level);
        return nullptr;
    }

    bool StaticData::read_skill_event() {
        using namespace std::literals;      // 方便使用 "..."sv 这种 string_view 字面量
        using pugi::xml_document;            // 引入 pugixml 的 xml_document 类型
        using pugi::xml_node;                // 引入 pugixml 的 xml_node 类型
        namespace fs = std::filesystem;      // 简化 std::filesystem 命名空间

        const fs::path xmlPath = "Skill/Battle_Skill.xml";  // XML 文件路径

        // ---------- 检查文件是否存在 ----------
        if (!fs::exists(xmlPath)) {
            std::cerr << "Skill config not found: " << xmlPath << std::endl;
            return FALSE;  // 文件不存在直接返回
        }

        // ---------- 读取整个文件内容 ----------
        std::ifstream file(xmlPath, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open: " << xmlPath << std::endl;
            return FALSE;  // 打开文件失败
        }

        // 读取整个文件到字符串中
        std::string xmlContent(std::istreambuf_iterator<char>(file), {});
        file.close();

        // ---------- 解析 XML ----------
        xml_document doc;
        auto result = doc.load_string(xmlContent.data());
        if (!result) {
            std::cerr << "XML parse error: " << result.description() << std::endl;
            return false;  // 解析失败
        }

        // 获取根节点 <Root>
        auto root = doc.child("Root");
        if (!root) {
            std::cerr << "Missing <Root> node in " << xmlPath << std::endl;
            return false;  // 根节点不存在
        }

        // ---------- 遍历 <Skill> 节点 ----------
        for (xml_node &skillNode: root.children("Skill")) {
            StSkillEventInfo skillInfo{};  // 存储单个技能信息结构体

            // 便捷获取属性的 lambda，返回 string_view
            auto getAttr = [](const xml_node &node, std::string_view name) -> std::string_view {
                auto attr = node.attribute(name.data());
                return attr ? attr.value() : ""sv;
            };

            // ---------- 解析技能ID ----------
            if (auto val = getAttr(skillNode, "ID"sv); !val.empty())
                skillInfo.skill_id = std::stoi(val.data());
            else
                continue;  // 没有 ID 的技能跳过

            // ---------- 解析技能持续时间（秒 → 毫秒） ----------
            if (auto val = getAttr(skillNode, "Duration"sv); !val.empty())
                skillInfo.duration = static_cast<uint64_t>(std::stof(val.data()) * 1000);

            // ---------- 解析施放类型 ----------
            if (auto val = getAttr(skillNode, "CastType"sv); !val.empty()) {
                skillInfo.cast_type = std::stoi(val.data());
            }

            // ---------- 遍历 <ActScope> 子节点 ----------
            for (xml_node actNode: skillNode.children("ActScope")) {
                StSkillEvent event{};  // 存储技能行为事件

                // 解析攻击范围类型
                if (auto val = getAttr(actNode, "RangeType"sv); !val.empty())
                    event.range_type = static_cast<ERangeType>(std::stoi(val.data()));
                else
                    continue;

                // 解析攻击范围参数（波形或距离等）
                if (auto val = getAttr(actNode, "RangeParams"sv); !val.empty())
                    string_to_vector(val.data(), event.range_params, '~');  // 自定义解析函数
                else
                    continue;

                // 触发时间（秒 → 毫秒）
                if (auto val = getAttr(actNode, "StTime"sv); !val.empty())
                    event.trigger_time = static_cast<uint64_t>(std::stof(val.data()) * 1000);
                else
                    continue;

                // 命中动作ID
                if (auto val = getAttr(actNode, "HitActionID"sv); !val.empty())
                    event.hit_action_id = std::stoi(val.data());
                else
                    continue;

                // 命中特效ID
                if (auto val = getAttr(actNode, "HitEffectID"sv); !val.empty())
                    event.hit_effect = std::stoi(val.data());
                else
                    continue;

                // 命中距离
                if (auto val = getAttr(actNode, "HitDistance"sv); !val.empty())
                    event.hit_distance = std::stof(val.data());
                else
                    continue;

                // ---------- 遍历 <ActFlyObject> 子节点 ----------
                for (xml_node bulletNode: actNode.children("ActFlyObject")) {
                    StBulletObject bullet{};  // 子弹对象

                    if (auto val = getAttr(bulletNode, "ID"sv); !val.empty())
                        bullet.bullet_id = std::stoi(val.data());
                    else
                        continue;

                    if (auto val = getAttr(bulletNode, "Angle"sv); !val.empty())
                        bullet.angle = std::stof(val.data());
                    else
                        continue;

                    // 保存子弹对象到事件中
                    event.bullets.emplace_back(bullet);
                }

                // 保存事件到技能信息中
                skillInfo.events.emplace_back(std::move(event));
            }

            // ---------- 将解析好的技能信息保存到 map ----------
            skill_event_map.emplace(skillInfo.skill_id, std::move(skillInfo));
        }

        return true;  // 成功读取所有技能信息
    }

    const StSkillEventInfo* StaticData::get_skill_event_info(uint32_t skill_id) const noexcept {
        auto iter = skill_event_map.find(skill_id);
        if (iter != skill_event_map.end()) {
            return &iter->second;
        }
        return nullptr;
    }

    bool StaticData::read_combo_skill_info(db::SqlData &query) {
        combo_skill_map.clear();
        while(query.next()){
            StComboSkillInfo info;
            info.skill_id = query.get_int32("SkillId");
            int32_t v = query.get_int32("Combo1");
            if(v != 0){
                info.combo_skills.emplace_back(v);
            }
            v = query.get_int32("Combo2");
            if(v != 0){
                info.combo_skills.emplace_back(v);
            }
            v = query.get_int32("Combo3");
            if(v != 0){
                info.combo_skills.emplace_back(v);
            }
            v = query.get_int32("Combo4");
            if(v != 0){
                info.combo_skills.emplace_back(v);
            }
            combo_skill_map.emplace(info.skill_id, std::move(info));
        }
        return true;
    }

    const StComboSkillInfo* StaticData::get_combo_skill_info(uint32_t skill_id) const noexcept {
        auto iter = combo_skill_map.find(skill_id);
        if (iter != combo_skill_map.end()) {
            return &iter->second;
        }
        return nullptr;
    }

    bool StaticData::read_buff_info(db::SqlData &query) {
        // todo
        buff_map.clear();
        while(query.next()){
            StBuffInfo info;
            info.buff_id = query.get_int32("Id");
            buff_map.emplace(info.buff_id, std::move(info));
        }
        return true;
    }

    const StBuffInfo* StaticData::get_buff_info(uint32_t buff_id) const noexcept {
        auto iter = buff_map.find(buff_id);
        if (iter != buff_map.end()) {
            return &iter->second;
        }
        return nullptr;
    }

    bool StaticData::read_bullet_info(db::SqlData &query) {
        bullet_map.clear();
        while(query.next()){
            StBulletInfo info;
            info.bullet_id = query.get_int32("Id");
            info.bullet_type = static_cast<EBulletType>(query.get_int32("Type"));
            info.init_speed = query.get_float("InitSpeed");
            info.acc_speed = query.get_float("AcceSpeed");
            info.life_time = query.get_int32("LifeTime");
            info.range_type = static_cast<ERangeType>(query.get_int32("RangeType"));
            string_to_vector(query.get_string("RangeParams").data(), info.range_params, '~');
            bullet_map.emplace(info.bullet_id, info);
        }
        return true;
    }

    const StBulletInfo* StaticData::get_bullet_info(uint32_t bullet_id) const noexcept {
        auto iter = bullet_map.find(bullet_id);
        if (iter != bullet_map.end()) {
            return &iter->second;
        }
        return nullptr;
    }

    bool StaticData::read_charge_info(db::SqlData &query) {
        charge_map.clear();
        while(query.next()){
            StChargeInfo info;
            info.product_id = query.get_int32("Id");
            charge_map.emplace(info.product_id, info);
        }
        return true;
    }
    const StChargeInfo* StaticData::get_charge_info(uint32_t product_id) const noexcept {
        auto iter = charge_map.find(product_id);
        if (iter != charge_map.end()) {
            return &iter->second;
        }
        return nullptr;
    }
}