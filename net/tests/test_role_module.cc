#include "cfl/modules/role_module.h"
#include "cfl/shm/obj/role_data_obj.h"
#include "cfl/shm/shmpool.h"
#include "cfl/playerobj.h"
#include <iostream>

using namespace cfl;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    cfl::Config::Init();
    shm::DataPoolManager::instance().init();
    // 模拟一个 PlayerObjPtr
    auto player = std::make_shared<PlayerObject>();
    if (!player) {
        std::cerr << "PlayerObject make_shared failed!" << std::endl;
        return -1;
    }
    // 创建 RoleModule
    RoleModule role_module(player);
    spdlog::info("[RoleModule] {}", role_module.get_level());

    // 初始化基础数据
    std::uint64_t role_id = 1001;
    std::string name = "TestRole";
    std::uint32_t career_id = 1;
    std::uint64_t account_id = 20001;
    std::int32_t channel = 1;
    spdlog::info("init_base_data role_id={} name={} career_id={} account_id={} channel={}",
                 role_id, name, career_id, account_id, channel);
    if (!role_module.init_base_data(role_id, name, career_id, account_id, channel)) {
        std::cerr << "init_base_data failed!" << std::endl;
        return -1;
    }
    std::cout << "初始化完成, role_id=" << role_module.get_role_id()
              << " name=" << role_module.get_name()
              << " career=" << role_module.get_career_id() << std::endl;

    // 模拟 on_create
    if (role_module.on_create(role_id)) {
        std::cout << "角色创建成功，等级=" << role_module.get_level() << std::endl;
    }

    // 登录
    if (role_module.on_login()) {
        std::cout << "登录成功, logonTime=" << role_module.get_last_logon_time() << std::endl;
    }

    // 增加经验
    std::uint64_t new_exp = role_module.add_exp(500);
    std::cout << "增加经验 500, 当前经验=" << new_exp << std::endl;

    // 行动力测试
    std::uint32_t test_action_id = 1;
    role_module.add_action(test_action_id, 10);
    auto action_val = role_module.get_action(test_action_id);
    std::cout << "增加行动力10, 当前行动力=" << action_val << std::endl;

    bool enough = role_module.check_action_enough(test_action_id, 5);
    std::cout << "检查行动力是否足够(需要5): " << (enough ? "足够" : "不足") << std::endl;

    if (role_module.cost_action(test_action_id, 5)) {
        std::cout << "消耗行动力5成功, 剩余=" << role_module.get_action(test_action_id) << std::endl;
    }

    // 登出
    if (role_module.on_logout()) {
        std::cout << "登出成功, logoffTime=" << role_module.get_last_logoff_time() << std::endl;
    }

    // 跨天
    if (role_module.on_new_day()) {
        std::cout << "跨天成功, logoffTime=" << role_module.get_last_logoff_time() << std::endl;
    }

    // 设置删除
    role_module.set_delete(true);
    std::cout << "设置角色删除标记完成" << std::endl;

    // 销毁
    if (role_module.on_destroy()) {
        std::cout << "销毁完成" << std::endl;
    }

    return 0;
}
