#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include "cfl/shm/shmobj.h"
#include "cfl/db/db_mysql.h"
#include "cfl/db/db.h"

namespace cfl::shm {
    constexpr size_t MAX_EXTRA_INDEX = 60;

/**
 * @brief 全局数据对象，用于存储服务器全局信息。
 */
    struct GlobalDataObject : public SharedObject {
        uint32_t server_id = 0;                                  // 服务器ID
        uint64_t guid = 0;                                       // 全局GUID
        uint32_t max_online = 0;                                 // 最大在线人数
        std::array<int32_t, MAX_EXTRA_INDEX> extra_data{};        // 扩展数据
        GlobalDataObject() = default;
        ~GlobalDataObject() = default;

        /**
         * @brief 将对象数据写入数据库（插入或替换）
         */
        bool create() {
            int ret = cfl::db::MySQLUtil::execute_prepared(
                    "db_game",
                    "REPLACE INTO globaldata (serverid, maxguid, maxonline, extradata) VALUES(?, ?, ?, ?);",
                    server_id,
                    guid,
                    max_online,
                    *extra_data.data()
            );

            return ret >= 0;
        }

        /**
         * @brief 更新数据库中的记录（等价于 create）
         */
        bool update() {
            return create();
        }

        /**
         * @brief 删除记录（占位实现）
         */
        bool remove() noexcept {
            return true;
        }
    };
}