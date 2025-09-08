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
        static void InitGameInfo(const std::string &yaml_path);
        static void InitMysqlInfo(const std::string &yaml_path);
        // 辅助函数：通过 path 拆分
        static std::vector<std::string> SplitPath(const std::string &path) {
            std::vector<std::string> keys;
            std::stringstream ss(path);
            std::string item;
            while (std::getline(ss, item, '.')) {
                if (!item.empty()) keys.push_back(item);
            }
            return keys;
        }
        template<typename T>
        static T GetGameInfo(const std::string &path, const T &default_val) {
            if (!game_info_) return default_val;

            YAML::Node node = game_info_;
            for (auto &key : SplitPath(path)) {
                if (!node[key]) {
                    return default_val;
                }
                node = node[key];
            }

            try {
                return node.as<T>();
            } catch (...) {
                return default_val;
            }
        }

        static std::unordered_map<std::string, std::string> db_params_;
    private:
        Config() = default;
        static std::vector<spdlog::sink_ptr> CreateSinks(const YAML::Node &sinks_cfg);
        static YAML::Node game_info_;
//        static std::unordered_map<std::string, std::string> db_params_;
    };

} // namespace cfl
