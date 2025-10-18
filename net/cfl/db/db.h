#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <cstdint>
#include <ctime>
#include <format>

namespace cfl::db {

    /**
     * @brief 查询结果数据接口
     * @details 表示一次 SQL 查询的结果集，可以逐行遍历并读取各列的数据。
     */
    class SqlData {
    public:
        using Ptr = std::shared_ptr<SqlData>;
        virtual ~SqlData() = default;

        /**
         * @brief 获取错误码
         * @return 错误码（0 表示成功）
         */
        [[nodiscard]] virtual int error_code() const = 0;

        /**
         * @brief 获取错误信息
         * @return 错误描述字符串
         */
        [[nodiscard]] virtual std::string_view error_message() const = 0;

        /**
         * @brief 获取当前结果集的总行数
         */
        [[nodiscard]] virtual int row_count() const = 0;

        /**
         * @brief 获取当前结果集的列数
         */
        [[nodiscard]] virtual int column_count() const = 0;

        /**
         * @brief 获取指定列的数据字节数
         * @param idx 列索引
         */
        [[nodiscard]] virtual int column_bytes(int idx) const = 0;

        /**
         * @brief 获取指定列的数据类型
         * @param idx 列索引
         * @return 类型编号（具体由底层数据库定义）
         */
        [[nodiscard]] virtual int column_type(int idx) const = 0;

        /**
         * @brief 获取指定列的名称
         * @param idx 列索引
         * @return 列名字符串
         */
        [[nodiscard]] virtual std::string column_name(int idx) const = 0;

        /// 判断指定列是否为 NULL
        [[nodiscard]] virtual bool is_null(int idx) const = 0;

        /// 获取指定列的 int8 值
        [[nodiscard]] virtual int8_t get_int8(int idx) const = 0;
        /// 获取指定列的 uint8 值
        [[nodiscard]] virtual uint8_t get_uint8(int idx) const = 0;
        /// 获取指定列的 int16 值
        [[nodiscard]] virtual int16_t get_int16(int idx) const = 0;
        /// 获取指定列的 uint16 值
        [[nodiscard]] virtual uint16_t get_uint16(int idx) const = 0;
        /// 获取指定列的 int32 值
        [[nodiscard]] virtual int32_t get_int32(int idx) const = 0;
        /// 获取指定列的 uint32 值
        [[nodiscard]] virtual uint32_t get_uint32(int idx) const = 0;
        /// 获取指定列的 int64 值
        [[nodiscard]] virtual int64_t get_int64(int idx) const = 0;
        /// 获取指定列的 uint64 值
        [[nodiscard]] virtual uint64_t get_uint64(int idx) const = 0;
        /// 获取指定列的 float 值
        [[nodiscard]] virtual float get_float(int idx) const = 0;
        /// 获取指定列的 double 值
        [[nodiscard]] virtual double get_double(int idx) const = 0;
        /// 获取指定列的字符串
        [[nodiscard]] virtual std::string get_string(int idx) const = 0;
        /// 获取指定列的二进制数据（BLOB）
        [[nodiscard]] virtual std::vector<std::byte> get_blob(int idx) const = 0;
        /// 获取指定列的时间戳
        [[nodiscard]] virtual std::time_t get_time(int idx) const = 0;

        virtual int column_index(std::string_view name) const = 0;

        /// 判断指定列是否为 NULL
        [[nodiscard]] virtual bool is_null(std::string_view col_name) const = 0;
        /// 获取指定列的 int8 值
        [[nodiscard]] virtual int8_t get_int8(std::string_view col_name) const = 0;
        /// 获取指定列的 uint8 值
        [[nodiscard]] virtual uint8_t get_uint8(std::string_view col_name) const = 0;
        /// 获取指定列的 int16 值
        [[nodiscard]] virtual int16_t get_int16(std::string_view col_name) const = 0;
        /// 获取指定列的 uint16 值
        [[nodiscard]] virtual uint16_t get_uint16(std::string_view col_name) const = 0;
        /// 获取指定列的 int32 值
        [[nodiscard]] virtual int32_t get_int32(std::string_view col_name) const = 0;
        /// 获取指定列的 uint32 值
        [[nodiscard]] virtual uint32_t get_uint32(std::string_view col_name) const = 0;
        /// 获取指定列的 int64 值
        [[nodiscard]] virtual int64_t get_int64(std::string_view col_name) const = 0;
        /// 获取指定列的 uint64 值
        [[nodiscard]] virtual uint64_t get_uint64(std::string_view col_name) const = 0;
        /// 获取指定列的 float 值
        [[nodiscard]] virtual float get_float(std::string_view col_name) const = 0;
        /// 获取指定列的 double 值
        [[nodiscard]] virtual double get_double(std::string_view col_name) const = 0;
        /// 获取指定列的字符串
        [[nodiscard]] virtual std::string get_string(std::string_view col_name) const = 0;
        /// 获取指定列的二进制数据（BLOB）
        [[nodiscard]] virtual std::vector<std::byte> get_blob(std::string_view col_name) const = 0;
        /// 获取指定列的时间戳
        [[nodiscard]] virtual std::time_t get_time(std::string_view col_name) const = 0;
        /**
         * @brief 移动到下一行数据
         * @return 是否还有数据行
         */
        virtual bool next() = 0;
    };

    /**
     * @brief SQL 更新接口
     * @details 用于执行 `INSERT` / `UPDATE` / `DELETE` 等写操作。
     */
    class SqlUpdate {
    public:
        virtual ~SqlUpdate() = default;

        /**
         * @brief 执行 SQL 更新语句
         * @param sql SQL 语句
         * @return 影响的行数或错误码
         */
        virtual int execute(std::string_view sql) = 0;

        /**
         * @brief 执行格式化 SQL 更新语句
         * @tparam Args 可变参数类型
         * @param fmt 格式化模板
         * @param args 模板参数
         */
        template <typename... Args>
        int execute_fmt(std::string_view fmt, Args&&... args) {
            return execute(std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
        }

        /**
         * @brief 获取最近一次插入的自增主键 ID
         */
        [[nodiscard]] virtual int64_t last_insert_id() const = 0;
    };

    /**
     * @brief SQL 查询接口
     * @details 用于执行 `SELECT` 查询，返回结果集。
     */
    class SqlQuery {
    public:
        virtual ~SqlQuery() = default;

        /**
         * @brief 执行 SQL 查询语句
         * @param sql SQL 语句
         * @return 查询结果集对象
         */
        [[nodiscard]] virtual SqlData::Ptr query(std::string_view sql) = 0;

        /**
         * @brief 执行格式化 SQL 查询语句
         * @tparam Args 可变参数类型
         * @param fmt 格式化模板
         * @param args 模板参数
         */
        template <typename... Args>
        [[nodiscard]] SqlData::Ptr query_fmt(std::string_view fmt, Args&&... args) {
            return query(std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
        }
    };

    /**
     * @brief 预编译语句接口
     * @details 用于执行带参数绑定的 SQL。
     */
    class Statement {
    public:
        using Ptr = std::shared_ptr<Statement>;
        virtual ~Statement() = default;

        /// 绑定 NULL
        virtual int bind(int idx, std::nullptr_t) = 0;
        /// 绑定 int8
        virtual int bind(int idx, int8_t value) = 0;
        /// 绑定 uint8
        virtual int bind(int idx, uint8_t value) = 0;
        /// 绑定 int16
        virtual int bind(int idx, int16_t value) = 0;
        /// 绑定 uint16
        virtual int bind(int idx, uint16_t value) = 0;
        /// 绑定 int32
        virtual int bind(int idx, int32_t value) = 0;
        /// 绑定 uint32
        virtual int bind(int idx, uint32_t value) = 0;
        /// 绑定 int64
        virtual int bind(int idx, int64_t value) = 0;
        /// 绑定 uint64
        virtual int bind(int idx, uint64_t value) = 0;
        /// 绑定 float
        virtual int bind(int idx, float value) = 0;
        /// 绑定 double
        virtual int bind(int idx, double value) = 0;
        /// 绑定字符串
        virtual int bind(int idx, std::string_view value) = 0;
        /// 绑定二进制数据（BLOB）
        virtual int bind(int idx, const void* data, int64_t size) = 0;
        /// 绑定时间戳（使用uint64_t）
//        virtual int bind(int idx, std::time_t value) = 0;

        /**
         * @brief 执行语句（非查询）
         * @return 影响的行数或错误码
         */
        virtual int execute() = 0;

        /**
         * @brief 获取最近一次插入的自增主键 ID
         */
        [[nodiscard]] virtual int64_t last_insert_id() const = 0;

        /**
         * @brief 执行查询并返回结果集
         */
        [[nodiscard]] virtual SqlData::Ptr query() = 0;

        /// 获取错误码
        [[nodiscard]] virtual int error_code() const = 0;
        /// 获取错误描述
        [[nodiscard]] virtual std::string_view error_message() const = 0;
    };

    /**
     * @brief 事务接口
     * @details 用于手动控制事务（开启、提交、回滚）。
     */
    class Transaction : public SqlUpdate {
    public:
        using Ptr = std::shared_ptr<Transaction>;
        virtual ~Transaction() = default;

        /// 开始事务
        virtual bool begin() = 0;
        /// 提交事务
        virtual bool commit() = 0;
        /// 回滚事务
        virtual bool rollback() = 0;
        virtual int error_code() const = 0;
        virtual std::string_view error_message() const = 0;
    };

    /**
     * @brief 数据库主接口
     * @details 提供统一的 SQL 执行、查询、事务与预编译语句功能。
     */
    class Database : public SqlUpdate, public SqlQuery {
    public:
        using Ptr = std::shared_ptr<Database>;
        virtual ~Database() = default;

        /**
         * @brief 预编译 SQL 语句
         * @param stmt SQL 模板
         * @return 预编译语句对象
         */
        [[nodiscard]] virtual Statement::Ptr prepare(std::string_view stmt) = 0;

        /// 获取错误码
        [[nodiscard]] virtual int error_code() const = 0;
        /// 获取错误描述
        [[nodiscard]] virtual std::string_view error_message() const = 0;

        /**
         * @brief 打开事务
         * @param auto_commit 是否启用自动提交
         * @return 事务对象
         */
        [[nodiscard]] virtual Transaction::Ptr open_transaction(bool auto_commit = false) = 0;
    };

} // namespace cfl::db
