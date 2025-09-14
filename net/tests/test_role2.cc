#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <spdlog/spdlog.h>
#include "cfl/shm/obj/role_data_obj.h"
#include "cfl/db/db_sqlite.h"

using namespace cfl::shm;

void worker_task(int thread_id, int ops, std::atomic<int> &success_count) {
    for (int i = 0; i < ops; i++) {
        RoleDataObject role;
        role.roleId = thread_id * 100000 + i; // 避免主键冲突
        role.accountId = thread_id;
        strncpy_s(role.name, sizeof(role.name), "并发角色", sizeof(role.name) - 1);
        role.level = 10 + i;
        role.carrerId = 1;

        // 保存
        if (!role.SaveSQLite()) continue;

        // 查询
        auto result = cfl::db::SQLiteUtil::query_fmt("gameserver",
                                                     "SELECT id FROM role WHERE id = {}", role.roleId);
        if (!result || !result->next()) continue;

        // 删除
        if (!role.DeleteSQLite()) continue;

        success_count++;
    }
}

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

template<typename SaveFunc, typename DeleteFunc>
void test_performance(const std::string &db_name, SaveFunc save_func, DeleteFunc delete_func, int thread_id, int ops,
                      std::atomic<int> &success_count) {
    int N = ops;  // 测试 1000 条数据

//    spdlog::info("=== [{}] 插入 {} 条数据性能测试 ===", db_name, N);
//    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= N; i++) {
        RoleDataObject role;
        make_role(role, thread_id * 100000 + i);
        save_func(role);
        success_count++;
    }
//    auto end = std::chrono::high_resolution_clock::now();
//    spdlog::info("[{}] 插入耗时: {} ms", db_name,
//                 std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
//
//    // 查询测试
//    spdlog::info("=== [{}] 查询性能测试 ===", db_name);
//    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= N; i++) {
        int id = thread_id * 100000 + i;
//        auto query_result = cfl::db::SQLiteUtil::query_fmt("gameserver",
//                                                           "SELECT id FROM role WHERE id = {}",(id));
        auto query_result = cfl::db::MySQLUtil::query_fmt("gameserver",
                                                           "SELECT id FROM role WHERE id = {}",(id));
    }
//    end = std::chrono::high_resolution_clock::now();
//    spdlog::info("[{}] 查询耗时: {} ms", db_name,
//                 std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
//
//    // 删除测试
//    spdlog::info("=== [{}] 删除 {} 条数据性能测试 ===", db_name, N);
//    start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= N; i++) {
        RoleDataObject role;
        make_role(role, thread_id * 100000 + i);
        delete_func(role);
    }
//    end = std::chrono::high_resolution_clock::now();
//    spdlog::info("[{}] 删除耗时: {} ms", db_name,
//                 std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
}

int main() {
    // 设置控制台 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 初始化
    cfl::Config::Init();
    cfl::db::SQLiteMgr::instance()->register_sqlite("gameserver");
    cfl::db::MySQLMgr::instance()->register_mysql("gameserver");

    // WAL 模式，提升并发写性能
    cfl::db::SQLiteUtil::execute("gameserver", "PRAGMA journal_mode=WAL;");

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

    int thread_num = 8;   // 并发线程数
    int ops_per_thread = 1000; // 每个线程的操作次数

    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

//    for (int t = 0; t < thread_num; t++) {
//        threads.emplace_back([&, t]() {
//            test_performance("SQLite",
//                             [](RoleDataObject &r) { return r.SaveSQLite(); },
//                             [](RoleDataObject &r) { return r.DeleteSQLite(); },
//                             t, ops_per_thread, success_count);
//        });
//    }
    for (int t = 0; t < thread_num; t++) {
        threads.emplace_back([&, t]() {
            test_performance("MySQL",
                             [](RoleDataObject &r) { return r.Save(); },
                             [](RoleDataObject &r) { return r.Delete(); },
                             t, ops_per_thread, success_count);
        });
    }
    for (auto &th: threads) th.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    int total_ops = thread_num * ops_per_thread;
    spdlog::info("总操作数: {}, 成功数: {}", total_ops, success_count.load());
    spdlog::info("耗时: {:.3f} 秒, QPS: {:.2f}",
                 diff.count(), success_count.load() / diff.count());

    std::this_thread::sleep_for(std::chrono::seconds(1000));
    return 0;
}
