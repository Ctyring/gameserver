#pragma once
#include "cfl/cfl.h"
#include "module_base.h"
#include "cfl/shm/obj/role_data_obj.h"
#include "cfl/playerobj.h"
#include <string>
#include <cstdint>
#include <array>
#include <memory>

namespace cfl {
    namespace shm {
        /**
         * @brief 共享内存中的角色数据对象
         */
        struct RoleDataObject;
    }

    /**
     * @class RoleModule
     * @brief 角色模块，负责管理角色的基础属性、登录登出逻辑、数据同步等。
     *
     * 该模块主要提供角色的生命周期管理（创建、销毁、登录、登出）、属性管理、
     * 行动力与经验值的增减、与数据库和客户端的数据交互等功能。
     */
    class RoleModule : public ModuleBase {
    public:
        /**
         * @brief 构造函数
         * @param owner 指向所属玩家对象的指针
         */
        explicit RoleModule(PlayerObjPtr owner);

        /**
         * @brief 析构函数
         */
        ~RoleModule() override = default;

    public:
        /**
         * @brief 角色创建时调用
         * @param role_id 角色ID
         * @return true 成功，false 失败
         */
        bool on_create(std::uint64_t role_id) override;

        /**
         * @brief 角色销毁时调用
         * @return true 成功，false 失败
         */
        bool on_destroy() override;

        /**
         * @brief 玩家登录时调用
         * @return true 成功，false 失败
         */
        bool on_login() override;

        /**
         * @brief 玩家登出时调用
         * @return true 成功，false 失败
         */
        bool on_logout() override;

        /**
         * @brief 新的一天（跨天）时调用
         * @return true 成功，false 失败
         */
        bool on_new_day() override;

        /**
         * @brief 初始化基础数据
         * @param role_id 角色ID
         * @param name 角色名称
         * @param career_id 职业ID
         * @param account_id 账号ID
         * @param channel 渠道号
         * @return true 成功，false 失败
         */
        bool init_base_data(std::uint64_t role_id,
                            std::string_view name,
                            std::uint32_t career_id,
                            std::uint64_t account_id,
                            std::int32_t channel);

        /**
         * @brief 从数据库登录数据读取
         * @param ack 数据库返回的登录应答
         * @return true 成功，false 失败
         */
        bool read_from_db_login_data(const DBRoleLoginAck& ack);

        /**
         * @brief 保存数据到客户端登录数据
         * @param ack 客户端登录应答包
         * @return true 成功，false 失败
         */
        bool save_to_client_login_data(RoleLoginAck& ack) override;

        /**
         * @brief 通知属性或数据发生变化
         * @return true 成功，false 失败
         */
        bool notify_change() override;

        /**
         * @brief 计算战斗力
         * @param value 固定属性值
         * @param percent 百分比加成
         * @param fight_value 返回战斗力
         * @return true 成功，false 失败
         */
        bool calc_fight_value(const std::array<int32_t, PropertyNum>& value,
                              const std::array<int32_t, PropertyNum>& percent,
                              int32_t& fight_value);

        /**
         * @brief 注册消息处理函数
         */
        void register_message_handler();

        /**
         * @brief 获取某个属性值
         * @param property_id 属性ID
         * @return 属性值
         */
        std::uint32_t get_property(RoleProperty property_id) const;

    public:
        /**
         * @brief 扣除行动力
         * @param action_id 行动力ID
         * @param action_num 扣除数量
         * @return true 扣除成功，false 行动力不足
         */
        bool cost_action(std::uint32_t action_id, int32_t action_num);

        /**
         * @brief 检查行动力是否足够
         * @param action_id 行动力ID
         * @param action_num 所需数量
         * @return true 足够，false 不足
         */
        bool check_action_enough(std::uint32_t action_id, int32_t action_num);

        /**
         * @brief 获取行动力
         * @param action_id 行动力ID
         * @return 行动力值
         */
        std::uint64_t get_action(std::uint32_t action_id);

        /**
         * @brief 增加行动力
         * @param action_id 行动力ID
         * @param action_num 增加的数量
         * @return 增加后的行动力值
         */
        std::uint64_t add_action(std::uint32_t action_id, std::int64_t action_num);

        /**
         * @brief 更新行动力
         * @param action_id 行动力ID
         * @return true 成功，false 失败
         */
        bool update_action(std::uint32_t action_id);

        /**
         * @brief 设置角色是否删除
         * @param is_delete 是否删除
         * @return true 成功，false 失败
         */
        bool set_delete(bool is_delete);

        /**
         * @brief 增加经验值
         * @param exp 经验值
         * @return 增加后的总经验值
         */
        std::uint64_t add_exp(int32_t exp);

        /**
         * @brief 获取上次下线时间
         * @return 时间戳
         */
        std::uint64_t get_last_logoff_time() const;

        /**
         * @brief 设置上次下线时间
         * @param time 时间戳
         * @return true 成功，false 失败
         */
        bool set_last_logoff_time(std::uint64_t time);

        /**
         * @brief 获取上次登录时间
         * @return 时间戳
         */
        std::uint64_t get_last_logon_time() const;

        /**
         * @brief 获取角色创建时间
         * @return 时间戳
         */
        std::uint64_t get_create_time() const;

        /**
         * @brief 获取累计在线时长
         * @return 在线时长（秒）
         */
        std::uint32_t get_online_time() const;

        /**
         * @brief 设置群发邮件时间
         * @param time 时间戳
         */
        void set_group_mail_time(std::uint64_t time);

        /**
         * @brief 获取群发邮件时间
         * @return 时间戳
         */
        std::uint64_t get_group_mail_time() const;

    public:
        /**
         * @brief 获取角色ActorID
         * @return ActorID
         */
        std::uint32_t get_actor_id() const;

        /**
         * @brief 获取角色等级
         * @return 等级
         */
        uint32_t get_level() const;

        /**
         * @brief 获取VIP等级
         * @return VIP等级
         */
        int32_t get_vip_level() const;

        /**
         * @brief 获取角色名称
         * @return 名称字符串
         */
        std::string get_name() const;

        /**
         * @brief 获取职业ID
         * @return 职业ID
         */
        std::uint32_t get_career_id() const;

        /**
         * @brief 获取角色ID
         * @return 角色ID
         */
        std::uint64_t get_role_id() const;

    private:
        /**
         * @brief Actor ID
         */
        std::uint32_t actor_id_{};

        /**
         * @brief 共享内存中的角色数据对象
         */
        std::shared_ptr<shm::RoleDataObject> role_data_object_;
    };

}
