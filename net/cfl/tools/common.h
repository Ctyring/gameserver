#include <string>
#include <string_view>

namespace cfl {
    inline std::string EscapeSQLString(std::string_view input) {
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
}
