#include "mail_module.h"
#include "cfl/mail/mail_manager.h"
#include "cfl/shm/shmpool.h"
#include "cfl/global_data_manager.h"
#include "role_module.h"
#include "cfl/protos/gen_proto/msg.pb.h"

namespace cfl {
    bool MailModule::read_from_db_login_data(const DBRoleLoginAck &ack) {
        auto &mail_data = ack.mails();
        for (auto &mail: mail_data.items()) {
            auto obj = MailManager::instance().pick_up_mail_data(mail.guid());
            if (!obj) {
                obj = create_object<MailDataObject>(SHMTYPE::Mail, true);
                obj->role_id = mail.role_id();
                obj->guid = mail.guid();
                obj->group_guid = mail.group_id();
                obj->time = mail.time();
                obj->sender_id = mail.sender_id();
                obj->mail_type = mail.mail_type();
                obj->status = mail.status();
                obj->sender = str_copy(mail.sender(), ROLE_NAME_LEN);
                obj->title = str_copy(mail.title(), MAIL_TITLE_LEN);
                obj->content = str_copy(mail.content(), MAIL_CONTENT_LEN);
                std::string blob = mail.items();
                memcpy(obj->items.data(), blob.data(),
                       std::min(blob.size(), sizeof(obj->items)));
                mail_data_map_[obj->guid] = obj;
            } else {
                mail_data_map_[obj->guid] = obj;
            }
        }

        return true;
    }

    bool MailModule::add_mail(std::shared_ptr<MailDataObject> mail) {
        mail_data_map_.emplace(mail->guid, mail);
        return true;
    }

    bool MailModule::delete_mail(uint64_t guid) {
        auto it = mail_data_map_.find(guid);
        if (it == mail_data_map_.end()) {
            return false;
        }

        auto obj = it->second;
        obj->destroy();
        mail_data_map_.erase(it);

        add_remove_id(guid);
        return true;
    }

    bool MailModule::delete_mail_by_group_id(uint64_t group_id) {
        for (auto it = mail_data_map_.begin(); it != mail_data_map_.end();) {
            auto obj = it->second;
            if (obj->group_guid == group_id) {
                obj->destroy();
                it = mail_data_map_.erase(it);
                add_remove_id(obj->guid);
            } else {
                ++it;
            }
        }
        return true;
    }

    bool MailModule::add_mail(MailType mail_type, const std::string &sender, const std::string &title,
                              const std::string &content, const std::vector<StMailItem> &items) {
        auto obj = create_object<MailDataObject>(SHMTYPE::Mail, true);
        obj->lock();
        obj->guid = GlobalDataManager::instance().make_new_guid();
        auto pl = owner_player;
        obj->role_id = pl->role_id();
        obj->sender = str_copy(sender, ROLE_NAME_LEN);
        obj->title = str_copy(title, MAIL_TITLE_LEN);
        obj->content = str_copy(content, MAIL_CONTENT_LEN);
        obj->mail_type = mail_type;
        obj->time = get_timestamp();
        for(int i = 0; i < items.size(); ++i){
            if(items[i].item_id == 0){
                break;
            }
            obj->items[i] = items[i];
        }

        obj->unlock();
        return add_mail(obj);
    }

    std::shared_ptr<MailDataObject> MailModule::get_mail_by_guid(uint64_t guid) {
        auto it = mail_data_map_.find(guid);
        if (it == mail_data_map_.end()) {
            return nullptr;
        }
        return it->second;
    }

    bool MailModule::receive_group_mail(std::shared_ptr<GroupMailDataObject> group_mail) {
        auto obj = create_object<MailDataObject>(SHMTYPE::Mail, true);
        obj->lock();
        obj->guid = GlobalDataManager::instance().make_new_guid();
        auto pl = owner_player;
        obj->role_id = pl->role_id();
        obj->time = get_timestamp();
        obj->group_guid = group_mail->guid;
        obj->mail_type = group_mail->mail_type;
        obj->title = str_copy(group_mail->title, MAIL_TITLE_LEN);
        obj->content = str_copy(group_mail->content, MAIL_CONTENT_LEN);
        obj->sender = str_copy(group_mail->sender, ROLE_NAME_LEN);
        for(int i = 0; i < group_mail->items.size(); ++i){
            if(group_mail->items[i].item_id == 0){
                break;
            }
            obj->items[i] = group_mail->items[i];
        }
        obj->unlock();

        add_mail(obj);
        auto role_module = std::dynamic_pointer_cast<RoleModule>(pl->get_module_by_type(ModuleType::Role));
        role_module->set_group_mail_time(group_mail->time);

        spdlog::info("receive group mail:{}", group_mail->guid);
        return true;
    }

    bool MailModule::notify_change() {
        if(change_set.empty() && remove_set.empty()){
            return true;
        }
        MailChangeNty nty;
        for(auto &guid: change_set){
            auto item = nty.add_change_list();
            auto obj = get_mail_by_guid(guid);
            item->set_guid(obj->guid);
            item->set_mail_type(obj->mail_type);
            item->set_status(obj->status);
            item->set_title(obj->title);
            item->set_content(obj->content);
            item->set_sender(obj->sender);

            for(int i = 0; i < MAIL_ITEM_COUNT; i++){
                if(obj->items[i].item_id == 0){
                    break;
                }

                item->add_item_id(obj->items[i].item_id);
                item->add_item_num(obj->items[i].item_count);
            }
        }
        for(auto &guid: remove_set){
            nty.add_remove_list(guid);
        }
        auto pl = owner_player;
        pl->send_msg_protobuf(MSG_MAIL_CHANGE_NTY, nty);
        change_set.clear();
        remove_set.clear();
        return true;
    }
}