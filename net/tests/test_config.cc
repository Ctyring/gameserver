#include <iostream>
#include "cfl/config.h"
#include <spdlog/spdlog.h>

int main() {
    // 初始化日志系统，加载 config.yaml
    Config::InitLogging("configs/config.yaml");

    // 获取默认 logger，默认是第一个配置的 logger
    auto default_logger = spdlog::default_logger();

    default_logger->info("This is an info message from the default logger.");
    default_logger->debug("This debug message might be hidden if level > debug.");

    // 获取第二个 logger，名字叫 "audit"
    auto audit_logger = spdlog::get("audit");
    if (audit_logger) {
        audit_logger->warn("Audit logger warning message.");
        audit_logger->error("Audit logger error message.");
    } else {
        std::cerr << "Failed to get 'audit' logger." << std::endl;
    }

    // 演示异步 logger日志 (假设第一个 logger 是异步)
    for (int i = 0; i < 10; ++i) {
        default_logger->info("Async log message #{}", i);
    }

    // 让异步日志线程flush下（可选）
    spdlog::shutdown();

    return 0;
}
