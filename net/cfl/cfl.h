#pragma once

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include "cfl/shm/shmobj.h"
#include <cstddef>       // for std::size_t
#include <cstdint>       // for uint32_t 等
#include "config.h"

#if defined(_WIN32) || defined(_WIN64)
#ifdef CFL_EXPORTS
#define CFL_API __declspec(dllexport)
#else
#define CFL_API __declspec(dllimport)
#endif
#else
#define CFL_API __attribute__((visibility("default")))
#endif


namespace cfl {

    // 货币/行动力/属性/等级
    inline constexpr std::size_t MoneyNum = 15;
    inline constexpr std::size_t ActionNum = 4;
    inline constexpr std::size_t PropertyNum = 21;
    inline constexpr std::size_t MaxRoleLevel = 150;

    // 服务器帧率
    inline constexpr std::size_t FpsTimeTick = 20;

    // 公会
    inline constexpr std::size_t GuildNameLen = 255;
    inline constexpr std::size_t GuildNoticeLen = 1024;
    inline constexpr std::size_t GuildMaxApply = 50;

    // 订单
    inline constexpr std::size_t PayOrderIdLen = 128;

    // 多语言
    inline constexpr std::size_t MaxLanguageNum = 15;

    // VIP
    inline constexpr std::size_t MaxVipLevel = 20;

    // SQL
    inline constexpr std::size_t SqlBufferLen = 1024;

    // 邮件
    inline constexpr std::size_t MailContentLen = 2048;
    inline constexpr std::size_t MailTitleLen = 255;
    inline constexpr std::size_t MailItemCount = 10;

    // 角色名
    inline constexpr std::size_t RoleNameLen = 255;

    // 装备/伙伴
    inline constexpr std::size_t EquipMaxNum = 8;
    inline constexpr std::size_t PartnerMaxNum = 2;

    // 基础速度
    inline constexpr float SpeedIdle = 0.0f;
    inline constexpr float SpeedWalk = 2.5f;
    inline constexpr float SpeedRun = 4.0f;
    inline constexpr float SpeedFly = 6.5f;

    // 模块类型
    enum class ModuleType : std::uint8_t {
        Role,
        Copy,
        Bag,
        Equip,
        Gem,
        Pet,
        Partner,
        Task,
        Mount,
        Activity,
        Counter,
        Store,
        Skill,
        Mail,
        Friend,
        End
    };

    // 共享数据类型
    enum class ShareData : std::uint8_t {
        Begin,
        Role,
        Global,
        Bag,
        Copy,
        Equip,
        Gem,
        Pet,
        Partner,
        Guild,
        GuildMember,
        Task,
        Mount,
        Mail,
        OffData,         // 离线操作
        GroupMail,
        Activity,
        Counter,
        Friend,
        Chapter,
        Skill,
        SealRole,
        Payment,
        End
    };

    // 增加方式
    enum class AddWay : std::uint8_t {
        GmCommand = 1,
        Add = 2,
        Cost = 3
    };

    // 活动类型
    enum class ActivityType : std::uint8_t {
        None = 0,
        LoginAward = 1,
        SingleCharge = 2,
        SumCharge = 3,
        OpenFund = 4,
        DiscountSale = 5
    };

    // 角色属性
    enum class RoleProperty : std::uint8_t {
        Id = 1,
        Level,
        VipLevel,
        Exp,
        Channel
    };

}// namespace cfl
