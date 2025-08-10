#pragma once
#include <string>
#include "cfl.h"
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

// spdlog 的额外头文件
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Config {
    // 从 YAML 配置初始化 spdlog
    void InitLogging(const std::string &yaml_path);

    spdlog::async_overflow_policy AsyncOverflowPolicyFromString(const std::string &policy_str);
    // 将字符串转为 spdlog 日志级别
    spdlog::level::level_enum LevelFromString(const std::string &level_str);
}
