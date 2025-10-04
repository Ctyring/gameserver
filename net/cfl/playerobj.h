#pragma once
/**
 * @file player_object.h
 * @brief 定义 PlayerObject 类，管理玩家的基本信息、状态、场景交互和模块。
 */

#include "handler_manager.h"
#include "cfl.h"
#include "cfl/protos/gen_proto/login.pb.h"
#include "cfl/protos/gen_proto/game.pb.h"
#include "cfl/protos/gen_proto/login_db.pb.h"
#include "cfl/modules/module_base.h"
namespace cfl {
    /**
     * @class PlayerObject
     * @brief 玩家对象类，负责维护玩家的基本属性、登录状态、场景操作和模块管理。
     *
     * 该类继承自 HandlerManager，支持消息处理功能。
     * 功能包括：
     * - 玩家生命周期管理（创建、销毁、登录、登出）
     * - 数据同步（数据库、场景服、客户端）
     * - 模块管理（战斗、背包、任务等模块）
     * - 网络连接维护
     */
    class PlayerObject : public HandlerManager {
    public:
        /**
         * @brief 默认构造函数。
         */
        PlayerObject() = default;

        /**
         * @brief 析构函数。
         */
        ~PlayerObject() = default;

        /**
         * @brief 初始化玩家对象。
         * @param role_id 玩家角色ID。
         * @return 初始化是否成功。
         */
        bool init(std::uint64_t role_id);

        /**
         * @brief 反初始化玩家对象。
         * @return 是否成功。
         */
        bool uninit();

        /**
         * @brief 玩家角色创建时调用。
         * @param role_id 玩家角色ID。
         * @return 是否成功。
         */
        bool on_create(std::uint64_t role_id);

        /**
         * @brief 玩家角色销毁时调用。
         * @return 是否成功。
         */
        bool on_destroy();

        /**
         * @brief 玩家登录处理。
         * @return 是否成功。
         */
        bool on_login();

        /**
         * @brief 玩家登出处理。
         * @return 是否成功。
         */
        bool on_logout();

        /**
         * @brief 新的一天事件处理（每日重置逻辑）。
         * @return 是否成功。
         */
        bool on_new_day();

        /**
         * @brief 从数据库读取玩家登录数据。
         * @param ack 数据库返回的登录应答消息。
         * @return 是否成功。
         */
        bool read_from_db_login_data(DBRoleLoginAck &ack);

        /**
         * @brief 发送 Protobuf 消息给客户端。
         * @param msg_id 消息ID。
         * @param data Protobuf 消息对象。
         * @return 是否成功。
         */
        bool send_msg_protobuf(std::int32_t msg_id, const google::protobuf::Message &data);

        /**
         * @brief 发送原始数据给客户端。
         * @param msg_id 消息ID。
         * @param data 数据指针。
         * @param len 数据长度。
         * @return 是否成功。
         */
        bool send_msg_raw(std::int32_t msg_id, const char *data, std::uint32_t len);

        /**
         * @brief 向玩家所在的场景服发送消息。
         * @param msg_id 消息ID。
         * @param data Protobuf 消息对象。
         * @return 是否成功。
         */
        bool send_msg_to_scene(std::int32_t msg_id, const google::protobuf::Message &data);

        /**
         * @brief 转换为可传输数据结构。
         * @param transfer_item 传输数据对象。
         * @return 是否成功。
         */
        bool to_transfer_data(TransferDataItem *transfer_item);

        /**
         * @brief 通知任务系统某个事件发生。
         * @param event_id 事件ID。
         * @param param1 附加参数1。
         * @param param2 附加参数2。
         * @return 是否成功。
         */
        bool notify_task_event(std::uint32_t event_id, std::uint32_t param1, std::uint32_t param2);

        /**
         * @brief 判断玩家是否在线。
         * @return 在线状态。
         */
        bool is_online() const;

        /**
         * @brief 设置玩家在线状态。
         * @param online 是否在线。
         */
        void set_online(bool online);

        /**
         * @brief 通知客户端玩家属性发生变化。
         * @return 是否成功。
         */
        bool notify_change();

        /**
         * @brief 发送进入场景通知。
         * @param copy_guid 副本唯一ID。
         * @param copy_id 副本ID。
         * @param server_id 场景服务器ID。
         * @return 是否成功。
         */
        bool send_into_scene_notify(std::uint32_t copy_guid, std::uint32_t copy_id, std::uint32_t server_id);

        /**
         * @brief 发送离开场景通知。
         * @param copy_guid 副本唯一ID。
         * @param server_id 场景服务器ID。
         * @return 是否成功。
         */
        bool send_leave_scene(std::uint32_t copy_guid, std::uint32_t server_id);

        /**
         * @brief 向客户端发送角色登录应答。
         * @return 是否成功。
         */
        bool send_role_login_ack();

        /**
         * @brief 通知客户端玩家属性变化。
         * @param change_type 变化类型。
         * @param value1 数值1。
         * @param value2 数值2。
         * @param str_value 字符串数值。
         * @return 是否成功。
         */
        bool send_player_change(ChangeType change_type, std::uint64_t value1, std::uint64_t value2,
                                std::string_view str_value);

        /**
         * @brief 设置玩家连接信息。
         * @param proxy_id 代理服务器连接ID。
         * @param client_id 客户端连接ID。
         * @return 是否成功。
         */
        bool set_connect_id(std::uint32_t proxy_id, std::uint32_t client_id);

        /**
         * @brief 清理副本状态。
         * @return 是否成功。
         */
        bool clear_copy_status();

        /**
         * @brief 设置副本状态。
         * @param copy_guid 副本唯一ID。
         * @param copy_id 副本ID。
         * @param copy_server_id 场景服务器ID。
         * @param main_city 是否为主城。
         * @return 是否成功。
         */
        bool set_copy_status(std::uint32_t copy_guid, std::uint32_t copy_id, std::uint32_t copy_server_id, bool main_city);

        /**
         * @brief 创建所有模块。
         * @return 是否成功。
         */
        bool create_all_modules();

        /**
         * @brief 销毁所有模块。
         * @return 是否成功。
         */
        bool destroy_all_modules();

        /**
         * @brief 根据类型获取模块指针。
         * @param module_type 模块类型ID。
         * @return 模块基类指针。
         */
        ModuleBase *get_module_by_type(std::uint32_t module_type);

        /**
         * @brief 检查进入副本的条件。
         * @param copy_id 副本ID。
         * @return 条件检查结果。
         */
        std::uint32_t check_copy_condition(std::uint32_t copy_id);

        // ---------------- 基础属性接口 ----------------

        /// @brief 获取角色ID。
        [[nodiscard]] std::uint64_t role_id() const noexcept { return role_id_; }

        /// @brief 获取账号ID。
        [[nodiscard]] std::uint64_t account_id() const noexcept { return account_id_; }

        /// @brief 获取所在主城副本ID。
        [[nodiscard]] std::uint32_t city_copy_id() const noexcept { return city_copy_id_; }

        /// @brief 获取角色在场景中的actor ID。
        [[nodiscard]] std::uint32_t actor_id() const noexcept { return actor_id_; }

        /// @brief 获取角色名称。
        [[nodiscard]] const std::string &name() const noexcept { return name_; }

        /// @brief 获取职业ID。
        [[nodiscard]] std::uint32_t career_id() const noexcept { return career_id_; }

        /**
         * @brief 获取某个角色属性值。
         * @param property_id 属性ID。
         * @return 属性值。
         */
        [[nodiscard]] std::int64_t get_property(RoleProperty property_id) const;

        // ---------------- 房间管理 ----------------

        /// @brief 获取玩家房间ID。
        [[nodiscard]] std::uint64_t room_id() const noexcept { return room_id_; }

        /// @brief 设置玩家房间ID。
        void set_room_id(std::uint64_t room_id) noexcept { room_id_ = room_id; }

        // ---------------- 战斗属性 ----------------

        /**
         * @brief 计算战斗属性信息。
         * @return 是否成功。
         */
        bool calc_fight_data_info();

    private:
        // 基本属性
        std::uint64_t role_id_{0};        ///< 角色ID
        std::uint64_t account_id_{0};     ///< 账号ID
        std::uint32_t city_copy_id_{0};   ///< 所在主城副本ID
        std::uint32_t actor_id_{0};       ///< 场景中的actor ID
        std::string name_;                ///< 角色名称
        std::uint32_t career_id_{0};      ///< 职业ID

        // 房间/战斗属性
        std::uint64_t room_id_{0};                                    ///< 房间ID
        std::array<std::int32_t, PropertyNum> properties_{};          ///< 属性数组（战斗属性）

        // 网络连接
        std::int32_t proxy_conn_id_{-1};   ///< 代理服连接ID
        std::int32_t client_conn_id_{-1};  ///< 客户端连接ID
        bool is_online_{false};            ///< 是否在线

        // 副本状态
        std::uint32_t copy_guid_{0};       ///< 副本唯一ID
        std::uint32_t copy_id_{0};         ///< 副本ID
        std::uint32_t copy_server_id_{0};  ///< 场景服务器ID
        bool is_main_city_{true};          ///< 是否为主城状态

        // 模块容器
        std::vector<ModuleBase *> modules_; ///< 玩家模块集合
    };
}
