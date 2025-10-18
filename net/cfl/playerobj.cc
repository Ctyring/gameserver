#include "playerobj.h"
#include "cfl/modules/role_module.h"
#include "cfl/modules/mail_module.h"

namespace cfl {
    bool PlayerObject::init(std::uint64_t role_id) {
        role_id_ = role_id;
        proxy_conn_id_ = 0;
        client_conn_id_ = 0;
        copy_guid_ = 0;      //当前的副本ID
        copy_id_ = 0;        //当前的副本类型
        copy_server_id_ = 0;        //副本服务器的ID
        is_online_ = false;
        room_id_ = 0;

//        create_all_modules();
        return true;
    }

    bool PlayerObject::uninit() {
        destroy_all_modules();
        role_id_ = 0;
        proxy_conn_id_ = 0;
        client_conn_id_ = 0;
        copy_guid_ = 0;      //当前的副本ID
        copy_id_ = 0;        //当前的副本类型
        copy_server_id_ = 0;        //副本服务器的ID
        is_online_ = false;
        room_id_ = 0;

        return true;
    }

    bool PlayerObject::on_create(std::uint64_t role_id) {
        for (auto type = static_cast<int>(ModuleType::Role);
             type < static_cast<int>(ModuleType::End); type++) {
            auto module = modules_.at(type);
            if (module == nullptr || !module->on_create(role_id)) {
                return false;
            }
        }
        return true;
    }

    bool PlayerObject::on_destroy() {
        destroy_all_modules();
        return true;
    }

    bool PlayerObject::on_login() {
        for (auto type = static_cast<int>(ModuleType::Role);
             type < static_cast<int>(ModuleType::End); type++) {
            auto module = modules_.at(type);
            if (module == nullptr || !module->on_login()) {
                return false;
            }
        }
        is_online_ = true;
        // todo: 处理跨天逻辑
        return true;
    }

    bool PlayerObject::on_logout() {
        for (auto type = static_cast<int>(ModuleType::Role);
             type < static_cast<int>(ModuleType::End); type++) {
            auto module = modules_.at(type);
            if (module == nullptr || !module->on_logout()) {
                return false;
            }
        }
        is_online_ = false;

        room_id_ = 0;
        return true;
    }

    bool PlayerObject::on_new_day() {
        for (auto type = static_cast<int>(ModuleType::Role);
             type < static_cast<int>(ModuleType::End); type++) {
            auto module = modules_.at(type);
            if (module == nullptr || !module->on_new_day()) {
                return false;
            }
        }
        return true;
    }

    bool PlayerObject::read_from_db_login_data(DBRoleLoginAck &ack) {
        for (auto type = static_cast<int>(ModuleType::Role);
             type < static_cast<int>(ModuleType::End); type++) {
            auto module = modules_.at(type);
            if (module == nullptr || !module->read_from_db_login_data(ack)) {
                return false;
            }
        }
        return true;
    }

    bool PlayerObject::create_all_modules() {
        modules_.resize(static_cast<std::size_t>(ModuleType::End));
        modules_[static_cast<std::size_t>(ModuleType::Role)] = std::make_shared<RoleModule>(this);
        modules_[static_cast<std::size_t>(ModuleType::Mail)] = std::make_shared<MailModule>(this);
        return true;
    }

    bool PlayerObject::destroy_all_modules() {
        for (auto type = static_cast<int>(ModuleType::Role);
             type < static_cast<int>(ModuleType::End); type++) {
            auto module = modules_.at(type);
            if (module == nullptr || !module->on_destroy()) {
                return false;
            }
        }
        modules_.clear();
        return true;
    }
}