#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <string_view>
#include "cfl/shm/obj/mail_data_obj.h"
#include "cfl/shm/shmpool.h"
#include "cfl/protos/gen_proto/define.pb.h"
#include "mail_manager.h"
#include "cfl/global_data_manager.h"
#include "cfl/tools/common.h"
#include "cfl/modules/role_module.h"
#include "cfl/modules/mail_module.h"

namespace cfl {
    using namespace cfl::shm;

    bool MailManager::send_group_mail(std::string_view sender,
                                      std::string_view title,
                                      std::string_view content,
                                      const std::vector<StMailItem> &items,
                                      int32_t recv_group) {
        if (recv_group == 2) {
            // todo: 使用PlayerManager群发
            //            auto node = CPlayerManager::GetInstancePtr()->MoveFirst();
//            if (!node) return false;
//
//            for (; node; node = CPlayerManager::GetInstancePtr()->MoveNext(node)) {
//                if (auto player = node->GetValue(); player && player->IsOnline()) {
//                    if (auto mailModule = dynamic_cast<CMailModule *>(player->GetModuleByType(MT_MAIL))) {
//                        mailModule->AddMail(EMT_CUSTOM, std::string(sender), std::string(title),
//                                            std::string(content), items);
//                    }
//                }
//            }
            return true;
        }

        auto group_mail = shm::create_object<GroupMailDataObject>(SHMTYPE::GroupMail, true);
        group_mail->lock();
        group_mail->guid = GlobalDataManager::instance().make_new_guid();
        group_mail->mail_type = mail_custom;
        group_mail->time = get_timestamp();

        group_mail->title = str_copy(title.data(), MAIL_TITLE_LEN);
        group_mail->content = str_copy(content.data(), MAIL_CONTENT_LEN);
        group_mail->sender = str_copy(sender.data(), ROLE_NAME_LEN);

        for (size_t i = 0; i < items.size() && i < MAIL_ITEM_COUNT; ++i) {
            if (items[i].item_id == 0) break;
            group_mail->items[i] = items[i];
        }

        group_mail->unlock();
        group_mail_data_.emplace(group_mail->guid, group_mail);

        // todo: 群发邮件
//        auto node = CPlayerManager::GetInstancePtr()->MoveFirst();
//        if (!node) return false;
//
//        for (; node; node = CPlayerManager::GetInstancePtr()->MoveNext(node)) {
//            if (auto player = node->GetValue(); player && player->IsOnline()) {
//                if (auto mailModule = dynamic_cast<CMailModule *>(player->GetModuleByType(MT_MAIL))) {
//                    mailModule->ReceiveGroupMail(groupMail);
//                }
//            }
//        }

        return true;
    }

    bool MailManager::send_single_mail(uint64_t roleId, MailType mailType, std::string_view content,
                                       const std::vector<StMailItem> &items, std::string_view sender,
                                       std::string_view title) {
        if (mailType <= 0) return false;
        // todo: 使用playermanager遍历添加邮件
//        if (auto player = CPlayerManager::GetInstancePtr()->GetPlayer(roleId); player) {
//            if (auto mailModule = dynamic_cast<CMailModule *>(player->GetModuleByType(MT_MAIL))) {
//                return mailModule->AddMail(mailType, std::string(sender), std::string(title),
//                                           std::string(content), items);
//            }
//            return false;
//        }

        auto mailObject = shm::create_object<MailDataObject>(SHMTYPE::Mail, true);
        mailObject->lock();
        mailObject->guid = GlobalDataManager::instance().make_new_guid();
        mailObject->role_id = roleId;
        mailObject->mail_type = mailType;
        mailObject->time = get_timestamp();
        mailObject->title = str_copy(title.data(), MAIL_TITLE_LEN);
        mailObject->content = str_copy(content.data(), MAIL_CONTENT_LEN);
        mailObject->sender = str_copy(sender.data(), ROLE_NAME_LEN);

        for (size_t i = 0; i < items.size() && i < MAIL_ITEM_COUNT; ++i) {
            if (items[i].item_id == 0) break;
            mailObject->items[i] = items[i];
        }
        mailObject->unlock();

        off_mail_data_.emplace(mailObject->guid, mailObject);
        return true;
    }

    std::shared_ptr<MailDataObject> MailManager::pick_up_mail_data(uint64_t guid) {
        if (auto it = off_mail_data_.find(guid); it != off_mail_data_.end()) {
            auto mail = it->second;
            off_mail_data_.erase(it);
            return mail;
        }
        return nullptr;
    }

    bool MailManager::send_off_operation(uint64_t role_id) {
        // todo
        spdlog::error("[MailManager::send_off_operation] todo");
        return true;
    }

    bool MailManager::delete_group_mail(uint64_t guid) {
        auto it = group_mail_data_.find(guid);
        if (it == group_mail_data_.end()) {
            spdlog::error("[MailManager::delete_group_mail] guid not found");
            return false;
        }

        auto mail = it->second;
        mail->destroy();

        group_mail_data_.erase(it);

        // todo: playermanager的一些工作
        return true;
    }

    bool MailManager::load_data() {
        if (!load_group_mail_data()) {
            return false;
        }
        return true;
    }

    bool MailManager::process_role_login(std::shared_ptr<PlayerObject> &player) {
        auto role_module = dynamic_pointer_cast<RoleModule>(player->get_module_by_type(ModuleType::Role));
        for(auto& mail : group_mail_data_){
            spdlog::info("[MailManager::process_role_login] group mail id", mail.first);
            auto mail_object = mail.second;
            if(mail_object->time > role_module->get_last_logon_time()){
                spdlog::error("[MailManager::process_role_login] todo: send group mail");
                auto mail_module = dynamic_pointer_cast<MailModule>(player->get_module_by_type(ModuleType::Mail));
                mail_module->receive_group_mail(mail_object);
            }
        }

        return true;
    }
}