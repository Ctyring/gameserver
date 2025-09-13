#pragma once

#include <memory>
#include <functional>
#include <map>
#include <vector>
#include "db.h"
#include "cfl/singleton.h"

#include <mysqlx/xdevapi.h>
#include "db.h"
#include <mutex>

namespace cfl::db {
    class MySQL;

    class MySQLStmt;

    struct MySQLTime {
        MySQLTime(time_t t)
                : ts(t) {}

        time_t ts;
    };

    inline std::tm localtime_safe(const std::time_t &time) {
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        return tm;
    }

    inline bool mysql_time_to_time_t(const MySQLTime &mt, std::time_t &ts) {
        ts = mt.ts;
        return true;
    }

    inline bool time_t_to_mysql_time(const std::time_t &ts, MySQLTime &mt) {
        mt.ts = ts;
        return true;
    }

    inline std::tm mysql_time_to_tm(const MySQLTime &mt) {
        return localtime_safe(mt.ts);
    }

    inline MySQLTime tm_to_mysql_time(const std::tm &tm) {
        return MySQLTime{std::mktime(const_cast<std::tm *>(&tm))};
    }

    /**
     * @brief MySQL 查询结果实现（基于 MySQL X DevAPI）
     * @details 统一封装 query / statement 返回的结果集
     */
    class MySQLResult : public SqlData {
    public:
        using Ptr = std::shared_ptr<MySQLResult>;

        MySQLResult(mysqlx::RowResult result, int err = 0, std::string errstr = {});

        ~MySQLResult() override = default;

        [[nodiscard]] int error_code() const override;

        [[nodiscard]] std::string_view error_message() const override;

        [[nodiscard]] int row_count() const override;

        [[nodiscard]] int column_count() const override;

        [[nodiscard]] int column_bytes(int idx) const override;

        [[nodiscard]] int column_type(int idx) const override;

        [[nodiscard]] std::string column_name(int idx) const override;

        [[nodiscard]] bool is_null(int idx) const override;

        [[nodiscard]] int8_t get_int8(int idx) const override;

        [[nodiscard]] uint8_t get_uint8(int idx) const override;

        [[nodiscard]] int16_t get_int16(int idx) const override;

        [[nodiscard]] uint16_t get_uint16(int idx) const override;

        [[nodiscard]] int32_t get_int32(int idx) const override;

        [[nodiscard]] uint32_t get_uint32(int idx) const override;

        [[nodiscard]] int64_t get_int64(int idx) const override;

        [[nodiscard]] uint64_t get_uint64(int idx) const override;

        [[nodiscard]] float get_float(int idx) const override;

        [[nodiscard]] double get_double(int idx) const override;

        [[nodiscard]] std::string get_string(int idx) const override;

        [[nodiscard]] std::string get_blob(int idx) const override;

        [[nodiscard]] std::time_t get_time(int idx) const override;

        bool next() override;

    private:
        int m_errno{};
        std::string m_errstr;
        mysqlx::RowResult m_result;
        mysqlx::Row m_current;
        int m_row_index{0};
    };

    class MySQLManager;

    class MySQL : public Database, public std::enable_shared_from_this<MySQL> {
        friend class MySQLManager;

    public:
        using Ptr = std::shared_ptr<MySQL>;

        explicit MySQL(const std::unordered_map<std::string, std::string> &args);

        /// 连接数据库
        bool connect();

        /// 心跳检查
        bool ping();

        // ===== SqlUpdate / SqlQuery =====
        int execute(std::string_view sql) override;

        int64_t last_insert_id() const override;

        SqlData::Ptr query(std::string_view sql) override;

        // ===== Transaction =====
        Transaction::Ptr open_transaction(bool auto_commit = false) override;

        // ===== Statement =====
        Statement::Ptr prepare(std::string_view sql) override;

        // ===== 错误信息 =====
        int error_code() const override;

        std::string_view error_message() const override;

        // ===== MySQL 特有接口 =====
        std::shared_ptr<MySQL> getMySQL();

        std::shared_ptr<mysqlx::Session> getRawSession();

        uint64_t affected_rows() const;

        bool use(const std::string &dbname);

        const char *cmd() const;

        template<typename... Args>
        int execStmt(const char *stmt, Args &&... args);

        template<class... Args>
        SqlData::Ptr queryStmt(const char *stmt, Args &&... args);

    private:
        bool is_need_check() const;

    private:
        std::unordered_map<std::string, std::string> m_params;
        std::shared_ptr<mysqlx::Session> m_session;
        std::string m_cmd;
        std::string m_dbname;
        uint64_t m_last_used_time{};
        bool m_has_error{};
        int32_t m_pool_size{};
    };

/**
     * @brief MySQL 事务实现（基于 mysqlx::Session）
     */
    class MySQLTransaction : public Transaction {
    public:
        using Ptr = std::shared_ptr<MySQLTransaction>;

        /**
         * @brief 工厂方法：创建事务对象
         * @param sess MySQL 会话对象
         * @param auto_commit 是否启用自动提交
         */
        static Ptr create(std::shared_ptr<mysqlx::Session> sess, bool auto_commit);

        ~MySQLTransaction() override;

        /// 开始事务
        bool begin() override;

        /// 提交事务
        bool commit() override;

        /// 回滚事务
        bool rollback() override;

        /// 执行 SQL 更新语句
        int execute(std::string_view sql) override;

        /// 获取最近一次插入的自增主键 ID
        [[nodiscard]] int64_t last_insert_id() const override;

        /// 获取错误码
        [[nodiscard]] int error_code() const override;

        /// 获取错误信息
        [[nodiscard]] std::string_view error_message() const override;

        /// 是否启用自动提交
        [[nodiscard]] bool is_auto_commit() const { return auto_commit_; }

        /// 是否已完成（提交或回滚）
        [[nodiscard]] bool is_finished() const { return is_finished_; }

        /// 是否出错
        [[nodiscard]] bool has_error() const { return has_error_; }

        /// 获取底层 MySQL 会话
        [[nodiscard]] std::shared_ptr<mysqlx::Session> session() const { return session_; }

    public:
        MySQLTransaction(std::shared_ptr<mysqlx::Session> sess, bool auto_commit);

    private:
        std::shared_ptr<mysqlx::Session> session_;
        bool auto_commit_{false};
        bool is_finished_{false};
        bool has_error_{false};
        int last_error_{0};
        std::string last_error_msg_;
    };

    /**
     * @brief MySQL 预编译语句实现（基于 mysqlx::SqlStatement）
     */
    class MySQLStatement : public Statement, public std::enable_shared_from_this<MySQLStatement> {
    public:
        using Ptr = std::shared_ptr<MySQLStatement>;

        /**
         * @brief 工厂方法：创建预编译语句
         * @param sess 已连接的 mysqlx::Session
         * @param stmt SQL 模板
         */
        static Ptr create(std::shared_ptr<mysqlx::Session> sess, std::string_view stmt);

        ~MySQLStatement() override;

        // ========== 绑定参数 ==========
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
        // 如果要支持时间，可以用 uint64_t 存储 time_t
        // int bind(int idx, std::time_t value) override;

        // ========== 执行 ==========
        int execute() override;

        [[nodiscard]] int64_t last_insert_id() const override;

        [[nodiscard]] SqlData::Ptr query() override;

        // ========== 错误 ==========
        [[nodiscard]] int error_code() const override;

        [[nodiscard]] std::string_view error_message() const override;

    public:
        MySQLStatement(std::shared_ptr<mysqlx::Session> sess, std::string stmt);

    private:
        std::shared_ptr<mysqlx::Session> session_;
        std::string sql_;
        std::vector<mysqlx::Value> bound_params_;
        int last_error_ = 0;
        std::string last_errmsg_;
    };

/**
     * @brief MySQL 数据库连接管理器（基于 mysqlx::Session）
     * @details 负责管理多个 MySQL 数据源，支持连接池、查询和事务。
     */
    class MySQLManager {
    public:
        using Ptr = std::shared_ptr<MySQLManager>;
        using MutexType = std::mutex;

        MySQLManager();

        ~MySQLManager();
//        CFL_API static MySQLManager& instance() {
//            static MySQLManager inst; // C++11+ 局部静态保证线程安全
//            return inst;
//        }
        /**
         * @brief 获取指定名称的数据源对应的 Database 对象
         */
        [[nodiscard]] Database::Ptr get(const std::string &name);

        /**
         * @brief 注册一个 MySQL 数据源
         * @param name 数据源名称
         * @param params 参数表（host, port, user, password, database 等）
         */
        void register_mysql(const std::string &name,
                            const std::unordered_map<std::string, std::string> &params);

        /**
         * @brief 注册一个 MySQL 数据源 (使用配置文件的参数)
         * @param name 数据源名称
         */
        void register_mysql(const std::string &name);

        /**
         * @brief 检查并维持连接池的健康
         * @param sec 检查间隔秒数
         */
        void check_connection(int sec = 30);

        /// 获取/设置最大连接数
        [[nodiscard]] uint32_t max_connections() const { return max_conn_; }

        void set_max_connections(uint32_t v) { max_conn_ = v; }

        /**
         * @brief 执行 SQL 更新
         * @param name 数据源名称
         * @param sql SQL 语句
         * @return 影响的行数
         */
        int execute(const std::string &name, std::string_view sql);

        /**
         * @brief 执行格式化 SQL 更新
         */
        template<typename... Args>
        int execute_fmt(const std::string &name, std::string_view fmt, Args &&... args) {
            return execute(name, std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
        }

        /**
         * @brief 执行 SQL 查询
         * @param name 数据源名称
         * @param sql SQL 语句
         * @return 查询结果集
         */
        [[nodiscard]] SqlData::Ptr query(const std::string &name, std::string_view sql);

        /**
         * @brief 执行格式化 SQL 查询
         */
        template<typename... Args>
        [[nodiscard]] SqlData::Ptr query_fmt(const std::string &name, std::string_view fmt, Args &&... args) {
            return query(name, std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
        }

        /**
         * @brief 打开事务
         * @param name 数据源名称
         * @param auto_commit 是否自动提交
         * @return 事务对象
         */
        [[nodiscard]] Transaction::Ptr open_transaction(const std::string &name, bool auto_commit);

    private:
        /// 释放某个连接（归还到连接池）
        void release_mysql(const std::string &name, Database *db);

    private:
        uint32_t max_conn_{10};
        mutable MutexType mutex_;
        std::unordered_map<std::string, std::list<Database::Ptr>> conns_;
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> db_defines_;
    };

    typedef cfl::SingletonPtr<MySQLManager> MySQLMgr;

    /**
     * @brief MySQL 工具类
     * @details 提供便捷的 SQL 查询与执行入口，基于 mysqlx::Session。
     */
    class MySQLUtil {
    public:
        using DataPtr = SqlData::Ptr;

        /// 执行查询
        [[nodiscard]] static DataPtr query(std::string_view name, std::string_view sql);

        /// 执行格式化查询
        template<typename... Args>
        [[nodiscard]] static DataPtr query_fmt(std::string_view name, std::string_view fmt, Args &&... args) {
            auto sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return query(name, sql);
        }

        /// 尝试执行查询（重试 count 次）
        [[nodiscard]] static DataPtr try_query(std::string_view name, uint32_t count, std::string_view sql);

        template<typename... Args>
        [[nodiscard]] static DataPtr
        try_query_fmt(std::string_view name, uint32_t count, std::string_view fmt, Args &&... args) {
            auto sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return try_query(name, count, sql);
        }

        /// 执行更新
        static int execute(std::string_view name, std::string_view sql);

        template<typename... Args>
        static int execute_prepared(const char *name, const char *sql, Args &&... args) {
            auto db = MySQLMgr::instance()->get(std::string{name});
            auto mysql = std::dynamic_pointer_cast<MySQL>(db);
            if (!mysql) {
                return -1;
            }

            return mysql->execStmt(sql, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static int execute_fmt(std::string_view name, std::string_view fmt, Args &&... args) {
            auto sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return execute(name, sql);
        }

        /// 尝试执行更新（重试 count 次）
        static int try_execute(std::string_view name, uint32_t count, std::string_view sql);

        template<typename... Args>
        static int try_execute_fmt(std::string_view name, uint32_t count, std::string_view fmt, Args &&... args) {
            auto sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return try_execute(name, count, sql);
        }
    };

    namespace {
        template<size_t N, typename... Args>
        struct MySQLBinder {
            static int Bind(MySQLStatement::Ptr) { return 0; }
        };

        template<typename... Args>
        int bindX(MySQLStatement::Ptr stmt, Args &... args) {
            return MySQLBinder<1, Args...>::Bind(stmt, args...);
        }

        template<size_t N, typename T, typename... Tail>
        struct MySQLBinder<N, T, Tail...> {
            static int Bind(MySQLStatement::Ptr stmt, T value, Tail &... tail) {
                int rt = BindSingle(stmt, value);
                if (rt != 0) return rt;
                if constexpr (sizeof...(Tail) > 0) {
                    return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
                }
                return 0;
            }

        private:
            static int BindSingle(MySQLStatement::Ptr stmt, int8_t v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, uint8_t v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, int16_t v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, uint16_t v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, int32_t v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, uint32_t v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, int64_t v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, uint64_t v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, float v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, double v) { return stmt->bind(N, v); }

            static int BindSingle(MySQLStatement::Ptr stmt, const std::string &v) { return stmt->bind(N,
                                                                                                      std::string_view(
                                                                                                              v));
            }

            static int BindSingle(MySQLStatement::Ptr stmt, std::string_view v) { return stmt->bind(N, v); }
        };
    }

    template<typename... Args>
    int MySQL::execStmt(const char *stmt, Args &&... args) {
        auto st = MySQLStatement::create(m_session, stmt);
        if (!st) {
            spdlog::error("[MySQL][execStmt] bind error: {}", stmt);
            return -1;
        }
        int rt = bindX(st, args...);
        if (rt != 0) {
            spdlog::error("[MySQL][execStmt] bind error: {}", stmt);
            return rt;
        }
        auto res = st->execute();
        if (res == -1) {
            spdlog::error("[MySQL][execStmt] execute error: {} {}", res ,stmt);
            return -1;
        }
        return res;
    }

    template<class... Args>
    SqlData::Ptr MySQL::queryStmt(const char *stmt, Args &&... args) {
        auto st = MySQLStatement::create(m_session, stmt);
        if (!st) {
            return nullptr;
        }
        int rt = bindX(st, args...);
        if (rt != 0) {
            return nullptr;
        }
        return st->query();
    }
}