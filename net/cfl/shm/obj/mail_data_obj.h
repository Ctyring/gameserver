#pragma once

#include <array>
#include <string>
#include <cstdint>
#include <string_view>
#include "cfl/shm/shmobj.h"
#include "cfl/db/db_mysql.h"
#include "cfl/db/db.h"

namespace cfl::shm {

    // 邮件标题最大长度
    constexpr size_t MAIL_TITLE_LEN = 128;
    // 邮件内容最大长度
    constexpr size_t MAIL_CONTENT_LEN = 512;
    // 角色名字最大长度
    constexpr size_t ROLE_NAME_LEN = 64;
    // 每封邮件最多携带的道具数量
    constexpr size_t MAIL_ITEM_COUNT = 8;

    /**
     * @brief 邮件道具结构体
     * 每个邮件道具包含物品ID和数量
     */
    struct StMailItem {
        std::int32_t item_id{};    // 道具ID
        std::int32_t item_count{}; // 道具数量

        StMailItem(std::int32_t id = 0, std::int32_t count = 0)
                : item_id(id), item_count(count) {}
    };

    /**
     * @brief 群发邮件数据对象
     * 继承自共享对象，用于共享内存和数据库操作
     */
    struct GroupMailDataObject : public SharedObject {
        std::uint64_t guid{};               // 唯一ID
        std::string title;                  // 邮件标题
        std::string content;                // 邮件内容
        std::string sender;                 // 发送者
        std::uint64_t time{};               // 邮件发送时间
        std::int32_t mail_type{};           // 邮件类型
        std::int32_t channel{};             // 邮件频道
        std::int32_t language{-1};          // 语言类型，-1表示未指定
        std::array<StMailItem, MAIL_ITEM_COUNT> items{}; // 邮件附带的道具列表
        std::int32_t group_type{};          // 群发类型：1->当前玩家, 2->当前+未来玩家

        GroupMailDataObject() = default;
        ~GroupMailDataObject() = default;

        /**
         * @brief 创建或替换数据库中的群发邮件记录
         * @return 操作是否成功
         */
        bool create() {
            int ret = cfl::db::MySQLUtil::execute_prepared(
                    "db_game",
                    "REPLACE INTO mail_group "
                    "(id, title, content, sender, mail_time, mailtype, channel, language, grouptype, itemdata) "
                    "VALUES(?,?,?,?,?,?,?,?,?,?);",
                    guid,
                    std::string_view(title),
                    std::string_view(content),
                    std::string_view(sender),
                    time,
                    mail_type,
                    channel,
                    language,
                    group_type,
                    std::string_view(reinterpret_cast<const char *>(items.data()), sizeof(StMailItem) * items.size())
            );
            return ret >= 0;
        }

        /**
         * @brief 更新群发邮件数据
         * @return 操作是否成功
         * @note 这里直接调用 create() 实现替换
         */
        bool update() { return create(); }

        /**
         * @brief 删除数据库中的群发邮件
         * @return 操作是否成功
         */
        bool remove() {
            int ret = cfl::db::MySQLUtil::execute_prepared("db_game", "DELETE FROM mail_group WHERE id = ?;", guid);
            return ret >= 0;
        }
    };

    /**
     * @brief 单封邮件数据对象
     * 继承自共享对象，用于共享内存和数据库操作
     */
    struct MailDataObject : public SharedObject {
        std::uint64_t guid{};               // 邮件唯一ID
        std::uint64_t role_id{};            // 接收玩家ID
        std::uint64_t group_guid{};         // 对应群发邮件ID
        std::uint64_t time{};               // 邮件发送时间
        std::uint64_t sender_id{};          // 发送者ID
        std::int32_t mail_type{};           // 邮件类型
        std::int32_t status{};              // 邮件状态
        std::string sender;                 // 发送者名字
        std::string title;                  // 邮件标题
        std::string content;                // 邮件内容
        std::array<StMailItem, MAIL_ITEM_COUNT> items{}; // 邮件附带的道具列表

        MailDataObject() = default;
        ~MailDataObject() = default;

        /**
         * @brief 创建或替换数据库中的邮件记录
         * @return 操作是否成功
         */
        bool create() {
            int ret = cfl::db::MySQLUtil::execute_prepared(
                    "db_game",
                    "REPLACE INTO mail "
                    "(roleid, id, groupid, mailtype, mailstatus, senderid, sendername, title, content, mail_time, itemdata) "
                    "VALUES(?,?,?,?,?,?,?,?,?,?,?);",
                    role_id,
                    guid,
                    group_guid,
                    mail_type,
                    status,
                    sender_id,
                    std::string_view(sender),
                    std::string_view(title),
                    std::string_view(content),
                    time,
                    std::string_view(reinterpret_cast<const char *>(items.data()), sizeof(StMailItem) * items.size())
            );
            return ret >= 0;
        }

        /**
         * @brief 更新邮件数据
         * @return 操作是否成功
         * @note 直接调用 create() 替换
         */
        bool update() { return create(); }

        /**
         * @brief 删除数据库中的邮件
         * @return 操作是否成功
         */
        bool remove() const {
            int ret = cfl::db::MySQLUtil::execute_prepared("db_game", "DELETE FROM mail WHERE id = ?;", guid);
            return ret >= 0;
        }
    };

    /**
     * @brief 离线数据对象
     * 用于记录离线事件，例如离线奖励、消息等
     * todo: 待完善
     */
    struct OfflineDataObject : public SharedObject {
        std::uint32_t op_type{};            // 操作类型
        std::uint64_t role_id{};            // 玩家ID

        /**
         * @brief 操作参数
         * 可用64位或32位数组访问
         */
        union Params {
            std::array<std::uint64_t, 4> u64; // 64位参数
            std::array<std::uint32_t, 8> u32; // 32位参数

            Params() : u64{} {} // 初始化为0
        } params;

        OfflineDataObject() = default;
        ~OfflineDataObject() = default;

        /**
         * @brief 创建离线数据
         * @return 总是返回 true
         */
        bool create() const noexcept { return true; }

        /**
         * @brief 更新离线数据
         * @return 总是返回 true
         */
        bool update() const noexcept { return true; }

        /**
         * @brief 删除离线数据
         * @return 总是返回 true
         */
        bool remove() const noexcept { return true; }
    };

} // namespace cfl
