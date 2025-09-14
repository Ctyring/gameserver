#include <iostream>
#include <windows.h>
#include <chrono>
#include "cfl/shm/obj/role_data_obj.h"
#include "cfl/db/db_mysql.h"
#include "cfl/db/db_sqlite.h"
#include "cfl/shm/obj/role_data_obj.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace cfl::shm;

// 工具函数：创建测试用 RoleDataObject
void make_role(RoleDataObject &role, int id) {
    role.roleId = id;
    role.accountId = 100 + id;
    strncpy_s(role.name, sizeof(role.name), "性能测试角色", sizeof(role.name) - 1);
    role.carrerId = 1;
    role.level = 10;
    role.action[0] = 100;
    role.action[1] = 200;
    role.action[2] = 300;
    role.action[3] = 400;
    role.actime[0] = 1000;
    role.actime[1] = 2000;
    role.actime[2] = 3000;
    role.actime[3] = 4000;
    role.exp = 5000;
    role.langId = 1;
    role.fightValue = 9999;
    role.vipLevel = 2;
    role.vipExp = 200;
    role.cityCopyId = 5;
    role.channel = 1;
    role.isDeleted = false;
    role.qq = 123456789;
    role.createTime = 1609459200;
    role.logonTime = 1609545600;
    role.logoffTime = 1609552800;
    role.groupMailTime = 1609560000;
    role.guildId = 10001;
    role.onlineTime = 7200;
    role.signNum = 5;
    role.signDay = 20210101;
    role.recvAction = 1;
}

// 测试函数
template<typename SaveFunc, typename DeleteFunc>
void test_performance(const std::string &db_name, SaveFunc save_func, DeleteFunc delete_func) {
    constexpr int N = 1000;  // 测试 1000 条数据

    spdlog::info("=== [{}] 插入 {} 条数据性能测试 ===", db_name, N);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= N; i++) {
        RoleDataObject role;
        make_role(role, i);
        save_func(role);
    }
    auto end = std::chrono::high_resolution_clock::now();
    spdlog::info("[{}] 插入耗时: {} ms", db_name,
                 std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    // 查询测试
    spdlog::info("=== [{}] 查询性能测试 ===", db_name);
    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= N; i++) {
        if (db_name == "MySQL"){
            auto query_result = cfl::db::MySQLUtil::query_fmt("gameserver",
                                                              "SELECT id FROM role WHERE id = {}", i);
        }

        if (db_name == "SQLite"){
            auto query_result = cfl::db::SQLiteUtil::query_fmt("gameserver",
                                                               "SELECT id FROM role WHERE id = {}", i);
        }
//        if (!query_result) continue;
    }
    end = std::chrono::high_resolution_clock::now();
    spdlog::info("[{}] 查询耗时: {} ms", db_name,
                 std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    // 删除测试
    spdlog::info("=== [{}] 删除 {} 条数据性能测试 ===", db_name, N);
    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= N; i++) {
        RoleDataObject role;
        make_role(role, i);
        delete_func(role);
    }
    end = std::chrono::high_resolution_clock::now();
    spdlog::info("[{}] 删除耗时: {} ms", db_name,
                 std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
}

int main() {
    try {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        cfl::Config::Init();

        // MySQL 测试
        cfl::db::MySQLMgr::instance()->register_mysql("gameserver");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        test_performance(
                "MySQL",
                [](RoleDataObject &r) { return r.Save(); },
                [](RoleDataObject &r) { return r.Delete(); });

        // SQLite 测试
        cfl::db::SQLiteMgr::instance()->register_sqlite("gameserver");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 建表
        cfl::db::SQLiteUtil::execute("gameserver", "DROP TABLE IF EXISTS role");
        std::string create_table_sql = R"(
            CREATE TABLE role (
                id          INTEGER PRIMARY KEY,
                accountid   INTEGER,
                name        TEXT,
                carrerid    INTEGER,
                level       INTEGER,
                exp         INTEGER,
                citycopyid  INTEGER,
                langid      INTEGER,
                viplevel    INTEGER,
                vipexp      INTEGER,
                action1     INTEGER,
                action2     INTEGER,
                action3     INTEGER,
                action4     INTEGER,
                actime1     INTEGER,
                actime2     INTEGER,
                actime3     INTEGER,
                actime4     INTEGER,
                createtime  INTEGER,
                logontime   INTEGER,
                logofftime  INTEGER,
                grouptime   INTEGER,
                fightvalue  INTEGER,
                guildid     INTEGER,
                isdelete    INTEGER,
                qq          INTEGER,
                onlinetime  INTEGER,
                signnum     INTEGER,
                signday     INTEGER,
                recvaction  INTEGER,
                channel     INTEGER
            );
        )";
        cfl::db::SQLiteUtil::execute("gameserver", create_table_sql);

        test_performance(
                "SQLite",
                [](RoleDataObject &r) { return r.SaveSQLite(); },
                [](RoleDataObject &r) { return r.DeleteSQLite(); });

        spdlog::info("性能测试完成");
    } catch (const std::exception &ex) {
        spdlog::error("异常: {}", ex.what());
        return 1;
    } catch (...) {
        spdlog::error("未知异常");
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1000));
    return 0;
}
