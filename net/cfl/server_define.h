#pragma once

#include <cstdint>  // 使用标准整型别名 uint32_t
#include "cfl.h"
#include "protos/gen_proto/define.pb.h"

namespace cfl {
    // -------------------------
    // 战斗记录
    // -------------------------
    struct BattleRecord {
        uint32_t result;   // 战斗结果: 0=未结算, 1=胜利, 2=失败, 3=平局
        uint32_t kill;     // 击杀数
        uint32_t death;    // 被杀次数
        uint32_t heal;     // 治疗量
        uint32_t damage;   // 总伤害值
    };

    // -------------------------
    // 属性变更类型
    // -------------------------
    enum class ChangeType : uint32_t {
        equip = 1,  // 装备改变
        mount = 2,  // 坐骑改变
        level = 3,  // 等级提升
        partner = 4,  // 伙伴变化
        pet = 5   // 宠物变化
    };

    // -------------------------
    // 胜利条件
    // -------------------------
    enum class WinCondition : uint32_t {
        none,            // 无条件
        kill_all,        // 击杀全部怪物
        kill_num,        // 击杀指定数量怪物
        destination,     // 达到目的地
        player_alive,    // 玩家存活
        npc_alive,       // 护送 NPC 存活
        end              // 结束标记
    };

    // -------------------------
    // 战斗阵营
    // -------------------------
    enum class BattleCamp : uint32_t {
        none,      // 中立阵营
        player,    // 玩家阵营
        monster    // 怪物阵营
    };

    // -------------------------
    // 触发类型
    // -------------------------
    enum class TriggerType : uint32_t {
        normal,    // 直接触发
        tribox,    // 触发盒触发
        time       // 时间触发
    };

    /// @brief 常量值结构
    struct StConstantValue {
        /// @brief 默认构造函数
        StConstantValue() = default;

        /// @brief 常量值
        uint32_t xxxx = 0;
    };

/// @brief 副本信息
    struct StCopyInfo {
        /// @brief 副本ID
        uint32_t copy_id = 0;

        /// @brief 副本类型
        uint32_t copy_type = 0;

        /// @brief 消耗体力ID
        uint32_t cost_act_id = 0;

        /// @brief 消耗体力数量
        uint32_t cost_act_num = 0;

        /// @brief 获取货币ID
        uint32_t get_money_id = 0;

        /// @brief 获取货币系数
        uint32_t get_money_ratio = 0;

        /// @brief 获取经验系数
        uint32_t get_exp_ratio = 0;

        /// @brief 奖励ID
        uint32_t award_id = 0;

        /// @brief 首胜奖励ID
        uint32_t first_award_id = 0;

        /// @brief 每日可战斗次数
        uint32_t battle_times = 0;

        /// @brief 对应的XML字符串
        std::string xml;
    };

/// @brief 本地化字符串结构
    struct StLocalString {
        /// @brief 本地化ID
        uint32_t id = 0;

        /// @brief 语言字符串数组
        std::array<std::string, MaxLanguageNum> language;
    };

/// @brief 角色信息
    struct StActorInfo {
        /// @brief 角色ID
        uint32_t id = 0;

        /// @brief 角色碰撞半径
        float radius = 0.0f;

        /// @brief 默认速度
        float default_speed = 0.0f;

        /// @brief 角色类型
        int32_t type = 0;

        /// @brief 初始等级
        int32_t init_level = 0;

        /// @brief AI配置ID
        int32_t ai_id = 0;

        /// @brief 属性数组
        std::array<int32_t, PropertyNum> properties{};

        /// @brief 角色名称
        std::string name;
    };

/// @brief 角色技能信息
    struct StActorSkillInfo {
        /// @brief 角色ID
        uint32_t actor_id = 0;

        /// @brief 普攻技能ID
        int32_t normal_id = 0;

        /// @brief 特殊技能ID数组
        std::array<int32_t, 5> specials{};
    };

/// @brief 充值信息
    struct StChargeInfo {
        /// @brief 商品ID
        uint32_t product_id = 0;
    };

/// @brief 职业信息
    struct StCarrerInfo {
        /// @brief 职业ID
        uint32_t id = 0;

        /// @brief 角色ID
        uint32_t actor_id = 0;

        /// @brief 出生城市ID
        uint32_t born_city = 0;

        /// @brief 职业名称
        std::string name;
    };

/// @brief 等级信息
    struct StLevelInfo {
        /// @brief 职业ID
        uint32_t carrer_id = 0;

        /// @brief 等级
        uint32_t level = 0;

        /// @brief 所需经验
        uint32_t need_exp = 0;

        /// @brief 属性数组
        std::array<int32_t, PropertyNum> properties{};

        /// @brief 战斗力
        uint32_t fight_value = 0;
    };

/// @brief 行动力信息
    struct StActionInfo {
        /// @brief 行动力ID
        uint32_t action_id = 0;

        /// @brief 恢复1个单位所需时间（秒）
        int32_t unit_time = 0;

        /// @brief 最大值
        uint32_t max_value = 0;
    };

/// @brief 物品数据
    struct StItemData {
        /// @brief 物品ID
        uint32_t item_id = 0;

        /// @brief 物品数量
        uint32_t item_num = 0;
    };

    /// @brief 掉落项
    struct StDropItem {
        /// @brief 物品ID
        uint32_t item_id = 0;

        /// @brief 掉落物品数量范围（最小值、最大值）
        std::array<uint32_t, 2> item_num{};

        /// @brief 掉落概率范围
        uint32_t ratio = 0;
    };

/// @brief 奖励项
    struct StAwardItem {
        /// @brief 奖励ID
        int32_t award_id = 0;

        /// @brief 概率掉落个数
        int32_t ratio_count = 0;

        /// @brief 是否需要去重
        bool distinct = false;

        /// @brief 对应职业（0为通用）
        int32_t carrer = 0;

        /// @brief 必定掉落的物品
        std::vector<StDropItem> fix_items;

        /// @brief 概率掉落的物品
        std::vector<StDropItem> ratio_items;
    };

/// @brief 物品信息
    struct StItemInfo {
        /// @brief 物品ID
        uint32_t item_id = 0;

        /// @brief 物品类型
        ItemType item_type{};

        /// @brief 背包类型
        uint32_t bag_type = 0;

        /// @brief 物品品质
        uint32_t quality = 0;

        /// @brief 出售货币ID
        uint32_t sell_id = 0;

        /// @brief 出售价格
        int32_t sell_price = 0;

        /// @brief 使用类型
        uint32_t use_type = 0;

        /// @brief 职业限制
        uint32_t carrer_id = 0;

        /// @brief 最大堆叠数
        int64_t stack_max = 0;

        /// @brief 参数1
        int32_t data1 = 0;

        /// @brief 参数2
        int32_t data2 = 0;
    };

/// @brief 功能开放信息
    struct StFuncInfo {
        /// @brief 功能ID
        uint32_t func_id = 0;

        /// @brief 功能名
        std::string name;

        /// @brief 开放等级
        int32_t open_level = 0;

        /// @brief VIP提前开放等级
        int32_t vip_level = 0;

        /// @brief 逻辑关系
        /// @details 1: 两者取其一；2: 两者都必须；
        /// 3: 满足VIP条件；4: 满足等级条件
        int32_t logic = 0;
    };

/// @brief 功能VIP信息
    struct StFuncVipInfo {
        /// @brief 功能ID
        uint32_t func_id = 0;

        /// @brief 各VIP等级对应数值
        std::array<int32_t, MaxVipLevel> vip_value{};
    };

/// @brief 装备信息
    struct StEquipInfo {
        /// @brief 装备ID
        uint32_t equip_id = 0;

        /// @brief 套装ID
        uint32_t suit_id = 0;

        /// @brief 装备部位
        uint32_t pos = 0;
    };

/// @brief 宝石信息
    struct StGemInfo {
        /// @brief 宝石ID
        uint32_t gem_id = 0;

        /// @brief 镶嵌部位
        uint32_t pos = 0;
    };

/// @brief 宠物信息
    struct StPetInfo {
        /// @brief 宠物ID
        uint32_t pet_id = 0;

        /// @brief 对应角色ID
        uint32_t actor_id = 0;
    };

/// @brief 伙伴信息
    struct StPartnerInfo {
        /// @brief 伙伴ID
        uint32_t partner_id = 0;

        /// @brief 对应角色ID
        uint32_t actor_id = 0;
    };

/// @brief 坐骑信息
    struct StMountInfo {
        /// @brief 坐骑ID
        uint32_t mount_id = 0;

        /// @brief 对应角色ID
        uint32_t actor_id = 0;
    };

/// @brief 任务信息
    struct StTaskInfo {
        /// @brief 任务ID
        uint32_t task_id = 0;

        /// @brief 前置任务ID
        uint32_t prev_task_id = 0;

        /// @brief 任务类型（日常、主线、分支、公会等）
        uint32_t task_type = 0;

        /// @brief 需要达成的条件数
        uint32_t need_count = 0;

        /// @brief 任务事件类型
        uint32_t task_event = 0;

        /// @brief 开放等级
        int32_t need_level = 0;

        /// @brief 任务奖励ID
        uint32_t award_id = 0;
    };

/// @brief 商店物品信息
    struct StStoreItemInfo {
        /// @brief 商品ID
        uint32_t store_id = 0;

        /// @brief 物品ID
        uint32_t item_id = 0;

        /// @brief 物品数量
        uint32_t item_num = 0;

        /// @brief 消耗货币ID
        uint32_t cost_money_id = 0;

        /// @brief 消耗货币数量
        uint32_t cost_money_num = 0;

        /// @brief 商店类型
        uint32_t store_type = 0;
    };

/// @brief 活动信息
    struct StActivityInfo {
        /// @brief 活动ID
        uint32_t activity_id = 0;

        /// @brief 活动名称
        std::string name;

        /// @brief 活动描述
        std::string desc;

        /// @brief 活动广告或展示图片
        std::string ad;

        /// @brief 活动状态
        uint32_t status = 0;

        /// @brief 活动类型
        uint32_t activity_type = 0;

        /// @brief 奖励索引
        uint32_t award_index = 0;

        /// @brief 开始时间（时间戳）
        uint32_t start_time = 0;

        /// @brief 结束时间（时间戳）
        uint32_t end_time = 0;
    };

/// @brief 技能中心点类型
    enum class ECenterType : uint32_t
    {
        TargetPos     = 1, ///< 以客户端选定的位置为中心
        TargetObj     = 2, ///< 以客户端选定的对象为中心
        CasterPos     = 3, ///< 以施法者为中心
        CasterOffset  = 4  ///< 以施法者为中心的偏移位置为中心
    };

/// @brief 技能施放类型
    enum class ESkillCastType : uint32_t
    {
        Instant       = 0, ///< 瞬发（无选择）
        Target        = 1, ///< 目标选取辅助（目标）
        Direction     = 2, ///< 方向贴花辅助（朝向）
        Position      = 3, ///< 地面贴花辅助（AOE区域）
        DirectionArc  = 4, ///< 扇形方向贴花辅助（朝向）
        Effect        = 5, ///< 模型特效辅助（召唤）
        Sight         = 6, ///< 准星UI变化辅助（射击）
        Parabola      = 7, ///< 抛物线辅助（投掷）
        DragLine      = 8, ///< 拉线辅助（墙类召唤）
        Gesture       = 9  ///< 鼠标手势辅助（多段线条传入点）
    };

/// @brief 技能作用范围类型
    enum class ERangeType : uint32_t
    {
        Objects   = 1, ///< 客户端指定的目标
        Cylinder  = 2, ///< 扇形圆柱
        Circle    = 3, ///< 圆形圆柱
        Box       = 4  ///< 矩形区域
    };

/// @brief 子弹类型
    enum class EBulletType : uint32_t
    {
        Chase         = 0, ///< 追踪型飞弹
        FixDirection  = 1, ///< 固定方向型飞弹
        FixTargetPos  = 2, ///< 固定目标点飞弹
        Point         = 3, ///< 固定点飞弹
        Link          = 4, ///< 连接飞弹
        Annular       = 5, ///< 环形飞弹
        Back          = 6, ///< 回旋飞弹
        Extract       = 7, ///< 抽取飞弹
        Bounce        = 8, ///< 弹跳飞弹
        Wave          = 9  ///< 冲击波
    };

/// @brief 技能状态
    enum class ESkillStatus : uint32_t
    {
        Init      = 0, ///< 初始状态
        Running   = 1, ///< 正在运行
        Finished  = 2  ///< 已完成
    };

/// @brief 技能影响关系
    enum class EAffectShip : uint32_t
    {
        Self = 0, ///< 影响自己
        Enemy = 1, ///< 影响敌方
        Ally = 2, ///< 影响友方
        Each = 3  ///< 影响所有
    };

/// @brief 目标选择策略
    enum class ESelectPolicy : uint32_t
    {
        Default         = 0, ///< 默认
        ByMoreHealth    = 1, ///< 按血量比例最高
        ByLessHealth    = 2, ///< 按血量比例最低
        ByMoreDistance  = 3, ///< 按距离比例最高
        ByLessDistance  = 4  ///< 按距离比例最低
    };

/// @brief 技能类型
    enum class ESkillType : uint32_t
    {
        None     = 0, ///< 无
        Physics  = 1, ///< 物理伤害
        Magic    = 2  ///< 法术伤害
    };

/// @brief Buff信息结构
    struct StBuffInfo
    {
        /// @brief Buff唯一ID
        uint32_t buff_id = 0;

        /// @brief Buff目标特效ID
        uint32_t effect_id = 0;

        /// @brief 属性固定加值
        std::array<int32_t, PropertyNum> property_value{};

        /// @brief 属性百分比加值
        std::array<int32_t, PropertyNum> property_percent{};

        /// @brief 总时长（毫秒）
        int32_t total_time = 0;

        /// @brief 间隔时间（毫秒）
        int32_t interval = 0;

        /// @brief 改变的角色状态标志
        uint32_t change_status = 0;

        /// @brief 是否可叠加
        bool overlay = false;

        /// @brief Lua添加事件函数名
        std::string lua_add;

        /// @brief Lua周期触发函数名
        std::string lua_tick;

        /// @brief Lua移除事件函数名
        std::string lua_remove;
    };

/// @brief 子弹信息结构
    struct StBulletInfo
    {
        /// @brief 子弹ID
        uint32_t bullet_id = 0;

        /// @brief 子弹类型
        EBulletType bullet_type = EBulletType::FixDirection;

        /// @brief 初始速度
        float init_speed = 0.0f;

        /// @brief 加速度
        float acc_speed = 0.0f;

        /// @brief 范围类型
        ERangeType range_type = ERangeType::Circle;

        /// @brief 范围参数
        std::array<float, 5> range_params{};

        /// @brief 生命周期（毫秒）
        uint32_t life_time = 0;

        /// @brief 受击动作ID（击退/击飞/击倒）
        uint32_t hit_action_id = 0;

        /// @brief 受击特效ID
        uint32_t hit_effect = 0;

        /// @brief 受击移动距离
        float hit_distance = 0.0f;
    };

/// @brief 子弹对象结构
    struct StBulletObject
    {
        /// @brief 子弹ID
        uint32_t bullet_id = 0;

        /// @brief 子弹发射角度
        float angle = 0.0f;
    };

/// @brief 召唤物（妖精）信息
    struct StGoblinInfo
    {
        /// @brief 角色ID
        uint32_t actor_id = 0;
    };

/// @brief 技能事件
    struct StSkillEvent
    {
        /// @brief 触发时间（毫秒）
        uint64_t trigger_time = 0;

        /// @brief 自身Buff ID
        uint32_t self_buff_id = 0;

        /// @brief 目标Buff ID
        uint32_t target_buff_id = 0;

        /// @brief 范围参数
        std::array<float, 5> range_params{};

        /// @brief 范围类型
        ERangeType range_type = ERangeType::Objects;

        /// @brief 中心点类型
        uint32_t center_type = 0;

        /// @brief 子弹列表
        std::vector<StBulletObject> bullets;

        /// @brief 召唤妖精列表
        std::vector<StGoblinInfo> goblins;

        /// @brief 受击动作ID
        uint32_t hit_action_id = 0;

        /// @brief 受击特效ID
        uint32_t hit_effect = 0;

        /// @brief 受击移动距离
        float hit_distance = 0.0f;
    };

/// @brief 技能基础信息
    struct StSkillInfo
    {
        /// @brief 技能ID
        uint32_t skill_id = 0;

        /// @brief 技能等级
        uint32_t level = 0;

        /// @brief 技能类型（物理/法术）
        uint32_t skill_type = 0;

        /// @brief 作用目标关系类型
        HitShipType hit_ship_type = HitShipType::hit_all;

        /// @brief 是否作用于自身
        bool hit_myself = false;

        /// @brief 技能冷却时间（毫秒）
        uint32_t cd = 0;

        /// @brief 技能持续总时间（毫秒）
        uint64_t duration = 0;

        /// @brief 固定伤害值
        int32_t hurt_fix = 0;

        /// @brief 加成伤害系数
        int32_t hurt_multi = 0;
    };

/// @brief 技能事件信息
    struct StSkillEventInfo
    {
        /// @brief 技能ID
        uint32_t skill_id = 0;

        /// @brief 技能持续总时间
        uint64_t duration = 0;

        /// @brief 目标选择方式
        uint32_t cast_type = 0;

        /// @brief 技能事件列表
        std::vector<StSkillEvent> events;
    };

    /// @brief 连击技能信息
    struct StComboSkillInfo
    {
        /// @brief 主技能ID
        uint32_t skill_id = 0;

        /// @brief 连击技能ID列表
        std::vector<uint32_t> combo_skills;
    };

}
