#include "cfl/cfl.h"

int main() {
    // 初始化日志器（默认输出到控制台）
    spdlog::set_level(spdlog::level::debug);
    
    // 记录不同级别日志
    spdlog::debug("This is a debug message");
    spdlog::info("Welcome to spdlog!");
    spdlog::error("An error occurred: {}", 404);
    
    // 刷新日志确保输出
    spdlog::shutdown();
    return 0;
}