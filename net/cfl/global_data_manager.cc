#include "cfl/db/db_mysql.h"
#include "cfl/shm/obj/global_data_obj.h"

#include <cstring>      // for std::memcpy
#include <format>       // for std::format
#include <stdexcept>
#include "global_data_manager.h"
#include "server_define.h"
#include "shm/shmpool.h"

namespace cfl {

    using namespace shm;

    bool GlobalDataManager::load_data() {
        std::scoped_lock lock(mutex_);

        // 创建或获取共享内存对象
        global_data_ = (create_object<GlobalDataObject>(SHMTYPE::Global, false));
        if (!global_data_) {
            throw std::runtime_error("Failed to create GlobalDataObject.");
        }

        global_data_->lock();
        // todo: 根据服务器数据设置：服务器ID、GUID、最大在线人数、扩展数据
//        global_data_->server_id = GameService::instance().getServerId();
//
//        const std::string sql = std::format(
//                "SELECT * FROM globaldata WHERE serverid = {}",
//                GameService::instance().getServerId()
//        );
//
//        CppMySQLQuery query = DBMysql::instance().query(sql.c_str());
//        if (!query.eof()) {
//            global_data_->guid = query.getInt64Field("maxguid");
//            global_data_->max_online = query.getIntField("maxonline");
//
//            int blobLen = 0;
//            const unsigned char* blobData = query.getBlobField("exdata", blobLen);
//            if (blobData && blobLen > 0) {
//                std::memcpy(global_data_->extraData.data(), blobData,
//                            std::min(blobLen, static_cast<int>(global_data_->extraData.size() * sizeof(int32_t))));
//            }
//        }
//
//        if (global_data_->guid <= 0) {
//            const auto sid = GameService::instance().getServerId();
//            global_data_->guid = (static_cast<std::uint64_t>(sid) << 48) + 1;
//        }
//
//        // 预分配一定范围
//        global_data_->guid += 100;
        global_data_->unlock();

        return true;
    }

    std::uint64_t GlobalDataManager::make_new_guid() {
        std::scoped_lock lock(mutex_);
        if (!global_data_) {
            throw std::runtime_error("GlobalDataObject is not initialized.");
        }

        global_data_->lock();
        const auto newGuid = ++global_data_->guid;
        global_data_->unlock();

        return newGuid;
    }

    void GlobalDataManager::set_max_online(std::int32_t num) noexcept {
        std::scoped_lock lock(mutex_);
        if (!global_data_) return;

        global_data_->lock();
        global_data_->max_online = num;
        global_data_->unlock();
    }

    std::int32_t GlobalDataManager::get_max_online() const noexcept {
        std::scoped_lock lock(mutex_);
        if (!global_data_) return 0;
        return global_data_->max_online;
    }

    bool GlobalDataManager::set_extra_data(std::int32_t index, std::int32_t value) {
        if (index <= 0 || index > static_cast<std::int32_t>(global_data_->extra_data.size()))
            return false;

        std::scoped_lock lock(mutex_);
        global_data_->lock();
        global_data_->extra_data[index - 1] = value;
        global_data_->unlock();

        return true;
    }

    std::int32_t GlobalDataManager::get_extra_data(std::int32_t index) const {
        if (index <= 0 || index > static_cast<std::int32_t>(global_data_->extra_data.size()))
            return 0;

        std::scoped_lock lock(mutex_);
        return global_data_->extra_data[index - 1];
    }

} // namespace cfl
