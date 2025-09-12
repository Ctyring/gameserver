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
    spdlog::info("[RoleDataObject Attacher] Starting attacher process...");

    // 附加到共享内存
    SharedMemoryManager<RoleDataObject> manager(
            2001,           // module_id，与creator保持一致
            8,              // blocks_per_page
            true            // attach_only = true, 表示附加
    );

    // 等待一会儿让 creator 创建好对象
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 获取已经分配的对象
    auto obj_opt = manager.allocate_object(false);
    if (!obj_opt.has_value()) {
        spdlog::error("[RoleDataObject Attacher] Failed to get allocated RoleDataObject!");
        return 1;
    }

    RoleDataObject* role = obj_opt.value();

//    spdlog::info("[RoleDataObject Attacher] Attached RoleDataObject at {}", role);
    spdlog::info("[RoleDataObject Attacher] Role ID: {}", role->roleId);
    spdlog::info("[RoleDataObject Attacher] Role Name: {}", role->name);
    spdlog::info("[RoleDataObject Attacher] Role Level: {}", role->level);
    spdlog::info("[RoleDataObject Attacher] Check code = {}", role->check_code());
    spdlog::info("[RoleDataObject Attacher] State = {}", static_cast<int>(role->state()));

    // 验证数据一致性
    if (role->roleId == 1001 && strcmp(role->name, "测试角色") == 0 && role->level == 10) {
        spdlog::info("[RoleDataObject Attacher] Data validation passed!");
    } else {
        spdlog::warn("[RoleDataObject Attacher] Data validation failed!");
    }

    // 修改部分数据
    role->level = 15;
    role->exp = 7500;

    spdlog::info("[RoleDataObject Attacher] Updated level to {}", role->level);
    spdlog::info("[RoleDataObject Attacher] Updated exp to {}", role->exp);

    if (role->is_in_use()) {
        spdlog::info("[RoleDataObject Attacher] RoleDataObject is in use, resetting...");
        role->reset();
    }

    spdlog::info("[RoleDataObject Attacher] Done.");
    return 0;
}
