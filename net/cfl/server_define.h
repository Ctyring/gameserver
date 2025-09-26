#pragma once

#include <cstdint>  // 使用标准整型别名 uint32_t

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
}
