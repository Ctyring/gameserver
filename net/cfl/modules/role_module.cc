#include "role_module.h"
#include "cfl/shm/shmpool.h"
namespace cfl{
    RoleModule::RoleModule(PlayerObjPtr owner)
        : ModuleBase(std::move(owner))
    {
        register_message_handler();
    }

    bool RoleModule::on_create(std::uint64_t role_id) {
        // todo: 获取玩家数据
        if(role_data_object_ == nullptr){
            return false;
        }

        role_data_object_->lock();
        role_data_object_->level = 1;
        for(int i = 0; i < ActionNum; i++){
            // todo: 初始化每个技能
        }
        // todo: 初始化玩家数据

        role_data_object_->unlock();
        return true;
    }

    bool RoleModule::on_destroy() {
        role_data_object_->release();
        role_data_object_.reset();
        return true;
    }

    bool RoleModule::on_login() {
        role_data_object_->lock();
        for(int i = 0; i < ActionNum; i++){
            update_action(i + 1);
        }

        // 更新登录和下线时间
        if(role_data_object_->logoffTime < role_data_object_->logonTime){
            // 处理异常数据
            role_data_object_->logoffTime = role_data_object_->logonTime + 1;
            // todo: 重新设置登出时间
        }

        role_data_object_->logonTime = cfl::get_timestamp();
        role_data_object_->unlock();
        return true;
    }

    bool RoleModule::on_logout() {
        if(!role_data_object_){
            return false;
        }
        role_data_object_->lock();
        role_data_object_->logoffTime = cfl::get_timestamp();
        role_data_object_->onlineTime += role_data_object_->logoffTime - role_data_object_->logonTime;
        role_data_object_->unlock();
        return true;
    }

    bool RoleModule::on_new_day() {
        role_data_object_->lock();
        role_data_object_->logoffTime = cfl::get_timestamp() + 1;
        role_data_object_->unlock();
        return true;
    }

    bool RoleModule::init_base_data(std::uint64_t role_id, std::string_view name, std::uint32_t career_id,
                                    std::uint64_t account_id, std::int32_t channel) {
        role_data_object_ = shm::create_object<shm::RoleDataObject>(shm::SHMTYPE::RoleData, true);
        role_data_object_->lock();
        role_data_object_->roleId = role_id;
        role_data_object_->accountId = account_id;
        role_data_object_->carrerId = career_id;
        role_data_object_->channel = channel;
        // 复制name
        strcpy_s(role_data_object_->name, name.data());
        role_data_object_->langId = 0;
        role_data_object_->unlock();
        // todo: 从静态数据里面获取相关信息
        return true;
    }

    bool RoleModule::read_from_db_login_data(const DBRoleLoginAck &ack) {
        role_data_object_ = shm::create_object<shm::RoleDataObject>(shm::SHMTYPE::RoleData, false);
        role_data_object_->lock();
        role_data_object_->roleId = ack.role_data().role_id();
        role_data_object_->accountId = ack.role_data().account_id();
        strcpy_s(role_data_object_->name, ack.role_data().name().data());
        role_data_object_->langId = ack.role_data().lang_id();
        role_data_object_->carrerId = ack.role_data().career_id();
        role_data_object_->level = ack.role_data().level();
        role_data_object_->exp = ack.role_data().exp();
        role_data_object_->vipLevel = ack.role_data().vip_level();
        role_data_object_->vipExp = ack.role_data().vip_exp();
        role_data_object_->cityCopyId = ack.role_data().city_copy_id();
        role_data_object_->guildId = ack.role_data().guild_id();
        role_data_object_->createTime = ack.role_data().create_time();
        role_data_object_->logonTime = ack.role_data().logon_time();
        role_data_object_->logoffTime = ack.role_data().logoff_time();
        role_data_object_->channel = ack.role_data().channel();
        role_data_object_->onlineTime = ack.role_data().online_time();
        if(role_data_object_->cityCopyId == 0){
            // todo: 初始化出生城市
        }

        for(int i = 0; i < ActionNum; i++){
            role_data_object_->action[i] = ack.role_data().action(i);
            role_data_object_->actime[i] = ack.role_data().action_time(i);
        }

        role_data_object_->unlock();

        // todo: 从静态数据里面读取信息
        return true;
    }

    bool RoleModule::save_to_client_login_data(RoleLoginAck &ack) {
        ack.set_account_id(role_data_object_->accountId);
        ack.set_role_id(role_data_object_->roleId);
        ack.set_name(role_data_object_->name);
        ack.set_level(role_data_object_->level);
        ack.set_exp(role_data_object_->exp);
        ack.set_vip_lvl(role_data_object_->vipLevel);
        ack.set_vip_exp(role_data_object_->vipExp);
        ack.set_carrer(role_data_object_->carrerId);
        ack.set_fight_value(role_data_object_->fightValue);
        for(int i = 0; i < ActionNum; i++){
            auto* p = ack.add_action_list();
            p->set_action(role_data_object_->action[i]);
            p->set_actime(role_data_object_->actime[i]);
        }
        return true;
    }

    bool RoleModule::notify_change() {
        return true;
    }

    bool RoleModule::calc_fight_value(const std::array<int32_t, PropertyNum> &value,
                                      const std::array<int32_t, PropertyNum> &percent, int32_t &fight_value) {
        return true;
    }

    void RoleModule::register_message_handler() {}

    std::uint32_t RoleModule::get_property(cfl::RoleProperty property_id) const {
        switch (property_id) {
            case cfl::RoleProperty::Id:
                return role_data_object_->roleId;
            case cfl::RoleProperty::Level:
                return role_data_object_->level;
            case cfl::RoleProperty::Exp:
                return role_data_object_->exp;
            case cfl::RoleProperty::VipLevel:
                return role_data_object_->vipLevel;
            case cfl::RoleProperty::Channel:
                return role_data_object_->channel;
            default:
                return 0;
        }
    }

    bool RoleModule::cost_action(std::uint32_t action_id, int32_t action_num) {
        if(action_id <= 0 || action_id >= ActionNum){
            spdlog::error("cost_action error, action_id: {}", action_id);
            return false;
        }
        if(action_num <= 0){
            spdlog::error("cost_action error, action_num: {}", action_num);
            return false;
        }

        if(role_data_object_->action[action_id - 1] < action_num){
            return false;
        }

        role_data_object_->lock();
        role_data_object_->action[action_id - 1] -= action_num;
        // todo: 小于最大行动力的时候要计算CD
        role_data_object_->unlock();
        return true;
    }

    bool RoleModule::check_action_enough(std::uint32_t action_id, int32_t action_num) {
        if (action_id == 0 || action_id > ActionNum) {
            spdlog::error("check_action_enough error, invalid action_id: {}", action_id);
            return false;
        }
        if (action_num <= 0) {
            spdlog::error("check_action_enough error, invalid action_num: {}", action_num);
            return false;
        }

        // role_data_object_ 必须存在
        if (!role_data_object_) {
            spdlog::error("check_action_enough error, role_data_object_ is null");
            return false;
        }

        update_action(action_id);
        return role_data_object_->action[action_id - 1] >= static_cast<std::uint64_t>(action_num);
    }

    std::uint64_t RoleModule::get_action(std::uint32_t action_id) {
        if (action_id == 0 || action_id > ActionNum) {
            spdlog::error("get_action error, invalid action_id: {}", action_id);
            return 0;
        }
        if (!role_data_object_) {
            spdlog::error("get_action error, role_data_object_ is null");
            return 0;
        }

        update_action(action_id);
        return role_data_object_->action[action_id - 1];
    }

    std::uint64_t RoleModule::add_action(std::uint32_t action_id, std::int64_t action_num) {
        if (action_id == 0 || action_id > ActionNum) {
            spdlog::error("add_action error, invalid action_id: {}", action_id);
            return 0;
        }
        if (action_num == 0) {
            // nothing to add
            return role_data_object_ ? role_data_object_->action[action_id - 1] : 0;
        }
        if (!role_data_object_) {
            spdlog::error("add_action error, role_data_object_ is null");
            return 0;
        }

        role_data_object_->lock();
        // 先刷新行动力
        update_action(action_id);

        // 增加（允许负数用于回退）
        if (action_num > 0) {
            role_data_object_->action[action_id - 1] += static_cast<std::uint64_t>(action_num);
        } else {
            // action_num < 0，谨慎处理避免无符号下溢
            std::uint64_t cur = role_data_object_->action[action_id - 1];
            std::int64_t absv = -action_num;
            if (static_cast<std::uint64_t>(absv) >= cur) {
                role_data_object_->action[action_id - 1] = 0;
            } else {
                role_data_object_->action[action_id - 1] = cur - static_cast<std::uint64_t>(absv);
            }
        }

        // todo: 超过最大值则设置为最大并把计时置 0（与原实现一致）
//        std::uint64_t maxv = CStaticData::GetInstancePtr()->GetActoinMaxValue(action_id);
//        if (role_data_object_->action[action_id - 1] >= maxv) {
//            role_data_object_->action[action_id - 1] = maxv;
//            role_data_object_->actime[action_id - 1] = 0;
//        }

        std::uint64_t ret = role_data_object_->action[action_id - 1];
        role_data_object_->unlock();
        return ret;
    }

    bool RoleModule::update_action(std::uint32_t action_id) {
        if (!role_data_object_) {
            spdlog::error("update_action error, role_data_object_ is null");
            return false;
        }
        if (action_id == 0 || action_id > ActionNum) {
            spdlog::error("update_action error, invalid action_id: {}", action_id);
            return false;
        }

        // todo: 如果已经满，则确保 start time 为 0 并直接返回 false（无变化）
//        std::uint64_t maxv = CStaticData::GetInstancePtr()->GetActoinMaxValue(action_id);
//        if (role_data_object_->action[action_id - 1] >= maxv) {
//            if (role_data_object_->actime[action_id - 1] != 0) {
//                spdlog::warn("update_action: action is max but actime is not 0 (action_id={})", action_id);
//            }
//            role_data_object_->actime[action_id - 1] = 0;
//            return false;
//        }

        // 如果 start time 为 0，说明计时未开始，直接返回
        if (role_data_object_->actime[action_id - 1] == 0) {
            // 没有启动计时（可能从 DB 下发的为 0 或尚未开始恢复）
            return false;
        }

        std::uint64_t now = cfl::get_timestamp();
        std::uint64_t elapsed = now - role_data_object_->actime[action_id - 1];

        // todo: 获取单位时间
//        std::uint64_t unit = CStaticData::GetInstancePtr()->GetActoinUnitTime(action_id);
//        if (unit == 0) {
//            spdlog::error("update_action error, unit time is 0 for action_id: {}", action_id);
//            return false;
//        }

//        if (elapsed < unit) {
//            return false; // 还没到恢复一个单位
//        }

//        std::uint32_t addNum = static_cast<std::uint32_t>(elapsed / unit);
//        role_data_object_->action[action_id - 1] += addNum;

//        if (role_data_object_->action[action_id - 1] >= maxv) {
//            role_data_object_->action[action_id - 1] = maxv;
//            role_data_object_->actime[action_id - 1] = 0;
//        } else {
//             更新起始时间到上次补满的后续时间点（避免重复计入）
//            role_data_object_->actime[action_id - 1] = role_data_object_->actime[action_id - 1] + static_cast<std::uint64_t>(addNum) * unit;
//        }

        return true;
    }

    bool RoleModule::set_delete(bool is_delete) {
        if (!role_data_object_) {
            spdlog::error("set_delete error, role_data_object_ is null");
            return false;
        }
        role_data_object_->lock();
        role_data_object_->isDeleted = is_delete; // 假定字段名为 isDelete；若你真实字段名不同请改为相应字段
        role_data_object_->unlock();
        return true;
    }

    std::uint64_t RoleModule::add_exp(int32_t exp) {
        if (!role_data_object_) {
            spdlog::error("add_exp error, role_data_object_ is null");
            return 0;
        }
        if (exp <= 0) {
            return role_data_object_->exp;
        }

        role_data_object_->lock();
        role_data_object_->exp += static_cast<std::uint64_t>(exp);
        role_data_object_->unlock();

        // todo: 升级循环（可能多级）
//        while (role_data_object_->level < MAX_ROLE_LEVEL) {
//            StLevelInfo* pLevelInfo = CStaticData::GetInstancePtr()->GetCarrerLevelInfo(role_data_object_->carrerId, role_data_object_->level + 1);
//            if (pLevelInfo == nullptr) {
//                spdlog::error("add_exp: missing level info for career {} level {}", role_data_object_->carrerId, role_data_object_->level + 1);
//                break;
//            }

//            if (role_data_object_->exp >= pLevelInfo->dwNeedExp) {
//                role_data_object_->lock();
//                role_data_object_->exp -= pLevelInfo->dwNeedExp;
//                role_data_object_->level += 1;
//                role_data_object_->unlock();
//            } else {
//                break;
//            }
//        }

        return role_data_object_->exp;
    }

    std::uint64_t RoleModule::get_last_logoff_time() const {
        if (!role_data_object_) {
            return 0;
        }
        return role_data_object_->logoffTime;
    }

    bool RoleModule::set_last_logoff_time(std::uint64_t time) {
        if (!role_data_object_) {
            spdlog::error("set_last_logoff_time error, role_data_object_ is null");
            return false;
        }
        role_data_object_->lock();
        role_data_object_->logoffTime = time;
        role_data_object_->unlock();
        return true;
    }

    std::uint64_t RoleModule::get_last_logon_time() const {
        if (!role_data_object_) {
            return 0;
        }
        return role_data_object_->logonTime;
    }

    std::uint64_t RoleModule::get_create_time() const {
        if (!role_data_object_) {
            return 0;
        }
        return role_data_object_->createTime;
    }

    std::uint32_t RoleModule::get_online_time() const {
        if (!role_data_object_) {
            return 0;
        }
        return static_cast<std::uint32_t>(role_data_object_->onlineTime);
    }

    void RoleModule::set_group_mail_time(std::uint64_t time) {
        if (!role_data_object_) {
            spdlog::error("set_group_mail_time error, role_data_object_ is null");
            return;
        }
        role_data_object_->lock();
        role_data_object_->groupMailTime = time; // 假定字段名为 groupMailTime，若字段名不同请调整
        role_data_object_->unlock();
    }

    std::uint64_t RoleModule::get_group_mail_time() const {
        if (!role_data_object_) {
            return 0;
        }
        return role_data_object_->groupMailTime; // 若字段名不同请调整
    }

    std::uint32_t RoleModule::get_actor_id() const {
        return actor_id_;
    }

    uint32_t RoleModule::get_level() const {
        if (!role_data_object_) return 0;
        return role_data_object_->level;
    }

    int32_t RoleModule::get_vip_level() const {
        if (!role_data_object_) return 0;
        return role_data_object_->vipLevel;
    }

    std::string RoleModule::get_name() const {
        if (!role_data_object_) return std::string();
        return std::string(role_data_object_->name);
    }

    std::uint32_t RoleModule::get_career_id() const {
        if (!role_data_object_) return 0;
        return role_data_object_->carrerId;
    }

    std::uint64_t RoleModule::get_role_id() const {
        if (!role_data_object_) return 0;
        return role_data_object_->roleId;
    }
} // namespace cfl