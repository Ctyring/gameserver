#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "module_base.h"
#include "cfl/protos/gen_proto/define.pb.h"
#include "cfl/shm/obj/mail_data_obj.h"

namespace cfl {
    using namespace cfl::shm;

    class MailModule : public ModuleBase {
    public:
        explicit MailModule(ModuleBase::PlayerObjPtr owner) : ModuleBase(std::move(owner)) {
            register_message_handler();
        }

        ~MailModule() override = default;

    public:
        bool on_create(uint64_t role_id) override { return true; }

        bool on_destroy() override {
            for (auto &mail: mail_data_map_) {
                mail.second->release();
            }
            mail_data_map_.clear();
            return true;
        }

        bool on_login() override {
            return true;
        }

        bool on_logout() override{
            return true;
        }

        bool on_new_day() override{
            return true;
        }

        bool read_from_db_login_data(const DBRoleLoginAck &ack) override;

        bool save_to_client_login_data(RoleLoginAck &ack) override{
            return true;
        }

        bool calc_fight_value(
                int32_t value[PropertyNum],
                int32_t percent[PropertyNum],
                int32_t &fight_value) override {
            return true;
        }

        void register_message_handler(){}

    public:
        bool add_mail(std::shared_ptr<MailDataObject> mail);

        bool delete_mail(uint64_t guid);

        bool delete_mail_by_group_id(uint64_t group_id);

        bool add_mail(
                MailType mail_type,
                const std::string &sender,
                const std::string &title,
                const std::string &content,
                const std::vector<StMailItem> &items);

        std::shared_ptr<MailDataObject> get_mail_by_guid(uint64_t guid);

        bool receive_group_mail(std::shared_ptr<GroupMailDataObject> group_mail);

        bool notify_change() override;

    private:
        std::unordered_map<uint64_t, std::shared_ptr<MailDataObject>> mail_data_map_;
    };
}