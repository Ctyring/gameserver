#include <iostream>
#include <cassert>
#include "cfl/simple_manager.h"
#include "cfl/db/db_mysql.h"

using namespace cfl;

void test_load_data()
{
    std::cout << "[Test] Loading data from database...\n";
    auto &mgr = SimpleManager::instance();

    bool ok = mgr.loadData();
    assert(ok && "loadData() should return true");

    std::cout << "Total loaded records: " << mgr.getTotalCount() << "\n";
    assert(mgr.getTotalCount() > 0);
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
    SimpleInfo *info = mgr.createSimpleInfo(roleId, accountId, name, career);
    assert(info != nullptr);
    assert(info->name == name);

    // 测试 getSimpleInfoById
    SimpleInfo *found = mgr.getSimpleInfoById(roleId);
    assert(found != nullptr);
    assert(found->role_id == roleId);

    // 测试 setFightValue / getFightValue
    mgr.setFightValue(roleId, 987654, 15);
    assert(mgr.getFightValue(roleId) == 987654);
    assert(found->level == 15);

    // 测试 setName / getRoleIdByName
    mgr.setName(roleId, "RenamedHero");
    assert(mgr.getRoleIdByName("RenamedHero") == roleId);
    assert(!mgr.checkNameExist("TestHero")); // 旧名应被移除

    // 测试 setGuildId
    mgr.setGuildId(roleId, 555);
    assert(mgr.getGuildId(roleId) == 555);

    // 测试 VIP 等级修改
    mgr.setVipLevel(roleId, 7);
    assert(found->vip_level == 7);

    // 测试删除标志
    mgr.setRoleDeleted(roleId, true);
    assert(found->is_deleted == true);

    std::cout << "Role " << found->name << " modified successfully.\n";
}

void test_check_functions()
{
    std::cout << "\n[Test] Checking helper functions...\n";
    auto &mgr = SimpleManager::instance();

    // 名字检测
    assert(!mgr.checkNameFormat("a!"));            // 太短
    assert(!mgr.checkNameFormat("ThisNameIsTooLongForTheGameCharacter")); // 太长
    assert(!mgr.checkNameFormat("bad,name"));      // 含非法字符
    assert(mgr.checkNameFormat("GoodName"));       // 合法

    // 检测角色名存在
    assert(mgr.checkNameExist("RenamedHero"));
    assert(!mgr.checkNameExist("NoSuchName"));

    // 测试通过账号 ID 获取角色
    std::vector<uint64_t> roleIds;
    mgr.getRoleIdsByAccountId(12345, roleIds);
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
