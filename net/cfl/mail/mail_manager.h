#pragma once
// 防止头文件被重复包含（比传统的 #ifndef/#define 更简洁高效）

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include "cfl/protos/gen_proto/define.pb.h"   // 导入协议定义（通常包含 MailType、StMailItem 等结构）
#include "cfl/server_define.h"                 // 一些服务器全局定义（如枚举、宏、常量等）
#include "cfl/shm/obj/mail_data_obj.h"         // 包含邮件对象的共享内存结构定义
#include "cfl/db/db_mysql.h"                   // MySQL 数据库访问封装
#include "cfl/playerobj.h"                     // 玩家对象类定义

namespace cfl::shm {

//==============================================================
// @class MailManager
// @brief 管理游戏内的邮件系统，包括群邮件、单人邮件、离线邮件处理、数据库加载等功能。
//==============================================================
    class MailManager {
    public:
        //======================
        // 单例访问接口
        //======================
        static MailManager &instance() {
            // 使用局部静态变量实现线程安全的单例（C++11 保证线程安全初始化）
            static MailManager instance_;
            return instance_;
        }

        // 禁止拷贝构造和拷贝赋值，防止单例被复制
        MailManager(const MailManager &) = delete;
        MailManager &operator=(const MailManager &) = delete;

        //==========================================================
        // @brief 群发邮件（例如系统公告、活动奖励邮件）
        // @param sender   发件人名称（可为系统或GM）
        // @param title    邮件标题
        // @param content  邮件内容
        // @param items    附件物品列表
        // @param recv_group 接收群体标识（如全服、某等级段、某阵营）
        // @return 是否发送成功
        //==========================================================
        [[nodiscard]]
        bool send_group_mail(std::string_view sender,
                             std::string_view title,
                             std::string_view content,
                             const std::vector<StMailItem> &items,
                             int32_t recv_group);

        //==========================================================
        // @brief 发送单人邮件
        // @param role_id  接收玩家角色ID
        // @param mail_type 邮件类型（系统/私人/活动）
        // @param content  邮件正文
        // @param items    邮件附件
        // @param sender   发件人名称（默认可为空）
        // @param title    邮件标题（默认可为空）
        // @return 是否发送成功
        //==========================================================
        [[nodiscard]]
        bool send_single_mail(uint64_t role_id,
                              MailType mail_type,
                              std::string_view content,
                              const std::vector<StMailItem> &items,
                              std::string_view sender = "",
                              std::string_view title = "");

        //==========================================================
        // @brief 离线操作（例如在玩家离线时缓存邮件，等登录后再下发）
        // @param role_id 玩家角色ID
        // @return 是否成功缓存离线操作
        //==========================================================
        [[nodiscard]]
        bool send_off_operation(uint64_t role_id);

        //==========================================================
        // @brief 删除群邮件
        // @param guid 群邮件唯一标识（通常对应数据库主键）
        // @return 是否删除成功
        //==========================================================
        [[nodiscard]]
        bool delete_group_mail(uint64_t guid);

        //==========================================================
        // @brief 从数据库加载所有邮件相关数据
        //        一般在服务器启动时调用，加载历史邮件记录
        // @return 是否加载成功
        //==========================================================
        [[nodiscard]]
        bool load_data();

        //==========================================================
        // @brief 加载群邮件数据（从数据库 mail_group 表）
        //        将数据库中的群邮件加载到共享内存对象中
        // @return 是否加载成功
        //==========================================================
        [[nodiscard]]
        bool load_group_mail_data(){
            // 执行数据库查询，返回 mail_group 表的所有记录
            auto res = db::MySQLUtil::query("", "SELECT * FROM mail_group");

            // 遍历结果集
            while (res->next()){
                // 创建共享内存对象 GroupMailDataObject
                auto group_mail = create_object<GroupMailDataObject>(SHMTYPE::GroupMail, false);

                // 邮件类型固定为自定义类型
                group_mail->mail_type = mail_custom;

                // 从结果集中取出频道、GUID、时间戳等字段
                group_mail->channel = res->get_int32("channel");
                group_mail->guid = res->get_int64("id");
                group_mail->time = res->get_int64("mail_time");

                // 将字符串字段拷贝到固定长度的缓冲区中
                group_mail->title = str_copy(res->get_string("title"), MAIL_TITLE_LEN);
                group_mail->content = str_copy(res->get_string("content"), MAIL_CONTENT_LEN);
                group_mail->sender = str_copy(res->get_string("sender"), ROLE_NAME_LEN);

                // 读取二进制附件数据（itemdata 字段存储的是 StMailItem 数组）
                std::vector<std::byte> blob = res->get_blob("itemdata");

                // 根据数据大小计算附件数量，并复制到共享内存对象的 items 数组中
                auto count = std::min(blob.size() / sizeof(StMailItem), group_mail->items.size());
                std::memcpy(group_mail->items.data(), blob.data(), count * sizeof(StMailItem));

                // 将该群邮件对象放入管理容器，以 guid 为 key
                group_mail_data_.emplace(group_mail->guid, group_mail);
            }
            return true;
        }

        //==========================================================
        // @brief 玩家登录时处理邮件逻辑
        //        例如加载未读邮件、群发邮件合并、清理过期邮件等
        // @param player 玩家对象指针
        // @return 是否处理成功
        //==========================================================
        bool process_role_login(std::shared_ptr<PlayerObject> &player);

        //==========================================================
        // @brief 拾取单封邮件数据（通常在客户端请求领取附件时调用）
        // @param guid 邮件唯一ID
        // @return 返回该邮件的共享指针对象
        //==========================================================
        std::shared_ptr<MailDataObject> pick_up_mail_data(uint64_t guid);

        //==========================================================
        // @brief 离线邮件缓存（key: 角色ID，value: 邮件对象）
        //        玩家不在线时发送的邮件会暂存在这里
        //==========================================================
        std::unordered_map<uint64_t, std::shared_ptr<MailDataObject>> off_mail_data_;

        //==========================================================
        // @brief 群邮件缓存（key: 邮件GUID，value: 群邮件对象）
        //        所有群发邮件加载到此容器中，供在线玩家登录时下发
        //==========================================================
        std::unordered_map<uint64_t, std::shared_ptr<GroupMailDataObject>> group_mail_data_;

    private:
        // 构造函数私有化，保证单例模式
        MailManager() = default;

        // 析构函数私有化，防止外部析构单例
        ~MailManager() = default;
    };

} // namespace cfl::shm
