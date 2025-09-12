#include <iostream>
#include <windows.h>
#include "cfl/shm/obj/role_data_obj.h"
#include "cfl/db/db_mysql.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace cfl::shm;

int main() {
    try {
        // 设置控制台为 UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 初始化配置
        cfl::Config::Init();

        // 注册数据库
        cfl::db::MySQLMgr::instance()->register_mysql("player.save");
        cfl::db::MySQLMgr::instance()->register_mysql("player.delete");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 创建测试用的RoleDataObject
        spdlog::info("=== 创建RoleDataObject测试对象 ===");
        RoleDataObject role;
        role.roleId = 1001;
        role.accountId = 101;
        role.name = "测试角色";
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
        role.createTime = 1609459200;  // 2021-01-01 00:00:00
        role.logonTime = 1609545600;   // 2021-01-02 00:00:00
        role.logoffTime = 1609552800;  // 2021-01-02 02:00:00
        role.groupMailTime = 1609560000;
        role.guildId = 10001;
        role.onlineTime = 7200;
        role.signNum = 5;
        role.signDay = 20210101;
        role.recvAction = 1;

        spdlog::info("RoleDataObject创建完成");
        spdlog::info("角色ID: {}, 角色名: {}, 等级: {}", role.roleId, role.name, role.level);

        // 测试保存功能
        spdlog::info("=== 测试保存功能 ===");
        bool save_result = role.Save();
        if (save_result) {
            spdlog::info("RoleDataObject保存成功");
        } else {
            spdlog::error("RoleDataObject保存失败");
        }

        // 测试删除功能
        spdlog::info("=== 测试删除功能 ===");
        bool delete_result = role.Delete();
        if (delete_result) {
            spdlog::info("RoleDataObject删除成功");
        } else {
            spdlog::error("RoleDataObject删除失败");
        }

        // 测试再次保存以确保表结构正确
        spdlog::info("=== 再次保存测试 ===");
        role.isDeleted = false;
        save_result = role.Save();
        if (save_result) {
            spdlog::info("RoleDataObject再次保存成功");

            // 验证数据是否正确保存
            auto query_result = cfl::db::MySQLUtil::query_fmt("player.save",
                                                              "SELECT name, level, carrerid FROM player WHERE id = {}", role.roleId);
            if (query_result && query_result->next()) {
                std::string name = query_result->get_string(0);
                int level = query_result->get_int32(1);
                int carrer = query_result->get_int32(2);

                spdlog::info("数据库查询结果 - 角色名: {}, 等级: {}, 职业: {}", name, level, carrer);
                if (name == role.name && level == role.level && carrer == role.carrerId) {
                    spdlog::info("数据验证成功，保存的数据与原始数据一致");
                } else {
                    spdlog::error("数据验证失败，保存的数据与原始数据不一致");
                }
            } else {
                spdlog::error("无法查询到保存的角色数据");
            }
        } else {
            spdlog::error("RoleDataObject再次保存失败");
        }

        spdlog::info("RoleDataObject测试完成!");

    } catch (const std::exception &ex) {
        spdlog::error("异常: {}", ex.what());
        return 1;
    } catch (...) {
        spdlog::error("未知异常");
        return 1;
    }

    return 0;
}
