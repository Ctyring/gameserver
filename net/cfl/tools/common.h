#pragma once
#include <string>
#include <string_view>
#include <chrono>
#include <format>
#include <random>

namespace cfl {
    inline std::string escape_sql_string(std::string_view input) {
        std::string out;
        out.reserve(input.size() * 2); // 预留空间，避免频繁扩容
        for (char c : input) {
            switch (c) {
                case '\'':
                    out += "''"; // 单引号转义成两个单引号
                    break;
                case '\\':
                    out += "\\\\"; // 反斜杠转义
                    break;
                case '\"':
                    out += "\\\""; // 双引号转义
                    break;
                case '\0':
                    out += "\\0"; // null 字符转义
                    break;
                default:
                    out.push_back(c);
                    break;
            }
        }
        return out;
    }

    inline uint64_t get_timestamp() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    inline std::vector<std::string_view> split_string(std::string_view str, std::string_view delim, bool skipEmpty = true) {
        std::vector<std::string_view> result;
        if (delim.empty()) {  // 分隔符为空，返回整个字符串
            if (!str.empty() || !skipEmpty)
                result.push_back(str);
            return result;
        }

        size_t pos = 0;
        while (pos <= str.size()) {
            size_t next = str.find(delim, pos);
            std::string_view token = (next == std::string_view::npos) ? str.substr(pos) : str.substr(pos, next - pos);
            if (!token.empty() || !skipEmpty) {
                result.push_back(token);
            }
            if (next == std::string_view::npos) break;
            pos = next + delim.size();
        }
        return result;
    }

    inline int random_int(int min = 0, int max = 10000) {
        static std::mt19937 gen{std::random_device{}()};
        std::uniform_int_distribution<int> dis(min, max);
        return dis(gen);
    }

    inline bool string_to_vector(const char* pStrValue, std::array<float, 5>& FloatVector, char cDelim = ',')
    {
        if (!pStrValue)
            return false;

        std::string_view str(pStrValue);
        size_t index = 0;
        size_t start = 0;

        while (start < str.size() && index < FloatVector.size()) {
            size_t end = str.find(cDelim, start);
            std::string_view token = str.substr(start, end - start);

            if (!token.empty()) {
                float value{};
                auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
                if (ec == std::errc{})
                    FloatVector[index++] = value;
            }

            if (end == std::string_view::npos)
                break;

            start = end + 1;
        }

        return true;
    }
}
