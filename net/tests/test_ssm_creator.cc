#include <iostream>
#include <thread>
#include <chrono>
#include "cfl/shm/shmpage.h"
#include "cfl/shm/shmobj.h"

using namespace cfl::shm;

int main() {
    std::cout << "[Creator] Starting creator process..." << std::endl;

    // 创建共享内存管理器
    SharedMemoryManager manager(
            1001,           // module_id
            256,            // raw_block_size
            8,              // blocks_per_page
            false           // attach_only = false, 表示创建
    );

    // 分配一个对象
    auto obj_opt = manager.allocate_object(true);
    if (!obj_opt.has_value()) {
        std::cerr << "[Creator] Failed to allocate object!" << std::endl;
        return 1;
    }

    SharedObject* obj = obj_opt.value();
    obj->set_check_code(12345);
    obj->use();

    std::cout << "[Creator] Object allocated at " << obj
              << " check_code = " << obj->check_code()
              << " state = " << static_cast<int>(obj->state())
              << std::endl;

    std::cout << "[Creator] Sleeping, waiting for attacher to attach..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(30));

    std::cout << "[Creator] Done." << std::endl;
    return 0;
}
