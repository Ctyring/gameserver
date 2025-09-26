#pragma once

#include <cstdint>
#include <set>
#include "cfl.h"
#include "cfl/protos/gen_proto/login.pb.h"

namespace cfl {
    class CPlayerObject;

    /**
     * @class ModuleBase
     * @brief 所有功能模块的基类，负责与玩家对象绑定并提供模块生命周期接口。
     *
     * 该类定义了游戏逻辑中所有模块需要实现的接口，包括：
     * - 创建与销毁（on_create / on_destroy）
     * - 登录与登出（on_login / on_logout）
     * - 跨天逻辑（on_new_day）
     * - 数据同步与存储（notify_change / read_from_db_login_data / save_to_client_login_data）
     * - 战斗力与属性计算接口（calc_fight_value / get_property）
     *
     * 模块通过维护 change_set 与 remove_set 来跟踪数据变化，便于后续的存储与同步。
     */
    class ModuleBase {
    public:
        /**
         * @brief 构造函数
         * @param owner 指向所属的玩家对象
         */
        explicit ModuleBase(CPlayerObject *owner);

        /**
         * @brief 析构函数
         */
        virtual ~ModuleBase() = default;

        /**
         * @brief 模块创建时调用
         * @param role_id 玩家角色ID
         * @return true 表示成功，false 表示失败
         */
        virtual bool on_create(std::uint64_t role_id) = 0;

        /**
         * @brief 模块销毁时调用
         * @return true 表示成功，false 表示失败
         */
        virtual bool on_destroy() = 0;

        /**
         * @brief 玩家登录时调用
         * @return true 表示成功，false 表示失败
         */
        virtual bool on_login() = 0;

        /**
         * @brief 玩家登出时调用
         * @return true 表示成功，false 表示失败
         */
        virtual bool on_logout() = 0;

        /**
         * @brief 跨天（每日零点）时调用
         * @return true 表示成功，false 表示失败
         */
        virtual bool on_new_day() = 0;

        /**
         * @brief 通知客户端模块内数据发生变化
         * @return true 表示成功，false 表示失败
         */
        virtual bool notify_change() = 0;

        /**
         * @brief 从数据库登录数据中读取模块数据
         * @param ack 登录数据结构（protobuf）
         * @return true 表示成功，false 表示失败
         */
        virtual bool read_from_db_login_data(RoleLoginAck &ack) = 0;

        /**
         * @brief 保存模块数据到客户端登录数据
         * @param ack 登录数据结构（protobuf）
         * @return true 表示成功，false 表示失败
         */
        virtual bool save_to_client_login_data(RoleLoginAck &ack) = 0;

        /**
         * @brief 计算战斗力
         * @param value 属性值数组
         * @param percent 属性百分比加成数组
         * @param fight_value 输出的最终战斗力
         * @return true 表示计算成功，false 表示失败
         */
        virtual bool calc_fight_value(
                int32_t value[PropertyNum],
                int32_t percent[PropertyNum],
                int32_t &fight_value);

        /**
         * @brief 获取指定属性值
         * @param property_id 属性ID
         * @return 属性值，未定义时返回0
         */
        virtual std::int64_t get_property(RoleProperty property_id);

    public:
        /**
         * @brief 添加变更ID，表示该数据需要同步或保存
         * @param id 数据ID
         * @return true 表示成功
         */
        bool add_change_id(std::uint64_t id);

        /**
         * @brief 添加移除ID，表示该数据需要删除
         * @param id 数据ID
         * @return true 表示成功
         */
        bool add_remove_id(std::uint64_t id);

        /**
         * @brief 设置所属的玩家对象
         * @param owner 玩家对象指针
         * @return true 表示成功
         */
        bool set_owner(CPlayerObject *owner);

        /**
         * @brief 获取所属的玩家对象
         * @return 玩家对象指针
         */
        CPlayerObject *get_owner();

    protected:
        CPlayerObject *owner_player{nullptr};   ///< 所属玩家对象指针
        std::set<std::uint64_t> change_set;     ///< 需要同步/保存的数据ID集合
        std::set<std::uint64_t> remove_set;     ///< 需要删除的数据ID集合
    };

} // namespace cfl
