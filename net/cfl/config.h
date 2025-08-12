#pragma once
#include <string>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <memory>

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace cfl {

    class Config {
    public:
        static void Init();
        static spdlog::level::level_enum LevelFromString(const std::string &level_str);
        static spdlog::async_overflow_policy AsyncOverflowPolicyFromString(const std::string &policy_str);
        static void InitLogging(const std::string &yaml_path);

    private:
        Config() = default;
        static std::vector<spdlog::sink_ptr> CreateSinks(const YAML::Node &sinks_cfg);
    };

} // namespace cfl
