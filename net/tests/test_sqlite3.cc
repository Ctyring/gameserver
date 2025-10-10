// test_sqlite.cpp
#include <iostream>
#include <string>
#include "cfl/cfl.h"
#include "cfl/db/db_sqlite.h"
using namespace cfl::db;

int main() {
    // 注册数据库
    SQLiteMgr::instance()->register_sqlite("test_db", {{"dbname", "data/test.db"}});

    // 获取数据库连接
    auto db = SQLiteMgr::instance()->get("test_db");

    // 1. 创建表
    std::string drop_table_sql = "DROP TABLE IF EXISTS users;";
    db->execute(drop_table_sql);
    std::string create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            age INTEGER
        );
    )";
    if (db->execute(create_table_sql) < 0) {
        std::cerr << "Create table failed: " << db->error_message() << std::endl;
        return 1;
    }

    // 2. 插入数据（普通 SQL）
    std::string insert_sql = "INSERT INTO users (name, age) VALUES ('Alice', 30);";
    if (db->execute(insert_sql) < 0) {
        std::cerr << "Insert failed: " << db->error_message() << std::endl;
        return 1;
    }
    std::cout << "Last insert id: " << db->last_insert_id() << std::endl;

    // 3. 插入数据（预编译 SQL）
    const char* stmt = "INSERT INTO users (name, age) VALUES (?, ?);";
    auto sqlite_db = std::dynamic_pointer_cast<cfl::db::SQLite>(db);
    if (!sqlite_db) {
        std::cerr << "Failed to cast to SQLite" << std::endl;
        return 1;
    }

    int result = sqlite_db->execStmt(stmt, "Bob", 25);
    if (result < 0) {
        std::cerr << "Prepared insert failed: " << sqlite_db->error_message() << std::endl;
        return 1;
    }

    // 4. 查询数据（普通 SQL）
    auto result_data = db->query("SELECT id, name, age FROM users;");
    if (!result_data) {
        std::cerr << "Query failed" << std::endl;
        return 1;
    }

    std::cout << "Query result:" << std::endl;
    while (result_data->next()) {
        std::cout << "id=" << result_data->get_int32(0)
                  << ", name=" << result_data->get_string(1)
                  << ", age=" << result_data->get_int32(2)
                  << std::endl;
    }

    // 5. 使用事务
    auto tx = db->open_transaction(false);
    if (!tx) {
        std::cerr << "Failed to open transaction" << std::endl;
        return 1;
    }
    if (tx->execute("INSERT INTO users (name, age) VALUES ('Charlie', 28);") < 0) {
        std::cerr << "Insert in transaction failed: " << tx->error_message() << std::endl;
        return 1;
    }
    if (!tx->commit()) {
        std::cerr << "Commit failed: " << tx->error_message() << std::endl;
        return 1;
    }

    // 6. 查询数据（预编译）
    const char* select_stmt = "SELECT name, age FROM users WHERE age > ?;";
    auto query_result = sqlite_db->queryStmt(select_stmt, 26);
    if (!query_result) {
        std::cerr << "Prepared query failed: " << sqlite_db->error_message() << std::endl;
        return 1;
    }

    std::cout << "Users with age > 26:" << std::endl;
    while (query_result->next()) {
        std::cout << "name=" << query_result->get_string(0)
                  << ", age=" << query_result->get_int32(1)
                  << std::endl;
    }

    std::cout << "Test finished successfully." << std::endl;
    return 0;
}
