#include <thread>
#include <chrono>
#include "spdlog/spdlog.h"
#include "cfl/shm/shmpage.h"
#include "cfl/shm/shmobj.h"
#include "cfl/shm/obj/role_data_obj.h"

using namespace cfl::shm;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    spdlog::info("[RoleDataObject Creator] Starting creator process...");

    // 创建共享内存管理器，专门为RoleDataObject设计
    SharedMemoryManager<RoleDataObject> manager(
            2001,           // module_id，使用不同的ID避免冲突
            8,              // blocks_per_page
            false           // attach_only = false, 表示创建
    );

    // 分配一个RoleDataObject对象
    auto obj_opt = manager.allocate_object(true);
    if (!obj_opt.has_value()) {
        spdlog::error("[RoleDataObject Creator] Failed to allocate RoleDataObject!");
        return 1;
    }
    spdlog::info("[RoleDataObject Creator] RoleDataObject allocated ok");

    RoleDataObject* role = obj_opt.value();

    // 初始化RoleDataObject数据
    role->roleId = 1001;
    role->accountId = 101;
    strncpy_s(role->name, sizeof(role->name), "测试角色", sizeof(role->name) - 1);
    role->carrerId = 1;
    role->level = 10;
    role->action[0] = 100;
    role->action[1] = 200;
    role->action[2] = 300;
    role->action[3] = 400;
    role->actime[0] = 1000;
    role->actime[1] = 2000;
    role->actime[2] = 3000;
    role->actime[3] = 4000;
    role->exp = 5000;
    role->langId = 1;
    role->fightValue = 9999;
    role->vipLevel = 2;
    role->vipExp = 200;
    role->cityCopyId = 5;
    role->channel = 1;
    role->isDeleted = false;
    role->qq = 123456789;
    role->createTime = 1609459200;  // 2021-01-01 00:00:00
    role->logonTime = 1609545600;   // 2021-01-02 00:00:00
    role->logoffTime = 1609552800;  // 2021-01-02 02:00:00
    role->groupMailTime = 1609560000;
    role->guildId = 10001;
    role->onlineTime = 7200;
    role->signNum = 5;
    role->signDay = 20210101;
    role->recvAction = 1;

    // 设置检查码和状态
    role->set_check_code(54321);
    role->use();

//    spdlog::info("[RoleDataObject Creator] RoleDataObject allocated at {:p}", role);
    spdlog::info("[RoleDataObject Creator] Role ID: {}", role->roleId);
    spdlog::info("[RoleDataObject Creator] Role Name: {}", role->name);
    spdlog::info("[RoleDataObject Creator] Role Level: {}", role->level);
    spdlog::info("[RoleDataObject Creator] Check code = {}", role->check_code());
    spdlog::info("[RoleDataObject Creator] State = {}", static_cast<int>(role->state()));

    spdlog::info("[RoleDataObject Creator] Sleeping, waiting for attacher to attach...");
    std::this_thread::sleep_for(std::chrono::seconds(30));

    spdlog::info("[RoleDataObject Creator] Done.");
    return 0;
}
