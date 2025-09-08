#include "db_mysql.h"
#include <cfl/cfl.h>

using namespace std::chrono;
namespace cfl::db {
/**
 * @brief MySQL 查询结果封装类
 * @details 基于 MySQL Connector/C++ X DevAPI (mysqlx) 的 RowResult，
 *          提供统一接口以便在游戏服务器中访问查询结果。
 */
    MySQLResult::MySQLResult(mysqlx::RowResult result, int err, std::string errstr)
            : m_errno(err)                        // 错误码（执行 SQL 时的错误）
            , m_errstr(std::move(errstr))         // 错误信息（错误描述字符串）
            , m_result(std::move(result))         // 查询结果集 (RowResult)
            , m_row_index(0) {}                   // 当前行索引（用于 next() 迭代）

/**
 * @brief 获取错误码
 * @return 执行 SQL 时的错误码（0 表示无错误）
 */
    [[nodiscard]] int MySQLResult::error_code() const {
        return m_errno;
    }

/**
 * @brief 获取错误信息
 * @return 错误字符串（无错误时为空字符串）
 */
    [[nodiscard]] std::string_view MySQLResult::error_message() const {
        return m_errstr;
    }

/**
 * @brief 获取结果集的行数
 * @note  row_count() 需要对 RowResult 调用 count()，可能抛异常
 * @return 行数（失败时返回 -1）
 */
    [[nodiscard]] int MySQLResult::row_count() const {
        try {
            auto &result = const_cast<mysqlx::RowResult &>(m_result);
            return static_cast<int>(result.count());
        } catch (...) {
            return -1;
        }
    }

/**
 * @brief 获取结果集的列数
 * @return 列数（失败时返回 -1）
 */
    [[nodiscard]] int MySQLResult::column_count() const {
        try {
            return static_cast<int>(m_result.getColumnCount());
        } catch (...) {
            return -1;
        }
    }

/**
 * @brief 获取指定列的数据字节数
 * @param idx 列索引（从 0 开始）
 * @return 如果值为 NULL 返回 0；字符串返回字节长度；其他类型返回 sizeof(val)
 */
    [[nodiscard]] int MySQLResult::column_bytes(int idx) const {
        if (!m_current) return 0;
        auto val = m_current[idx];
        if (val.isNull()) return 0;
        if (val.getType() == mysqlx::Value::STRING) {
            return static_cast<int>(val.get<std::string>().size());
        }
        return sizeof(val);
    }

/**
 * @brief 获取指定列的 MySQL 类型
 * @param idx 列索引
 * @return 类型 ID（参考 mysqlx::columntype_t），越界时返回 -1
 */
    [[nodiscard]] int MySQLResult::column_type(int idx) const {
        auto &cols = m_result.getColumns();
        if (idx < 0 || idx >= static_cast<int>(m_result.getColumnCount())) {
            return -1;
        }
        return static_cast<int>(cols[idx].getType());
    }

/**
 * @brief 获取指定列的名称
 * @param idx 列索引
 * @return 列名（越界时返回空字符串）
 */
    [[nodiscard]] std::string MySQLResult::column_name(int idx) const {
        auto &cols = m_result.getColumns();
        if (idx < 0 || idx >= static_cast<int>(m_result.getColumnCount())) {
            return {};
        }
        return cols[idx].getColumnName();
    }

/**
 * @brief 判断指定列是否为 NULL
 * @param idx 列索引
 * @return true 表示该列值为 NULL
 */
    [[nodiscard]] bool MySQLResult::is_null(int idx) const {
        if (!m_current) return true;
        return m_current[idx].isNull();
    }

// =============================
//  基础类型取值接口
// =============================

    [[nodiscard]] int8_t MySQLResult::get_int8(int idx) const {
        return static_cast<int8_t>(m_current[idx].get<int32_t>());
    }

    [[nodiscard]] uint8_t MySQLResult::get_uint8(int idx) const {
        return static_cast<uint8_t>(m_current[idx].get<uint32_t>());
    }

    [[nodiscard]] int16_t MySQLResult::get_int16(int idx) const {
        return static_cast<int16_t>(m_current[idx].get<int32_t>());
    }

    [[nodiscard]] uint16_t MySQLResult::get_uint16(int idx) const {
        return static_cast<uint16_t>(m_current[idx].get<uint32_t>());
    }

    [[nodiscard]] int32_t MySQLResult::get_int32(int idx) const {
        return m_current[idx].get<int32_t>();
    }

    [[nodiscard]] uint32_t MySQLResult::get_uint32(int idx) const {
        return m_current[idx].get<uint32_t>();
    }

    [[nodiscard]] int64_t MySQLResult::get_int64(int idx) const {
        return m_current[idx].get<int64_t>();
    }

    [[nodiscard]] uint64_t MySQLResult::get_uint64(int idx) const {
        return m_current[idx].get<uint64_t>();
    }

    [[nodiscard]] float MySQLResult::get_float(int idx) const {
        return m_current[idx].get<float>();
    }

    [[nodiscard]] double MySQLResult::get_double(int idx) const {
        return m_current[idx].get<double>();
    }

/**
 * @brief 获取字符串列值
 * @param idx 列索引
 * @return 字符串（NULL 返回空字符串）
 */
    [[nodiscard]] std::string MySQLResult::get_string(int idx) const {
        if (m_current[idx].isNull()) return {};
        return m_current[idx].get<std::string>();
    }

/**
 * @brief 获取 BLOB 类型的列值
 * @param idx 列索引
 * @return 字节数组（二进制数据），存储在 std::string 中
 */
    [[nodiscard]] std::string MySQLResult::get_blob(int idx) const {
        if (m_current[idx].isNull()) return {};
        return m_current[idx].get<std::string>();
    }

/**
 * @brief 获取时间列值（转换为 time_t）
 * @param idx 列索引
 * @return 转换后的时间戳（失败或 NULL 返回 0）
 * @note 假设数据库时间格式为 "YYYY-MM-DD HH:MM:SS" 或 "YYYY-MM-DD"
 */
    [[nodiscard]] std::time_t MySQLResult::get_time(int idx) const {
        if (!m_current || m_current[idx].isNull()) {
            return 0;
        }

        // mysqlx::Value -> std::string
        auto str = m_current[idx].get<std::string>();
        if (str.empty()) {
            return 0;
        }

        // 解析日期时间字符串
        std::tm tm{};
        int year, month, day, hour, minute, second;
#ifdef _MSC_VER
        if (sscanf_s(str.c_str(), "%d-%d-%d %d:%d:%d",
                     &year, &month, &day, &hour, &minute, &second) != 6) {
            if (sscanf_s(str.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
                return 0;
            }
            hour = minute = second = 0;
        }
#else
        if (sscanf(str.c_str(), "%d-%d-%d %d:%d:%d",
               &year, &month, &day, &hour, &minute, &second) != 6) {
        if (sscanf(str.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
            return 0;
        }
        hour = minute = second = 0;
    }
#endif

        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        tm.tm_isdst = -1; // 自动判断是否夏令时

        return std::mktime(&tm);
    }

/**
 * @brief 移动到下一行数据
 * @return true 表示成功移动到下一行；false 表示没有更多行
 */
    bool MySQLResult::next() {
        if (auto row = m_result.fetchOne()) {
            m_current = std::move(row);
            ++m_row_index;
            return true;
        }
        return false;
    }

    MySQL::MySQL(const std::unordered_map<std::string, std::string> &args)
            : m_params(args) {}

    bool MySQL::connect() {
        try {
            std::string host = m_params.contains("host") ? m_params.at("host") : "127.0.0.1";
            int port = m_params.contains("port") ? std::stoi(m_params.at("port")) : 33060;
            std::string user = m_params.contains("user") ? m_params.at("user") : "root";
            std::string pwd = m_params.contains("password") ? m_params.at("password") : "";
            std::string db = m_params.contains("dbname") ? m_params.at("dbname") : "";

            m_session = std::make_shared<mysqlx::Session>(host, port, user, pwd);

            if (!db.empty()) {
                use(db);
            }

            m_has_error = false;
            m_last_used_time = duration_cast<seconds>(
                    system_clock::now().time_since_epoch()).count();
            return true;
        } catch (const mysqlx::Error &err) {
            m_has_error = true;
            m_cmd = err.what();
            return false;
        }
    }

    bool MySQL::ping() {
        try {
            if (!m_session) return false;
            m_session->sql("SELECT 1").execute();
            return true;
        } catch (...) {
            return false;
        }
    }

    int MySQL::execute(std::string_view sql) {
        try {
            auto res = m_session->sql(std::string(sql)).execute();
            return static_cast<int>(res.getAffectedItemsCount());
        } catch (const mysqlx::Error &err) {
            m_has_error = true;
            m_cmd = err.what();
            return -1;
        }
    }

    int64_t MySQL::last_insert_id() const {
        try {
            if (!m_session){
                spdlog::error("[MySQL] last_insert_id: session is null");
                return 0;
            }
            auto res = m_session->sql("SELECT LAST_INSERT_ID()").execute();
//            spdlog::info("[MySQL] last_insert_id: {}", res.getAutoIncrementValue());
            if (res.count() > 0) {
                return res.fetchOne()[0].get<int64_t>();
            }
        } catch (const mysqlx::Error &err) {
            spdlog::error("[MySQL] last_insert_id: error {}", err.what());
        }
        return 0;
    }

    SqlData::Ptr MySQL::query(std::string_view sql) {
        try {
            auto res = m_session->sql(std::string(sql)).execute();
            return std::make_shared<MySQLResult>(std::move(res)); // 需要 MySQLRes 封装 SqlData
        } catch (const mysqlx::Error &err) {
            m_has_error = true;
            m_cmd = err.what();
            return nullptr;
        }
    }

    Transaction::Ptr MySQL::open_transaction(bool auto_commit) {
        try {
            if (!auto_commit) {
                m_session->startTransaction();
            }
            return std::make_shared<MySQLTransaction>(m_session, auto_commit);
        } catch (...) {
            return nullptr;
        }
    }

    Statement::Ptr MySQL::prepare(std::string_view sql) {
        try {
            return std::make_shared<MySQLStatement>(m_session, std::string(sql));
        } catch (...) {
            return nullptr;
        }
    }

    int MySQL::error_code() const {
        return m_has_error ? -1 : 0;
    }

    std::string_view MySQL::error_message() const {
        return m_cmd;
    }

    std::shared_ptr<MySQL> MySQL::getMySQL() {
        return shared_from_this();
    }

    std::shared_ptr<mysqlx::Session> MySQL::getRawSession() {
        return m_session;
    }

    uint64_t MySQL::affected_rows() const {
        try {
            auto res = m_session->sql("SELECT ROW_COUNT()").execute();
            if (res.count() > 0) {
                return res.fetchOne()[0].get<uint64_t>();
            }
        } catch (...) {}
        return 0;
    }

    bool MySQL::use(const std::string &dbname) {
        try {
            m_session->sql("USE " + dbname).execute();
            m_dbname = dbname;
            return true;
        } catch (...) {
            return false;
        }
    }

    const char *MySQL::cmd() const {
        return m_cmd.c_str();
    }

    bool MySQL::is_need_check() const {
        auto now = duration_cast<seconds>(
                system_clock::now().time_since_epoch()).count();
        return (now - m_last_used_time) > 60;
    }

    MySQLTransaction::MySQLTransaction(std::shared_ptr<mysqlx::Session> sess, bool auto_commit)
            : session_(std::move(sess)), auto_commit_(auto_commit) {
        if (!session_) {
            throw std::invalid_argument("MySQLTransaction: session is null");
        }
    }

    MySQLTransaction::~MySQLTransaction() {
        if (!auto_commit_ && !is_finished_) {
            try {
                session_->rollback();
            } catch (const mysqlx::Error &e) {
//                std << "[MySQLTransaction::~MySQLTransaction] rollback failed: "
//                          << e.what() << std::endl;
                spdlog::error("[MySQLTransaction::~MySQLTransaction] rollback failed: {}", e.what());
            }
        }
    }

    MySQLTransaction::Ptr MySQLTransaction::create(std::shared_ptr<mysqlx::Session> sess, bool auto_commit) {
        return std::make_shared<MySQLTransaction>(std::move(sess), auto_commit);
    }

    bool MySQLTransaction::begin() {
        if (auto_commit_ || is_finished_) {
            return true;
        }
        try {
            session_->startTransaction();
            return true;
        } catch (const mysqlx::Error &e) {
            has_error_ = true;
            last_error_msg_ = e.what();
            last_error_ = -1;
            return false;
        }
    }

    bool MySQLTransaction::commit() {
        if (auto_commit_ || is_finished_) {
            return true;
        }
        try {
            session_->commit();
            is_finished_ = true;
            return true;
        } catch (const mysqlx::Error &e) {
            has_error_ = true;
            last_error_msg_ = e.what();
            last_error_ = -1;
            return false;
        }
    }

    bool MySQLTransaction::rollback() {
        if (auto_commit_ || is_finished_) {
            return true;
        }
        try {
            session_->rollback();
            is_finished_ = true;
            return true;
        } catch (const mysqlx::Error &e) {
            has_error_ = true;
            last_error_msg_ = e.what();
            last_error_ = -1;
            return false;
        }
    }

    int MySQLTransaction::execute(std::string_view sql) {
        try {
            auto result = session_->sql(std::string(sql)).execute();
            return static_cast<int>(result.getAffectedItemsCount());
        } catch (const mysqlx::Error &e) {
            has_error_ = true;
            last_error_msg_ = e.what();
            last_error_ = -1;
            return -1;
        }
    }

    int64_t MySQLTransaction::last_insert_id() const {
        try {
            auto res = session_->sql("SELECT LAST_INSERT_ID()").execute();
//            spdlog::info("[MySQLTransaction] last_insert_id: {}", res.getAutoIncrementValue());
            if (res.count() > 0) {
                auto row = res.fetchOne();
                return row[0].get<int64_t>();
            }
            return 0;
        } catch (const mysqlx::Error &e) {
            spdlog::error("[MySQLTransaction] last_insert_id failed: {}", e.what());
            return -1;
        }
    }

    int MySQLTransaction::error_code() const {
        return last_error_;
    }

    std::string_view MySQLTransaction::error_message() const {
        return last_error_msg_;
    }

    MySQLStatement::MySQLStatement(std::shared_ptr<mysqlx::Session> sess, std::string stmt)
            : session_(std::move(sess)), sql_(std::move(stmt)) {}

    MySQLStatement::~MySQLStatement() = default;

    MySQLStatement::Ptr MySQLStatement::create(std::shared_ptr<mysqlx::Session> sess, std::string_view stmt) {
        return std::make_shared<MySQLStatement>(std::move(sess), std::string(stmt));
    }

// ========== 绑定参数 ==========
    int MySQLStatement::bind(int idx, std::nullptr_t) {
        if (idx <= 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) {
            bound_params_.resize(idx);
        }
        bound_params_[idx - 1] = mysqlx::Value(); // null
        return 0;
    }

    int MySQLStatement::bind(int idx, int8_t value) { return bind(idx, static_cast<int32_t>(value)); }

    int MySQLStatement::bind(int idx, uint8_t value) { return bind(idx, static_cast<uint32_t>(value)); }

    int MySQLStatement::bind(int idx, int16_t value) { return bind(idx, static_cast<int32_t>(value)); }

    int MySQLStatement::bind(int idx, uint16_t value) { return bind(idx, static_cast<uint32_t>(value)); }

    int MySQLStatement::bind(int idx, int32_t value) {
        if (idx <= 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) bound_params_.resize(idx);
        bound_params_[idx - 1] = mysqlx::Value(value);
        return 0;
    }

    int MySQLStatement::bind(int idx, uint32_t value) {
        if (idx <= 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) bound_params_.resize(idx);
        bound_params_[idx - 1] = mysqlx::Value(value);
        return 0;
    }

    int MySQLStatement::bind(int idx, int64_t value) {
        if (idx <= 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) bound_params_.resize(idx);
        bound_params_[idx - 1] = mysqlx::Value(value);
        return 0;
    }

    int MySQLStatement::bind(int idx, uint64_t value) {
        if (idx <= 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) bound_params_.resize(idx);
        bound_params_[idx - 1] = mysqlx::Value(value);
        return 0;
    }

    int MySQLStatement::bind(int idx, float value) {
        if (idx <= 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) bound_params_.resize(idx);
        bound_params_[idx - 1] = mysqlx::Value(value);
        return 0;
    }

    int MySQLStatement::bind(int idx, double value) {
        if (idx <= 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) bound_params_.resize(idx);
        bound_params_[idx - 1] = mysqlx::Value(value);
        return 0;
    }

    int MySQLStatement::bind(int idx, std::string_view value) {
        if (idx <= 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) bound_params_.resize(idx);
        bound_params_[idx - 1] = mysqlx::Value(std::string(value));
        return 0;
    }

    int MySQLStatement::bind(int idx, const void *data, int64_t size) {
        if (idx <= 0 || !data || size < 0) return -1;
        if (bound_params_.size() < static_cast<size_t>(idx)) bound_params_.resize(idx);
        bound_params_[idx - 1] = mysqlx::Value(std::string(static_cast<const char *>(data), static_cast<size_t>(size)));
        return 0;
    }

// ========== 执行 ==========
    int MySQLStatement::execute() {
        try {
            auto sql = session_->sql(sql_);
            if (!bound_params_.empty()) {
                sql.bind(bound_params_.begin(), bound_params_.end());
            }
            auto res = sql.execute();
            last_error_ = 0;
            last_errmsg_.clear();
            return static_cast<int>(res.getAffectedItemsCount());
        } catch (const mysqlx::Error &e) {
            last_error_ = -1;
            last_errmsg_ = e.what();
            return -1;
        }
    }

    SqlData::Ptr MySQLStatement::query() {
        try {
            auto sql = session_->sql(sql_);
            if (!bound_params_.empty()) {
                sql.bind(bound_params_.begin(), bound_params_.end());
            }
            auto res = sql.execute();
            last_error_ = 0;
            last_errmsg_.clear();
            return std::make_shared<MySQLResult>(std::move(res)); // 假设你有 MySQLRes 实现 SqlData
        } catch (const mysqlx::Error &e) {
            last_error_ = -1;
            last_errmsg_ = e.what();
            return nullptr;
        }
    }

    int64_t MySQLStatement::last_insert_id() const {
        try {
            auto res = session_->sql("SELECT LAST_INSERT_ID()").execute();
            if (res.count() > 0) {
                auto row = res.fetchOne();
                auto id = row[0].get<int64_t>();
//                spdlog::info("[MySQLStatement] last_insert_id: {}", id);
                return id;
            }
        } catch (const mysqlx::Error &e) {
            spdlog::error("[MySQLStatement] last_insert_id error {}", e.what());
            return 0;
        }
        return 0;
    }


// ========== 错误 ==========
    int MySQLStatement::error_code() const {
        return last_error_;
    }

    std::string_view MySQLStatement::error_message() const {
        return last_errmsg_;
    }

    MySQLManager::MySQLManager(){
        spdlog::info("[MySQLManager] ctor: {}", static_cast<const void*>(this));
    }

    MySQLManager::~MySQLManager() {
        std::lock_guard lock(mutex_);
        // 清空连接池（智能指针析构即可）
        conns_.clear();
        db_defines_.clear();

        spdlog::info("[MySQLManager] mysql manager destroyed: {}", db_defines_.size());
    }

    void MySQLManager::register_mysql(const std::string &name,
                                      const std::unordered_map<std::string, std::string> &params) {
        std::lock_guard lock(mutex_);
        db_defines_[name] = params;
        // 如果还没有对应的池表项，创建一个空列表占位
        if (conns_.find(name) == conns_.end()) {
            conns_.emplace(name, std::list<Database::Ptr>{});
        }

        spdlog::info("[MySQLManager] register mysql: {} {}", name, db_defines_.size());
    }

    void MySQLManager::register_mysql(const std::string &name) {
        std::lock_guard lock(mutex_);
        db_defines_[name] = Config::db_params_;
        // 如果还没有对应的池表项，创建一个空列表占位
        if (conns_.find(name) == conns_.end()) {
            conns_.emplace(name, std::list<Database::Ptr>{});
        }

        spdlog::info("[MySQLManager] register mysql: {} {}", name, db_defines_.size());
    }

    Database::Ptr MySQLManager::get(const std::string &name) {
        std::unique_lock lock(mutex_);
        // 如果没有注册参数，返回 nullptr
        auto def_it = db_defines_.find(name);
        if (def_it == db_defines_.end()) {
            spdlog::error("[MySQLManager] mysql not registered: {} {}", name, db_defines_.size());
            return nullptr;
        }

        // 先尝试从池中取一个可用连接
        auto &pool = conns_[name];
        while (!pool.empty()) {
            auto dbptr = pool.front();
            pool.pop_front();
            lock.unlock(); // 调用时释放锁（避免长时间持锁）
            // 尝试检查连接是否可用（如果是 MySQL 派生则调用 ping）
            if (auto mysql_ptr = std::dynamic_pointer_cast<MySQL>(dbptr)) {
                try {
                    if (mysql_ptr->ping()) {
                        return dbptr;
                    }
                    // ping 失败，丢弃并继续取下一个
                } catch (...) {
                    // 出现异常也丢弃继续
                }
            } else {
                // 不是 MySQL（但继承于 Database），直接返回
                return dbptr;
            }
            lock.lock();
        }

        // 池里没有可用连接：创建新连接（受 max_conn_ 限制）
        // 计算当前池中该 name 的总连接数（在其他线程可能改变，但这是粗略限制）
        size_t current_count = conns_[name].size();
        if (current_count >= max_conn_) {
            // 超出限制，不创建新连接
            return nullptr;
        }

        // 创建新的 MySQL 实例
        auto params = def_it->second;
        lock.unlock(); // 释放锁，创建/连接操作不应在锁内执行
        try {
            auto mysql = std::make_shared<MySQL>(params);
            if (!mysql->connect()) {
                return nullptr;
            }
            return std::static_pointer_cast<Database>(mysql);
        } catch (...) {
            return nullptr;
        }
    }

    void MySQLManager::release_mysql(const std::string &name, Database *db) {
        if (db == nullptr) return;
        std::lock_guard lock(mutex_);
        // 如果没有注册该 name，则直接丢弃
        if (db_defines_.find(name) == db_defines_.end()) {
            return;
        }

        // 将 raw pointer 包回 shared_ptr 需要小心：我们应当接收 Database::Ptr 而非 raw pointer。
        // 但头文件要求 release_mysql(const std::string&, Database*).
        // 这里我们假定调用处持有 shared_ptr 并将其归还（见下面 query/execute 实现）。
        // 因此，这个函数会什么也不做（或记录），由调用者负责传入 shared_ptr 归还池。
        // 为安全起见，我们保留一个空实现，且提供一个 overload（如果需要）可接受 Database::Ptr。
        (void) name;
        (void) db;
        // no-op
    }

// overload: 方便归还 shared_ptr（实际会被用到）
    void push_back_conn(std::unordered_map<std::string, std::list<Database::Ptr>> &conns_map,
                        const std::string &name,
                        Database::Ptr dbptr,
                        uint32_t max_conn) {
        auto &lst = conns_map[name];
        if (lst.size() >= max_conn) {
            // 超出池容量，丢弃（让智能指针析构）
            return;
        }
        lst.push_back(std::move(dbptr));
    }

    int MySQLManager::execute(const std::string &name, std::string_view sql) {
        // 获取连接
        Database::Ptr db = get(name);
        if (!db) {
            return -1;
        }
        int ret = -1;
        try {
            // 用 dynamic_cast 调用 MySQL 的 execute（Database 可能也定义了 execute）
            if (auto mysql = std::dynamic_pointer_cast<MySQL>(db)) {
                ret = mysql->execute(sql);
            } else {
                // fallback to Database interface
                ret = db->execute(sql);
            }
        } catch (...) {
            ret = -1;
        }

        // 归还连接到池
        {
            std::lock_guard lock(mutex_);
            push_back_conn(conns_, name, db, max_conn_);
        }
        return ret;
    }

    SqlData::Ptr MySQLManager::query(const std::string &name, std::string_view sql) {
        Database::Ptr db = get(name);
        if (!db) {
            return nullptr;
        }

        SqlData::Ptr result;
        try {
            if (auto mysql = std::dynamic_pointer_cast<MySQL>(db)) {
                result = mysql->query(sql);
            } else {
                result = db->query(sql);
            }
        } catch (...) {
            result = nullptr;
        }

        // 归还连接到池
        {
            std::lock_guard lock(mutex_);
            push_back_conn(conns_, name, db, max_conn_);
        }
        return result;
    }

    Transaction::Ptr MySQLManager::open_transaction(const std::string &name, bool auto_commit) {
        Database::Ptr db = get(name);
        if (!db) {
            return nullptr;
        }

        Transaction::Ptr trx;
        try {
            if (auto mysql = std::dynamic_pointer_cast<MySQL>(db)) {
                trx = mysql->open_transaction(auto_commit);
            } else {
                trx = db->open_transaction(auto_commit);
            }
        } catch (...) {
            trx = nullptr;
        }

        // 归还连接到池 — 事务对象内部应该持有会话，所以即便归还，事务仍然有效
        {
            std::lock_guard lock(mutex_);
            push_back_conn(conns_, name, db, max_conn_);
        }
        return trx;
    }

    void MySQLManager::check_connection(int /*sec*/) {
        std::lock_guard lock(mutex_);
        for (auto &kv: conns_) {
            auto &name = kv.first;
            auto &lst = kv.second;
            for (auto it = lst.begin(); it != lst.end();) {
                bool alive = true;
                if (auto mysql = std::dynamic_pointer_cast<MySQL>(*it)) {
                    try {
                        alive = mysql->ping();
                    } catch (...) {
                        alive = false;
                    }
                }
                if (!alive) {
                    it = lst.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    MySQLUtil::DataPtr MySQLUtil::query(std::string_view name, std::string_view sql) {
        auto db = MySQLMgr::instance()->get(std::string{name});
//        auto db = MySQLManager::instance().get(std::string{name});
        if (!db) {
            throw std::runtime_error(std::format("MySQLUtil::query - no datasource [{}]", name));
        }
        return db->query(sql);
    }

    MySQLUtil::DataPtr MySQLUtil::try_query(std::string_view name, uint32_t count, std::string_view sql) {
        for (uint32_t i = 0; i < count; ++i) {
            try {
                auto db = MySQLMgr::instance()->get(std::string{name});
//                auto db = MySQLManager::instance().get(std::string{name});
                if (!db) {
                    throw std::runtime_error(std::format("MySQLUtil::try_query - no datasource [{}]", name));
                }
                auto res = db->query(sql);
                if (res) {
                    return res;
                }
            } catch (const std::exception &ex) {
                if (i + 1 == count) {
                    throw; // 最后一次失败直接抛出
                }
                // 小延迟再试，可以根据需要调整
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        return nullptr;
    }

    int MySQLUtil::execute(std::string_view name, std::string_view sql) {
        auto db = MySQLMgr::instance()->get(std::string{name});
//        auto db = MySQLManager::instance().get(std::string{name});
        if (!db) {
            throw std::runtime_error(std::format("MySQLUtil::execute - no datasource [{}]", name));
        }
        return db->execute(sql);
    }

    int MySQLUtil::try_execute(std::string_view name, uint32_t count, std::string_view sql) {
        for (uint32_t i = 0; i < count; ++i) {
            try {
                auto db = MySQLMgr::instance()->get(std::string{name});
//                auto db = MySQLManager::instance().get(std::string{name});
                if (!db) {
                    throw std::runtime_error(std::format("MySQLUtil::try_execute - no datasource [{}]", name));
                }
                auto rt = db->execute(sql);
                if (rt >= 0) {
                    return rt;
                }
            } catch (const std::exception &ex) {
                if (i + 1 == count) {
                    throw; // 最后一次失败抛出
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        return -1;
    }
}