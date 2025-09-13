#include "config.h"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include "cfl/cfl.h"
namespace cfl {
    YAML::Node cfl::Config::game_info_;
    std::unordered_map<std::string, std::string> cfl::Config::db_params_;

    void Config::Init() {
//        spdlog::info("Current path: {}", std::filesystem::current_path().string());

        InitLogging("configs/config.yaml");
        InitGameInfo("configs/game_info.yaml");
        InitMysqlInfo("configs/mysql.yaml");
    }

    spdlog::level::level_enum Config::LevelFromString(const std::string &level_str) {
        std::string lvl = level_str;
        std::transform(lvl.begin(), lvl.end(), lvl.begin(), ::tolower);
        if (lvl == "trace") return spdlog::level::trace;
        if (lvl == "debug") return spdlog::level::debug;
        if (lvl == "info") return spdlog::level::info;
        if (lvl == "warn") return spdlog::level::warn;
        if (lvl == "error") return spdlog::level::err;
        if (lvl == "critical") return spdlog::level::critical;
        return spdlog::level::off;
    }

    spdlog::async_overflow_policy Config::AsyncOverflowPolicyFromString(const std::string &policy_str) {
        std::string p = policy_str;
        std::transform(p.begin(), p.end(), p.begin(), ::tolower);
        if (p == "block") return spdlog::async_overflow_policy::block;
        if (p == "overrun_oldest") return spdlog::async_overflow_policy::overrun_oldest;
        if (p == "discard_new") return spdlog::async_overflow_policy::discard_new;
        return spdlog::async_overflow_policy::block;
    }

    std::vector<spdlog::sink_ptr> Config::CreateSinks(const YAML::Node &sinks_cfg) {
        std::vector<spdlog::sink_ptr> sinks;
        if (!sinks_cfg) return sinks;

        for (auto &sink_cfg : sinks_cfg) {
            std::string type = sink_cfg["type"].as<std::string>();
            if (type == "console") {
                bool color = sink_cfg["color"] ? sink_cfg["color"].as<bool>() : true;
                if (color)
                    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
                else
                    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
            } else if (type == "file") {
                std::string path = sink_cfg["path"].as<std::string>();
                bool truncate = sink_cfg["truncate"] ? sink_cfg["truncate"].as<bool>() : false;
                sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, truncate));
            }
        }
        return sinks;
    }

    void Config::InitLogging(const std::string &yaml_path) {
        try {
            YAML::Node config = YAML::LoadFile(yaml_path);
            if (!config["loggers"]) {
                spdlog::error("YAML config missing 'loggers' section");
                return;
            }

            const YAML::Node &loggers_cfg = config["loggers"];
            if (!loggers_cfg.IsSequence() || loggers_cfg.size() == 0) {
                spdlog::error("'loggers' should be a non-empty sequence");
                return;
            }

            bool first_logger_set_default = false;
            for (auto &log_cfg: loggers_cfg) {
                std::string name = log_cfg["name"] ? log_cfg["name"].as<std::string>() : "unnamed_logger";
                bool async = log_cfg["async"] ? log_cfg["async"].as<bool>() : false;
                size_t queue_size = log_cfg["queue_size"] ? log_cfg["queue_size"].as<size_t>() : 8192;
                size_t thread_count = log_cfg["thread_count"] ? log_cfg["thread_count"].as<size_t>() : 1;
                std::string level_str = log_cfg["level"] ? log_cfg["level"].as<std::string>() : "info";
                std::string pattern = log_cfg["pattern"] ? log_cfg["pattern"].as<std::string>()
                                                         : "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
                std::string overflow_policy_str = log_cfg["overflow_policy"]
                                                  ? log_cfg["overflow_policy"].as<std::string>() : "block";

                auto sinks = CreateSinks(log_cfg["sinks"]);

                std::shared_ptr<spdlog::logger> logger;
                if (async) {
                    spdlog::async_overflow_policy overflow_policy = AsyncOverflowPolicyFromString(overflow_policy_str);
                    spdlog::init_thread_pool(queue_size, thread_count);
                    logger = std::make_shared<spdlog::async_logger>(
                            name, sinks.begin(), sinks.end(),
                            spdlog::thread_pool(),
                            overflow_policy
                    );
                } else {
                    logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
                }

                logger->set_level(LevelFromString(level_str));
                logger->set_pattern(pattern);

                spdlog::register_logger(logger);

                if (!first_logger_set_default) {
                    spdlog::set_default_logger(logger);
                    first_logger_set_default = true;
                }
            }

        } catch (const std::exception &ex) {
            spdlog::error("Failed to initialize logging: {}", ex.what());
        }
    }

    void Config::InitGameInfo(const std::string &yaml_path) {
        try {
            game_info_ = YAML::LoadFile(yaml_path);
        } catch (const std::exception &ex) {
            spdlog::error("Failed to initialize game info: {}", ex.what());
        }
    }

    void Config::InitMysqlInfo(const std::string &yaml_path) {
        namespace fs = std::filesystem;
        if (!fs::exists(yaml_path)) {
            spdlog::info("Config file not found: {}, skip loading.", yaml_path);
            return;
        }

        try {
            YAML::Node node = YAML::LoadFile(yaml_path);
            db_params_.clear();

            // 遍历 YAML 的 key-value
            for (auto it = node.begin(); it != node.end(); ++it) {
                auto key = it->first.as<std::string>();
                auto value = it->second.as<std::string>();
                db_params_[key] = value;
            }

//            spdlog::info("MySQL config loaded: {}", yaml_path);
            for (const auto &kv : db_params_) {
                spdlog::info("  {} = {}", kv.first, kv.second);
            }

        } catch (const std::exception &ex) {
            spdlog::error("Failed to initialize mysql info from {}: {}", yaml_path, ex.what());
        }
    }

} // namespace cfl
