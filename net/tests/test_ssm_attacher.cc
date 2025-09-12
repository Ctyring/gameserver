#include <iostream>
#include <thread>
#include <chrono>
#include "cfl/shm/shmpage.h"
#include "cfl/shm/shmobj.h"

using namespace cfl::shm;

int main() {
    std::cout << "[Attacher] Starting attacher process..." << std::endl;

    // 附加到共享内存
    SharedMemoryManagerBase manager(
            1001,           // module_id
            256,            // raw_block_size
            8,              // blocks_per_page
            true            // attach_only = true, 表示附加
    );

    // 等待一会儿让 creator 创建好对象
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 通过分配机制获取已经分配的对象，而不是直接通过索引获取
    // 这里我们尝试获取已经分配的对象
    auto obj_opt = manager.allocate_object(false);
    if (!obj_opt.has_value()) {
        std::cerr << "[Attacher] Failed to get allocated object!" << std::endl;
        return 1;
    }

    SharedObject* obj = obj_opt.value();

    std::cout << "[Attacher] Attached object at " << obj
              << " check_code = " << obj->check_code()
              << " state = " << static_cast<int>(obj->state())
              << std::endl;

    if (obj->is_in_use()) {
        std::cout << "[Attacher] Object is in use, resetting..." << std::endl;
        obj->reset();
    }

    std::cout << "[Attacher] Done." << std::endl;
    return 0;
}
