#include "shmpool.h"
#include "obj/role_data_obj.h"
namespace cfl::shm {

    DataPoolManager &DataPoolManager::instance() {
        static DataPoolManager instance;
        return instance;
    }

    bool DataPoolManager::init() {
        int area_id = Config::GetGameInfo("area_id", -1);
        if (area_id <= 0) {
            spdlog::error("初始化失败: areaid <= 0");
            return false;
        }

        shared_page_size_ = Config::GetGameInfo("share_page_size", -1);
        if (shared_page_size_ <= 1) {
            shared_page_size_ = 1024;
        }

        // 先清空，确保可重复 init
        data_object_pools_.clear();
        data_object_pools_.resize(static_cast<size_t>(SHMTYPE::End));

        auto add_pool = [&](SHMTYPE type, auto &&factory) {
            auto idx = static_cast<size_t>(type);
            data_object_pools_[idx] = factory();
            data_object_pools_[idx]->initialize_block_map();
            return true;
        };
        if (!add_pool(SHMTYPE::RoleData, [&] { return std::make_shared<SharedMemoryManager<RoleDataObject>>(static_cast<size_t>(SHMTYPE::RoleData), 1); })) return false;
//        data_object_pools_[static_cast<size_t>(SHMTYPE::RoleData)] = new SharedMemoryManager<RoleDataObject>(static_cast<size_t>(SHMTYPE::RoleData), 1);
//        data_object_pools_[static_cast<size_t>(SHMTYPE::RoleData)]->initialize_block_map();
        return true;
    }

    bool DataPoolManager::release() {
        data_object_pools_.clear();
        return true;
    }

    bool DataPoolManager::restore_from_shared_memory() {
        // 这里可以直接用范围 for
//        for (auto& pool : data_object_pools_) {
//            if (!pool) continue;
//
//            auto& used_map = pool->get_used_data_list();
//            for (auto& [ptr, block] : used_map) {
//                if (auto* role_obj = dynamic_cast<RoleDataObject*>(ptr)) {
//                    auto* player = CPlayerManager::GetInstancePtr()->CreatePlayer(role_obj->role_id);
//                    if (player) {
//                        player->Init(role_obj->role_id);
//                        if (auto* role_module = dynamic_cast<CRoleModule*>(player->GetModuleByType(MT_ROLE))) {
//                            role_module->RestoreData(role_obj);
//                        }
//                    }
//                }
//            }
//        }

        return true;
    }

    SharedMemoryManagerBasePtr DataPoolManager::get_shared_pool(SHMTYPE index) {
        spdlog::error("get_shared_pool");
        auto idx = static_cast<size_t>(index);
        if (idx >= data_object_pools_.size()) {
            spdlog::info("get_shared_pool 错误: index={} 超出范围", idx);
            return nullptr;
        }
        spdlog::info("get_shared_pool: index={}", idx);
        return data_object_pools_[idx];
    }

} // namespace cfl::shm
