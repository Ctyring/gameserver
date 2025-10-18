#include <iostream>
#include <vector>
#include <memory>
#include "cfl/mail/mail_manager.h"
#include "cfl/playerobj.h"
#include "cfl/modules/mail_module.h"
#include "cfl/modules/role_module.h"
#include "cfl/global_data_manager.h"
#include "cfl/shm/obj/mail_data_obj.h"

using namespace cfl;
using namespace cfl::shm;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    Config::Init();
    std::cout << "=== MailManager Unit Test Start ===" << std::endl;

    // 初始化全局管理器
    auto &mail_mgr = MailManager::instance();

    // 准备测试道具
    std::vector<StMailItem> items(3);
    items[0].item_id = 1001;
    items[0].item_count = (10);
    items[1].item_id = (1002);
    items[1].item_count = (5);
    items[2].item_id = (0); // 结束标记

    // 3测试群发邮件
    std::cout << "[Test] send_group_mail()" << std::endl;
    bool ok = mail_mgr.send_group_mail("System", "Welcome Gift",
                                       "Welcome to our world!",
                                       items, 1);
    std::cout << "send_group_mail result: " << std::boolalpha << ok << std::endl;

    // 测试单发邮件
    std::cout << "[Test] send_single_mail()" << std::endl;
    uint64_t test_role_id = 123456;
    ok = mail_mgr.send_single_mail(test_role_id, mail_custom,
                                   "Here is your reward!",
                                   items, "Admin", "Reward Mail");
    std::cout << "send_single_mail result: " << ok << std::endl;

    // 测试拾取离线邮件
    std::cout << "[Test] pick_up_mail_data()" << std::endl;
    if (!mail_mgr.off_mail_data_.empty()) {
        auto guid = mail_mgr.off_mail_data_.begin()->first;
        auto mail = mail_mgr.pick_up_mail_data(guid);
        if (mail) {
            std::cout << "Pick mail success, guid=" << mail->guid
                      << ", title=" << mail->title
                      << ", sender=" << mail->sender << std::endl;
        } else {
            std::cout << "Pick mail failed" << std::endl;
        }
    } else {
        std::cout << "No off-mail data found." << std::endl;
    }

    // 测试删除群邮件
//    std::cout << "[Test] delete_group_mail()" << std::endl;
//    if (!mail_mgr.group_mail_data_.empty()) {
//        auto guid = mail_mgr.group_mail_data_.begin()->first;
//        ok = mail_mgr.delete_group_mail(guid);
//        std::cout << "delete_group_mail result: " << ok << std::endl;
//    } else {
//        std::cout << "No group mail to delete." << std::endl;
//    }

//    // 模拟玩家登录
    std::cout << "[Test] process_role_login()" << std::endl;
    auto player = std::make_shared<PlayerObject>();
//    auto role_module = std::make_shared<RoleModule>(player);
//    auto mail_module = std::make_shared<MailModule>(player);
    player->init(123456);
    player->create_all_modules();

    ok = mail_mgr.process_role_login(player);
    std::cout << "process_role_login result: " << ok << std::endl;

//    std::cout << "=== MailManager Unit Test Finished ===" << std::endl;
    return 0;
}
