#include <iostream>
#include <windows.h>
#include "cfl/db/db_mysql.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace cfl::db;

int main() {
    try {
        // 设置控制台为 UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 初始化配置
        cfl::Config::Init();

        // 注册数据库
//        MySQLMgr::instance()->register_mysql("test");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 测试1: 基本查询
        spdlog::info("=== 测试1: 基本查询 ===");
        auto result = MySQLUtil::query("db_game", "SELECT * FROM player");
//        while (result->next()) {
//            spdlog::info("当前时间: {}", query->get_string(0));
//        }
        if (result && result->next()) {
            spdlog::info("数据: {}", result->get_int32(0));
        } else {
            if(!result){
                spdlog::error("没查到表");
            }
            spdlog::error("查询失败");
        }

        // 测试2: 执行更新语句
        spdlog::info("=== 测试2: 执行更新语句 ===");
        int affected = MySQLUtil::execute("test",
                                          "CREATE TABLE IF NOT EXISTS test_users ("
                                          "id INT AUTO_INCREMENT PRIMARY KEY, "
                                          "name VARCHAR(50), "
                                          "email VARCHAR(100), "
                                          "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                                          ")");
        spdlog::info("创建表 affected rows: {}", affected);

        // 测试3: 插入数据
        spdlog::info("=== 测试3: 插入数据 ===");
        affected = MySQLUtil::execute("test",
                                      "INSERT INTO test_users (name, email) VALUES ('张三', 'zhangsan@example.com')");
        spdlog::info("插入数据 affected rows: {}", affected);
        spdlog::info("最后插入ID: {}", MySQLMgr::instance()->get("test")->last_insert_id());

        // 测试4: 查询数据
        spdlog::info("=== 测试4: 查询数据 ===");
        auto query_result = MySQLUtil::query("test", "SELECT CAST(id AS CHAR) AS id,\n"
                                                     "       name,\n"
                                                     "       email,\n"
                                                     "       CAST(created_at AS CHAR) AS created_at\n"
                                                     "FROM test_users;");
        if (query_result) {
            spdlog::info("查询到 {} 行数据", query_result->row_count());
            spdlog::info("列数: {}", query_result->column_count());

            std::string colnames;
            for (int i = 0; i < query_result->column_count(); ++i) {
                colnames += query_result->column_name(i);
                colnames += "\t";
            }
            spdlog::info("列名: {}", colnames);

            while (query_result->next()) {
                std::string row;
                for (int i = 0; i < query_result->column_count(); ++i) {
                    if (query_result->is_null(i)) {
                        row += "NULL\t";
                    } else {
                        row += query_result->get_string(i) + "\t";
                    }
                }
                spdlog::info("{}", row);
            }
        }

        // 测试5: 预编译语句
        spdlog::info("=== 测试5: 预编译语句 ===");
        auto db = MySQLMgr::instance()->get("test");
        if (auto mysql = std::dynamic_pointer_cast<MySQL>(db)) {
            auto stmt = mysql->prepare("INSERT INTO test_users (name, email) VALUES (?, ?)");
            if (stmt) {
                stmt->bind(1, "李四");
                stmt->bind(2, "lisi@example.com");
                int result = stmt->execute();
                spdlog::info("预编译语句插入结果: {}", result);
                spdlog::info("最后插入ID: {}", stmt->last_insert_id());
            }
        }

        // 测试6: 事务处理
        spdlog::info("=== 测试6: 事务处理 ===");
        auto transaction = MySQLMgr::instance()->open_transaction("test", false);
        if (transaction) {
            transaction->begin();
            int result1 = transaction->execute(
                    "INSERT INTO test_users (name, email) VALUES ('事务用户1', 'trans1@example.com')");
            int result2 = transaction->execute(
                    "INSERT INTO test_users (name, email) VALUES ('事务用户2', 'trans2@example.com')");

            spdlog::info("事务中插入1结果: {}", result1);
            spdlog::info("事务中插入2结果: {}", result2);

            if (transaction->commit()) {
                spdlog::info("事务提交成功");
            } else {
                spdlog::error("事务提交失败");
            }
        }

        // 测试7: 格式化查询
        spdlog::info("=== 测试7: 格式化查询 ===");
        std::string name_filter = "张三";
        auto fmt_result = MySQLUtil::query_fmt("test",
                                               "SELECT * FROM test_users WHERE name = '{}'", name_filter);
        if (fmt_result && fmt_result->next()) {
            spdlog::info("找到用户: {}, 邮箱: {}", fmt_result->get_string(1), fmt_result->get_string(2));
        }

        // 测试8: 显示整张表的数据
        spdlog::info("=== 测试8: 显示整张表的数据 ===");
        auto all_data_result = MySQLUtil::query("test", "SELECT CAST(id AS CHAR) AS id,\n"
                                                        "       name,\n"
                                                        "       email,\n"
                                                        "       CAST(created_at AS CHAR) AS created_at\n"
                                                        "FROM test_users;");
        if (all_data_result) {
            spdlog::info("查询到 {} 行数据", all_data_result->row_count());
            spdlog::info("列数: {}", all_data_result->column_count());

            std::string colnames;
            for (int i = 0; i < all_data_result->column_count(); ++i) {
                colnames += all_data_result->column_name(i);
                colnames += "\t";
            }
            spdlog::info("列名: {}", colnames);

            while (all_data_result->next()) {
                std::string row;
                for (int i = 0; i < all_data_result->column_count(); ++i) {
                    if (all_data_result->is_null(i)) {
                        row += "NULL\t";
                    } else {
                        row += all_data_result->get_string(i) + "\t";
                    }
                }
                spdlog::info("{}", row);
            }
        }

        // 测试9: 清理数据
        spdlog::info("=== 测试9: 清理数据 ===");
        affected = MySQLUtil::execute("test", "DROP TABLE IF EXISTS test_users");
        spdlog::info("删除表 affected rows: {}", affected);

        spdlog::info("所有测试完成!");

    } catch (const std::exception &ex) {
        spdlog::error("异常: {}", ex.what());
        return 1;
    } catch (...) {
        spdlog::error("未知异常");
        return 1;
    }

    return 0;
}
