#include <cassert>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <ctime>
#include "db_sqlite.h"

namespace cfl::db {

// ========================== 时间工具函数 ==========================
    inline std::tm localtime_safe(const std::time_t &time) {
        std::tm result{};
#if defined(_WIN32)
        localtime_s(&result, &time);
#else
        localtime_r(&time, &result);
#endif
        return result;
    }

    inline bool sqlite_time_to_time_t(const SQLiteTime &mt, std::time_t &ts) {
        ts = mt.ts;
        return true;
    }

    inline bool time_t_to_sqlite_time(const std::time_t &ts, SQLiteTime &mt) {
        mt = SQLiteTime(ts);
        return true;
    }

    inline std::tm sqlite_time_to_tm(const SQLiteTime &mt) {
        return localtime_safe(mt.ts);
    }

    inline SQLiteTime tm_to_sqlite_time(const std::tm &tm) {
        return SQLiteTime(std::mktime(const_cast<std::tm *>(&tm)));
    }

// ========================== SQLiteResult ==========================
    SQLiteResult::SQLiteResult(int err, std::string errstr)
            : m_errno(err), m_errstr(std::move(errstr)) {}

    SQLiteResult::~SQLiteResult() = default;

    int SQLiteResult::error_code() const { return m_errno; }

    std::string_view SQLiteResult::error_message() const { return m_errstr; }

    int SQLiteResult::row_count() const { return m_row_count; }

    int SQLiteResult::column_count() const { return m_column_count; }

    int SQLiteResult::column_bytes(int idx) const {
        if (m_current_row < 0 || m_current_row >= m_row_count) return 0;
        return static_cast<int>(m_data[m_current_row][idx].size());
    }

    int SQLiteResult::column_type(int idx) const {
        if (is_null(idx)) return SQLITE_NULL;
        return SQLITE_TEXT; // 简化处理
    }

    std::string SQLiteResult::column_name(int idx) const {
        return m_column_names[idx];
    }

    bool SQLiteResult::is_null(int idx) const {
        if (m_current_row < 0 || m_current_row >= m_row_count) return true;
        return m_data[m_current_row][idx].empty();
    }

    int8_t SQLiteResult::get_int8(int idx) const { return static_cast<int8_t>(std::stoi(m_data[m_current_row][idx])); }

    uint8_t SQLiteResult::get_uint8(int idx) const {
        return static_cast<uint8_t>(std::stoul(m_data[m_current_row][idx]));
    }

    int16_t SQLiteResult::get_int16(int idx) const {
        return static_cast<int16_t>(std::stoi(m_data[m_current_row][idx]));
    }

    uint16_t SQLiteResult::get_uint16(int idx) const {
        return static_cast<uint16_t>(std::stoul(m_data[m_current_row][idx]));
    }

    int32_t SQLiteResult::get_int32(int idx) const { return std::stoi(m_data[m_current_row][idx]); }

    uint32_t SQLiteResult::get_uint32(int idx) const {
        return static_cast<uint32_t>(std::stoul(m_data[m_current_row][idx]));
    }

    int64_t SQLiteResult::get_int64(int idx) const { return std::stoll(m_data[m_current_row][idx]); }

    uint64_t SQLiteResult::get_uint64(int idx) const { return std::stoull(m_data[m_current_row][idx]); }

    float SQLiteResult::get_float(int idx) const { return std::stof(m_data[m_current_row][idx]); }

    double SQLiteResult::get_double(int idx) const { return std::stod(m_data[m_current_row][idx]); }

    std::string SQLiteResult::get_string(int idx) const { return m_data[m_current_row][idx]; }

    std::string SQLiteResult::get_blob(int idx) const {
        return std::string(m_data[m_current_row][idx].data(), m_data[m_current_row][idx].size());
    }

    std::time_t SQLiteResult::get_time(int idx) const { return std::stoll(m_data[m_current_row][idx]); }

    bool SQLiteResult::next() {
        if (m_current_row + 1 < m_row_count) {
            ++m_current_row;
            return true;
        }
        return false;
    }

    void SQLiteResult::set_column_count(int count) { m_column_count = count; }

    void SQLiteResult::set_row_count(int count) {
        m_row_count = count;
        m_data.resize(count);
    }

    void SQLiteResult::add_column_name(const std::string &name) { m_column_names.push_back(name); }

    void SQLiteResult::set_data(int row, int col, std::string &&value) {
        if ((int) m_data[row].size() < m_column_count) m_data[row].resize(m_column_count);
        m_data[row][col] = std::move(value);
    }

// ========================== SQLite ==========================
    SQLite::SQLite(const std::unordered_map<std::string, std::string> &args)
            : m_params(args) {}

    bool SQLite::connect() {
        auto it = m_params.find("dbname");
        if (it == m_params.end()) return false;
        m_dbname = it->second;
        int rc = sqlite3_open(m_dbname.c_str(), &m_db);
        if (rc != SQLITE_OK) {
            m_last_error_code = rc;
            m_last_error_msg = sqlite3_errmsg(m_db);
            sqlite3_close(m_db);
            return false;
        }
        return true;
    }

    bool SQLite::ping() { return m_db != nullptr; }

    int SQLite::execute(std::string_view sql) {
        char *errmsg = nullptr;
        m_cmd = sql;
        int rc = sqlite3_exec(m_db, m_cmd.c_str(), nullptr, nullptr, &errmsg);
        if (rc != SQLITE_OK) {
            m_last_error_code = rc;
            m_last_error_msg = errmsg ? errmsg : "unknown";
            sqlite3_free(errmsg);
        }
        m_affected_rows = sqlite3_changes(m_db);
        return rc == SQLITE_OK ? (int) m_affected_rows : -1;
    }

    int64_t SQLite::last_insert_id() const { return sqlite3_last_insert_rowid(m_db); }

    SqlData::Ptr SQLite::query(std::string_view sql) {
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK)
            return std::make_shared<SQLiteResult>(sqlite3_errcode(m_db), sqlite3_errmsg(m_db));

        auto result = std::make_shared<SQLiteResult>();
        int col_count = sqlite3_column_count(stmt);
        result->set_column_count(col_count);
        for (int i = 0; i < col_count; i++)
            result->add_column_name(sqlite3_column_name(stmt, i));

        int row = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            result->set_row_count(row + 1);
            for (int i = 0; i < col_count; i++) {
                const unsigned char *txt = sqlite3_column_text(stmt, i);
                result->set_data(row, i, txt ? std::string(reinterpret_cast<const char *>(txt)) : "");
            }
            row++;
        }

        sqlite3_finalize(stmt);
        return result;
    }

    Transaction::Ptr SQLite::open_transaction(bool auto_commit) {
        return SQLiteTransaction::create(shared_from_this(), auto_commit);
    }

    Statement::Ptr SQLite::prepare(std::string_view sql) {
        return SQLiteStatement::create(shared_from_this(), sql);
    }

    int SQLite::error_code() const { return m_last_error_code; }

    std::string_view SQLite::error_message() const { return m_last_error_msg; }

    std::shared_ptr<SQLite> SQLite::getSQLite() { return shared_from_this(); }

    sqlite3 *SQLite::getRawDb() { return m_db; }

    uint64_t SQLite::affected_rows() const { return m_affected_rows; }

    bool SQLite::use(const std::string &dbname) {
        m_dbname = dbname;
        return connect();
    }

    const char *SQLite::cmd() const { return m_cmd.c_str(); }

// ========================== SQLiteTransaction ==========================
    SQLiteTransaction::SQLiteTransaction(std::shared_ptr<SQLite> db, bool auto_commit)
            : db_(std::move(db)), auto_commit_(auto_commit) {}

    SQLiteTransaction::~SQLiteTransaction() {
        if (!is_finished_ && !auto_commit_) rollback();
    }

    SQLiteTransaction::Ptr SQLiteTransaction::create(std::shared_ptr<SQLite> db, bool auto_commit) {
        auto tx = std::make_shared<SQLiteTransaction>(db, auto_commit);
        tx->begin();
        return tx;
    }

    bool SQLiteTransaction::begin() { return db_->execute("BEGIN") >= 0; }

    bool SQLiteTransaction::commit() {
        is_finished_ = true;
        return db_->execute("COMMIT") >= 0;
    }

    bool SQLiteTransaction::rollback() {
        is_finished_ = true;
        return db_->execute("ROLLBACK") >= 0;
    }

    int SQLiteTransaction::execute(std::string_view sql) { return db_->execute(sql); }

    int64_t SQLiteTransaction::last_insert_id() const { return db_->last_insert_id(); }

    int SQLiteTransaction::error_code() const { return db_->error_code(); }

    std::string_view SQLiteTransaction::error_message() const { return db_->error_message(); }

// ========================== SQLiteStatement ==========================
    SQLiteStatement::SQLiteStatement(std::shared_ptr<SQLite> db, std::string stmt)
            : db_(std::move(db)), sql_(std::move(stmt)) {}

    SQLiteStatement::~SQLiteStatement() {
        if (stmt_) sqlite3_finalize(stmt_);
    }

    SQLiteStatement::Ptr SQLiteStatement::create(std::shared_ptr<SQLite> db, std::string_view stmt) {
        auto ptr = std::make_shared<SQLiteStatement>(db, std::string(stmt));
        if (sqlite3_prepare_v2(db->getRawDb(), ptr->sql_.c_str(), -1, &ptr->stmt_, nullptr) != SQLITE_OK) {
            ptr->last_error_ = sqlite3_errcode(db->getRawDb());
            ptr->last_errmsg_ = sqlite3_errmsg(db->getRawDb());
        }
        return ptr;
    }

    int SQLiteStatement::bind(int idx, std::nullptr_t) { return sqlite3_bind_null(stmt_, idx + 1); }

    int SQLiteStatement::bind(int idx, int8_t v) { return sqlite3_bind_int(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, uint8_t v) { return sqlite3_bind_int(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, int16_t v) { return sqlite3_bind_int(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, uint16_t v) { return sqlite3_bind_int(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, int32_t v) { return sqlite3_bind_int(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, uint32_t v) { return sqlite3_bind_int64(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, int64_t v) { return sqlite3_bind_int64(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, uint64_t v) { return sqlite3_bind_int64(stmt_, idx + 1, (sqlite3_int64) v); }

    int SQLiteStatement::bind(int idx, float v) { return sqlite3_bind_double(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, double v) { return sqlite3_bind_double(stmt_, idx + 1, v); }

    int SQLiteStatement::bind(int idx, std::string_view v) {
        return sqlite3_bind_text(stmt_, idx + 1, v.data(), (int) v.size(), SQLITE_TRANSIENT);
    }

    int SQLiteStatement::bind(int idx, const void *data, int64_t size) {
        return sqlite3_bind_blob(stmt_, idx + 1, data, (int) size, SQLITE_TRANSIENT);
    }

    int SQLiteStatement::execute() {
        int rc = sqlite3_step(stmt_);
        sqlite3_reset(stmt_);
        return rc == SQLITE_DONE ? (int) sqlite3_changes(db_->getRawDb()) : -1;
    }

    int64_t SQLiteStatement::last_insert_id() const { return sqlite3_last_insert_rowid(db_->getRawDb()); }

    SqlData::Ptr SQLiteStatement::query() {
        auto result = std::make_shared<SQLiteResult>();
        int col_count = sqlite3_column_count(stmt_);
        result->set_column_count(col_count);
        for (int i = 0; i < col_count; i++)
            result->add_column_name(sqlite3_column_name(stmt_, i));

        int row = 0;
        while (sqlite3_step(stmt_) == SQLITE_ROW) {
            result->set_row_count(row + 1);
            for (int i = 0; i < col_count; i++) {
                const unsigned char *txt = sqlite3_column_text(stmt_, i);
                result->set_data(row, i, txt ? std::string(reinterpret_cast<const char *>(txt)) : "");
            }
            row++;
        }
        sqlite3_reset(stmt_);
        return result;
    }

    int SQLiteStatement::error_code() const { return last_error_; }

    std::string_view SQLiteStatement::error_message() const { return last_errmsg_; }

// ========================== SQLiteManager ==========================
    SQLiteManager::SQLiteManager() = default;

    SQLiteManager::~SQLiteManager() = default;

    Database::Ptr SQLiteManager::get(const std::string &name) {
        std::lock_guard<MutexType> lock(mutex_);
        auto it = conns_.find(name);
        if (it != conns_.end() && !it->second.empty())
            return it->second.front();
        auto def = db_defines_[name];
        auto db = std::make_shared<SQLite>(def);
        db->connect();
        conns_[name].push_back(db);
        return db;
    }

    void SQLiteManager::register_sqlite(const std::string &name,
                                        const std::unordered_map<std::string, std::string> &params) {
        std::lock_guard<MutexType> lock(mutex_);
        db_defines_[name] = params;
    }

    void SQLiteManager::register_sqlite(const std::string &name) {
        std::unordered_map<std::string, std::string> params;
        params["dbname"] = name;
        register_sqlite(name, params);
    }

    void SQLiteManager::check_connection(int sec) {
        // 简化：暂不做心跳检测
    }

    int SQLiteManager::execute(const std::string &name, std::string_view sql) {
        auto db = get(name);
        return db->execute(sql);
    }

    SqlData::Ptr SQLiteManager::query(const std::string &name, std::string_view sql) {
        auto db = get(name);
        return db->query(sql);
    }

    Transaction::Ptr SQLiteManager::open_transaction(const std::string &name, bool auto_commit) {
        auto db = get(name);
        return db->open_transaction(auto_commit);
    }

    void SQLiteManager::release_sqlite(const std::string &name, Database *db) {
        // 简化处理：这里直接留空
    }

// ========================== SQLiteUtil ==========================
    SQLiteUtil::DataPtr SQLiteUtil::query(std::string_view name, std::string_view sql) {
        return SQLiteMgr::instance()->query(std::string(name), sql);
    }

    int SQLiteUtil::execute(std::string_view name, std::string_view sql) {
        return SQLiteMgr::instance()->execute(std::string(name), sql);
    }

    // 执行格式化查询
    template<typename... Args>
    [[nodiscard]] SqlData::Ptr SQLiteUtil::query_fmt(std::string_view name, std::string_view fmt, Args &&... args) {
        std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
        return SQLiteMgr::instance()->query(std::string{name}, sql);
    }

// 带重试的查询
    [[nodiscard]] SqlData::Ptr SQLiteUtil::try_query(std::string_view name, uint32_t count, std::string_view sql) {
        SqlData::Ptr result = nullptr;
        for (uint32_t i = 0; i < count; ++i) {
            result = SQLiteMgr::instance()->query(std::string{name}, sql);
            if (result) {
                break;
            }
        }
        return result;
    }

// 带重试的格式化查询
    template<typename... Args>
    [[nodiscard]] SqlData::Ptr
    SQLiteUtil::try_query_fmt(std::string_view name, uint32_t count, std::string_view fmt, Args &&... args) {
        std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
        return try_query(name, count, sql);
    }

// 执行预编译 SQL（带参数绑定）
    template<typename... Args>
    int SQLiteUtil::execute_prepared(const char *name, const char *sql, Args &&... args) {
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

// 执行格式化更新
    template<typename... Args>
    int SQLiteUtil::execute_fmt(std::string_view name, std::string_view fmt, Args &&... args) {
        std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
        return execute(name, sql);
    }

// 带重试的更新
    int SQLiteUtil::try_execute(std::string_view name, uint32_t count, std::string_view sql) {
        int rc = -1;
        for (uint32_t i = 0; i < count; ++i) {
            rc = execute(name, sql);
            if (rc >= 0) {
                break;
            }
        }
        return rc;
    }

// 带重试的格式化更新
    template<typename... Args>
    int SQLiteUtil::try_execute_fmt(std::string_view name, uint32_t count, std::string_view fmt, Args &&... args) {
        std::string sql = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
        return try_execute(name, count, sql);
    }

} // namespace cfl::db
