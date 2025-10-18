#include <iostream>
#include <cassert>
#include "cfl/simple_manager.h"
#include "cfl/db/db_mysql.h"

using namespace cfl;

void test_load_data()
{
    std::cout << "[Test] Loading data from database...\n";
    auto &mgr = SimpleManager::instance();

    bool ok = mgr.load_data();
    assert(ok && "loadData() should return true");

    std::cout << "Total loaded records: " << mgr.get_total_count() << "\n";
    assert(mgr.get_total_count() > 0);
}

void test_create_and_modify()
{
    std::cout << "\n[Test] Creating and modifying SimpleInfo...\n";
    auto &mgr = SimpleManager::instance();

    const uint64_t roleId = 99999;
    const uint64_t accountId = 12345;
    const std::string name = "TestHero";
    const uint32_t career = 2;

    // 创建角色
    SimpleInfo *info = mgr.create_simple_info(roleId, accountId, name, career);
    assert(info != nullptr);
    assert(info->name == name);

    // 测试 getSimpleInfoById
    SimpleInfo *found = mgr.get_simple_info_by_id(roleId);
    assert(found != nullptr);
    assert(found->role_id == roleId);

    // 测试 setFightValue / getFightValue
    mgr.set_fight_value(roleId, 987654, 15);
    assert(mgr.get_fight_value(roleId) == 987654);
    assert(found->level == 15);

    // 测试 setName / getRoleIdByName
    mgr.set_name(roleId, "RenamedHero");
    assert(mgr.get_role_id_by_name("RenamedHero") == roleId);
        assert(!mgr.check_name_exist("TestHero")); // 旧名应被移除

    // 测试 setGuildId
    mgr.set_guild_id(roleId, 555);
    assert(mgr.get_guild_id(roleId) == 555);

    // 测试 VIP 等级修改
    mgr.set_vip_level(roleId, 7);
    assert(found->vip_level == 7);

    // 测试删除标志
    mgr.set_role_deleted(roleId, true);
    assert(found->is_deleted == true);

    std::cout << "Role " << found->name << " modified successfully.\n";
}

void test_check_functions()
{
    std::cout << "\n[Test] Checking helper functions...\n";
    auto &mgr = SimpleManager::instance();

    // 名字检测
    assert(!mgr.check_name_format("a!"));            // 太短
    assert(!mgr.check_name_format("ThisNameIsTooLongForTheGameCharacter")); // 太长
    assert(!mgr.check_name_format("bad,name"));      // 含非法字符
    assert(mgr.check_name_format("GoodName"));       // 合法

    // 检测角色名存在
    assert(mgr.check_name_exist("RenamedHero"));
    assert(!mgr.check_name_exist("NoSuchName"));

    // 测试通过账号 ID 获取角色
    std::vector<uint64_t> roleIds;
    mgr.get_role_ids_by_account_id(12345, roleIds);
    assert(!roleIds.empty());
    std::cout << "Account 12345 owns " << roleIds.size() << " roles.\n";
}

int main()
{
    std::cout << "=============================\n";
    std::cout << "SimpleManager Unit Test Begin\n";
    std::cout << "=============================\n\n";
    cfl::Config::Init();
    // 模拟数据库加载
    test_load_data();

    // 创建和修改角色信息
    test_create_and_modify();

    // 检查辅助函数
    test_check_functions();

    std::cout << "\nAll tests passed successfully!\n";
    return 0;
}
