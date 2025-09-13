#pragma once

#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include "db.h"
#include "cfl/singleton.h"
#include <sqlite3.h>

struct sqlite3;
struct sqlite3_stmt;

namespace cfl::db {

    class SQLite;
    class SQLiteStmt;

    /**
     * @brief SQLite 时间类型封装
     * @details 用于在 SQLite 和 C++ 的 time_t / tm 之间进行转换。
     */
    struct SQLiteTime {
        /**
         * @brief 构造函数
         * @param t POSIX 时间戳
         */
        explicit SQLiteTime(time_t t) : ts(t) {}

        /// 时间戳
        time_t ts;
    };

    /**
     * @brief 跨平台安全 localtime 封装
     * @param time 输入时间戳
     * @return 本地时间 tm 结构
     */
    inline std::tm localtime_safe(const std::time_t &time);

    /**
     * @brief SQLiteTime 转换为 time_t
     * @param mt SQLiteTime
     * @param ts 输出 time_t
     * @return 是否转换成功
     */
    inline bool sqlite_time_to_time_t(const SQLiteTime &mt, std::time_t &ts);

    /**
     * @brief time_t 转换为 SQLiteTime
     * @param ts 输入 time_t
     * @param mt 输出 SQLiteTime
     * @return 是否转换成功
     */
    inline bool time_t_to_sqlite_time(const std::time_t &ts, SQLiteTime &mt);

    /**
     * @brief SQLiteTime 转换为 tm
     * @param mt SQLiteTime
     * @return tm 结构
     */
    inline std::tm sqlite_time_to_tm(const SQLiteTime &mt);

    /**
     * @brief tm 转换为 SQLiteTime
     * @param tm 输入 tm 结构
     * @return SQLiteTime
     */
    inline SQLiteTime tm_to_sqlite_time(const std::tm &tm);

    /**
     * @brief SQLite 查询结果封装
     * @details 统一封装 query / statement 的返回结果，提供按列访问。
     */
    class SQLiteResult : public SqlData {
    public:
        using Ptr = std::shared_ptr<SQLiteResult>;

        /**
         * @brief 构造函数
         * @param err 错误码
         * @param errstr 错误信息
         */
        SQLiteResult(int err = 0, std::string errstr = {});

        ~SQLiteResult() override;

        /// 获取错误码
        [[nodiscard]] int error_code() const override;

        /// 获取错误信息
        [[nodiscard]] std::string_view error_message() const override;

        /// 返回总行数
        [[nodiscard]] int row_count() const override;

        /// 返回列数
        [[nodiscard]] int column_count() const override;

        /// 返回指定列的字节数
        [[nodiscard]] int column_bytes(int idx) const override;

        /// 返回指定列的数据类型
        [[nodiscard]] int column_type(int idx) const override;

        /// 返回列名
        [[nodiscard]] std::string column_name(int idx) const override;

        /// 判断某列是否为空
        [[nodiscard]] bool is_null(int idx) const override;

        /// 获取整型（8位有符号）
        [[nodiscard]] int8_t get_int8(int idx) const override;

        /// 获取整型（8位无符号）
        [[nodiscard]] uint8_t get_uint8(int idx) const override;

        /// 获取整型（16位有符号）
        [[nodiscard]] int16_t get_int16(int idx) const override;

        /// 获取整型（16位无符号）
        [[nodiscard]] uint16_t get_uint16(int idx) const override;

        /// 获取整型（32位有符号）
        [[nodiscard]] int32_t get_int32(int idx) const override;

        /// 获取整型（32位无符号）
        [[nodiscard]] uint32_t get_uint32(int idx) const override;

        /// 获取整型（64位有符号）
        [[nodiscard]] int64_t get_int64(int idx) const override;

        /// 获取整型（64位无符号）
        [[nodiscard]] uint64_t get_uint64(int idx) const override;

        /// 获取浮点数
        [[nodiscard]] float get_float(int idx) const override;

        /// 获取双精度浮点数
        [[nodiscard]] double get_double(int idx) const override;

        /// 获取字符串
        [[nodiscard]] std::string get_string(int idx) const override;

        /// 获取 BLOB 数据
        [[nodiscard]] std::string get_blob(int idx) const override;

        /// 获取时间
        [[nodiscard]] std::time_t get_time(int idx) const override;

        /// 移动到下一行
        bool next() override;

        // SQLite 特有方法
        void set_column_count(int count);
        void set_row_count(int count);
        void add_column_name(const std::string& name);
        void set_data(int row, int col, std::string&& value);

    private:
        int m_errno{};                                      ///< 错误码
        std::string m_errstr;                               ///< 错误消息
        int m_row_count{0};                                 ///< 总行数
        int m_column_count{0};                              ///< 总列数
        std::vector<std::string> m_column_names;            ///< 列名数组
        std::vector<std::vector<std::string>> m_data;       ///< 查询结果数据
        int m_current_row{-1};                              ///< 当前行索引
    };

    /**
     * @brief SQLite 数据库封装
     * @details 封装 SQLite 的连接、执行、查询、事务与预编译语句。
     */
    class SQLite : public Database, public std::enable_shared_from_this<SQLite> {
        friend class SQLiteManager;

    public:
        using Ptr = std::shared_ptr<SQLite>;

        /// 构造函数，传入参数表
        explicit SQLite(const std::unordered_map<std::string, std::string> &args);

        /// 连接数据库
        bool connect();

        /// 心跳检查
        bool ping();

        // ===== SQL 执行接口 =====

        /// 执行更新语句
        int execute(std::string_view sql) override;

        /// 获取最近一次插入的自增 ID
        int64_t last_insert_id() const override;

        /// 执行查询
        SqlData::Ptr query(std::string_view sql) override;

        // ===== Transaction =====
        Transaction::Ptr open_transaction(bool auto_commit = false) override;

        // ===== Statement =====
        Statement::Ptr prepare(std::string_view sql) override;

        // ===== 错误信息 =====
        int error_code() const override;
        std::string_view error_message() const override;

        // ===== SQLite 特有接口 =====

        /// 获取自身 shared_ptr
        std::shared_ptr<SQLite> getSQLite();

        /// 获取底层 SQLite 原始指针
        sqlite3* getRawDb();

        /// 获取影响的行数
        uint64_t affected_rows() const;

        /// 切换数据库文件
        bool use(const std::string &dbname);

        /// 返回最近一次执行的 SQL
        const char *cmd() const;

        /// 执行预编译语句
        template<typename... Args>
        int execStmt(const char *stmt, Args &&... args);

        /// 查询预编译语句
        template<class... Args>
        SqlData::Ptr queryStmt(const char *stmt, Args &&... args);

    private:
        bool is_need_check() const;
        std::string escape_string(std::string_view str) const;

    private:
        std::unordered_map<std::string, std::string> m_params; ///< 连接参数
        sqlite3* m_db{nullptr};                                ///< SQLite 连接对象
        std::string m_cmd;                                     ///< 最近执行 SQL
        std::string m_dbname;                                  ///< 数据库名称
        uint64_t m_last_used_time{};                           ///< 最近使用时间
        bool m_has_error{false};                               ///< 是否出错
        int32_t m_pool_size{};                                 ///< 连接池大小
        int m_last_error_code{0};                              ///< 最近错误码
        std::string m_last_error_msg;                          ///< 最近错误信息
        int64_t m_last_insert_id{0};                           ///< 最近插入 ID
        uint64_t m_affected_rows{0};                           ///< 影响行数
    };

    /**
     * @brief SQLite 事务封装
     */
    class SQLiteTransaction : public Transaction {
    public:
        using Ptr = std::shared_ptr<SQLiteTransaction>;

        /// 工厂方法
        static Ptr create(std::shared_ptr<SQLite> db, bool auto_commit);

        ~SQLiteTransaction() override;

        /// 开始事务
        bool begin() override;

        /// 提交事务
        bool commit() override;

        /// 回滚事务
        bool rollback() override;

        /// 执行 SQL 更新
        int execute(std::string_view sql) override;

        /// 获取最近一次插入 ID
        [[nodiscard]] int64_t last_insert_id() const override;

        /// 获取错误码
        [[nodiscard]] int error_code() const override;

        /// 获取错误信息
        [[nodiscard]] std::string_view error_message() const override;

        /// 是否自动提交
        [[nodiscard]] bool is_auto_commit() const { return auto_commit_; }

        /// 是否已结束（提交或回滚）
        [[nodiscard]] bool is_finished() const { return is_finished_; }

        /// 是否出错
        [[nodiscard]] bool has_error() const { return has_error_; }

        /// 获取底层数据库
        [[nodiscard]] std::shared_ptr<SQLite> database() const { return db_; }

    public:
        SQLiteTransaction(std::shared_ptr<SQLite> db, bool auto_commit);

    private:
        std::shared_ptr<SQLite> db_;   ///< SQLite 数据库对象
        bool auto_commit_{false};      ///< 是否自动提交
        bool is_finished_{false};      ///< 是否已完成
        bool has_error_{false};        ///< 是否出错
        int last_error_{0};            ///< 最近错误码
        std::string last_error_msg_;   ///< 最近错误信息
    };

    /**
     * @brief SQLite 预编译语句封装
     */
    class SQLiteStatement : public Statement, public std::enable_shared_from_this<SQLiteStatement> {
    public:
        using Ptr = std::shared_ptr<SQLiteStatement>;

        /// 工厂方法：创建预编译语句
        static Ptr create(std::shared_ptr<SQLite> db, std::string_view stmt);

        ~SQLiteStatement() override;

        // ===== 参数绑定 =====
        int bind(int idx, std::nullptr_t) override;
        int bind(int idx, int8_t value) override;
        int bind(int idx, uint8_t value) override;
        int bind(int idx, int16_t value) override;
        int bind(int idx, uint16_t value) override;
        int bind(int idx, int32_t value) override;
        int bind(int idx, uint32_t value) override;
        int bind(int idx, int64_t value) override;
        int bind(int idx, uint64_t value) override;
        int bind(int idx, float value) override;
        int bind(int idx, double value) override;
        int bind(int idx, std::string_view value) override;
        int bind(int idx, const void *data, int64_t size) override;

        // ===== 执行 =====
        int execute() override;
        [[nodiscard]] int64_t last_insert_id() const override;
        [[nodiscard]] SqlData::Ptr query() override;

        // ===== 错误 =====
        [[nodiscard]] int error_code() const override;
        [[nodiscard]] std::string_view error_message() const override;

    public:
        SQLiteStatement(std::shared_ptr<SQLite> db, std::string stmt);

    private:
        std::shared_ptr<SQLite> db_;   ///< SQLite 数据库对象
        std::string sql_;              ///< SQL 模板
        sqlite3_stmt* stmt_{nullptr};  ///< SQLite 原始 stmt
        int last_error_ = 0;           ///< 错误码
        std::string last_errmsg_;      ///< 错误信息
        bool m_bound{false};           ///< 是否绑定过参数
    };

    /**
     * @brief SQLite 数据库连接池管理器
     */
    class SQLiteManager {
    public:
        using Ptr = std::shared_ptr<SQLiteManager>;
        using MutexType = std::mutex;

        SQLiteManager();
        ~SQLiteManager();

        /// 获取数据库连接
        [[nodiscard]] Database::Ptr get(const std::string &name);

        /// 注册一个数据源
        void register_sqlite(const std::string &name, const std::unordered_map<std::string, std::string> &params);

        /// 注册一个数据源（使用配置）
        void register_sqlite(const std::string &name);

        /// 定时检查连接
        void check_connection(int sec = 30);

        /// 最大连接数
        [[nodiscard]] uint32_t max_connections() const { return max_conn_; }
        void set_max_connections(uint32_t v) { max_conn_ = v; }

        /// 执行 SQL 更新
        int execute(const std::string &name, std::string_view sql);

        /// 执行格式化 SQL 更新
        template<typename... Args>
        int execute_fmt(const std::string &name, std::string_view fmt, Args &&... args) {
            // 格式化 SQL 字符串
            std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));

            // 获取数据库连接
            auto db = get(name);
            if (!db) {
                return -1;
            }

            return db->execute(sql);
        }

        /// 执行 SQL 查询
        [[nodiscard]] SqlData::Ptr query(const std::string &name, std::string_view sql);

        /// 执行格式化 SQL 查询
        template<typename... Args>
        SqlData::Ptr query_fmt(const std::string &name, std::string_view fmt, Args &&... args) {
            // 格式化 SQL 字符串
            std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));

            // 获取数据库连接
            auto db = get(name);
            if (!db) {
                spdlog::error("[db][sqlite] get db failed, name: {}", name);
                return nullptr;
            }

            return db->query(sql);
        }

        /// 打开事务
        [[nodiscard]] Transaction::Ptr open_transaction(const std::string &name, bool auto_commit);

    private:
        void release_sqlite(const std::string &name, Database *db);

    private:
        uint32_t max_conn_{10};  ///< 最大连接数
        mutable MutexType mutex_;
        std::unordered_map<std::string, std::list<Database::Ptr>> conns_; ///< 连接池
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> db_defines_; ///< 数据源定义
    };

    /// SQLite 管理器单例
    typedef cfl::SingletonPtr<SQLiteManager> SQLiteMgr;

    /**
     * @brief SQLite 工具类，提供便捷的执行接口
     */
    class SQLiteUtil {
    public:
        using DataPtr = SqlData::Ptr;

        /// 执行查询
        [[nodiscard]] static DataPtr query(std::string_view name, std::string_view sql);

        /// 执行格式化查询
        template<typename... Args>
        [[nodiscard]] static DataPtr query_fmt(std::string_view name, std::string_view fmt, Args &&... args) {
            std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return SQLiteMgr::instance()->query(std::string(name), sql);
        }

        /// 带重试的查询
        [[nodiscard]] static DataPtr try_query(std::string_view name, uint32_t count, std::string_view sql);

        /// 带重试的格式化查询
        template<typename... Args>
        [[nodiscard]] static DataPtr try_query_fmt(std::string_view name, uint32_t count, std::string_view fmt, Args &&... args) {
            std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return try_query(name, count, sql);
        }

        /// 执行更新
        static int execute(std::string_view name, std::string_view sql);

        /// 执行预编译 SQL（带参数绑定）
        template<typename... Args>
        static int execute_prepared(const char *name, const char *sql, Args &&... args) {
            auto db = SQLiteMgr::instance()->get(name);
            if (!db) {
                return -1;
            }

            auto sqlite = std::dynamic_pointer_cast<SQLite>(db);
            if (!sqlite) {
                return -1;
            }

            return sqlite->execStmt(sql, std::forward<Args>(args)...);
        }

        /// 执行格式化更新
        template<typename... Args>
        static int execute_fmt(std::string_view name, std::string_view fmt, Args &&... args) {
            std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return execute(name, sql);
        }

        /// 带重试的更新
        static int try_execute(std::string_view name, uint32_t count, std::string_view sql);

        /// 带重试的格式化更新
        template<typename... Args>
        static int try_execute_fmt(std::string_view name, uint32_t count, std::string_view fmt, Args &&... args) {
            std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return try_execute(name, count, sql);
        }
    };

    // ========================== 内部工具：参数绑定器 ==========================
    namespace {
        template<size_t N, typename... Args>
        struct SQLiteBinder;

        template<typename... Args>
        int bindX(SQLiteStatement::Ptr stmt, Args &&... args) {
            return SQLiteBinder<0, Args...>::Bind(stmt, std::forward<Args>(args)...);
        }

        template<size_t N, typename T, typename... Tail>
        struct SQLiteBinder<N, T, Tail...> {
            static int Bind(SQLiteStatement::Ptr stmt, T value, Tail &&... tail) {
                int rc = stmt->bind(static_cast<int>(N), std::forward<T>(value));
                if (rc != SQLITE_OK) {
                    return rc;
                }
                return SQLiteBinder<N + 1, Tail...>::Bind(stmt, std::forward<Tail>(tail)...);
            }
        };

        template<size_t N>
        struct SQLiteBinder<N> {
            static int Bind(SQLiteStatement::Ptr) { return SQLITE_OK; }
        };
    }

    // ========================== SQLite::execStmt 实现 ==========================
    template<typename... Args>
    int SQLite::execStmt(const char *sql, Args &&... args) {
        auto stmt = SQLiteStatement::create(getSQLite(), sql);
        if (!stmt) {
            return SQLITE_ERROR;
        }

        int rc = bindX(stmt, std::forward<Args>(args)...);
        if (rc != SQLITE_OK) {
            return rc;
        }

        return stmt->execute();
    }

    // ========================== SQLite::queryStmt 实现 ==========================
    template<class... Args>
    SqlData::Ptr SQLite::queryStmt(const char *sql, Args &&... args) {
        auto stmt = SQLiteStatement::create(getSQLite(), sql);
        if (!stmt) {
            spdlog::error("SQLite::queryStmt: SQLiteStatement::create error");
            return nullptr;
        }

        int rc = bindX(stmt, std::forward<Args>(args)...);
        if (rc != SQLITE_OK) {
            spdlog::error("SQLite::queryStmt: bindX error, rc={}", rc);
            return nullptr;
        }

        return stmt->query();
    }

} // namespace cfl::db
