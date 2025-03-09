#include <db/db_conn.h>

#include <common/assert.h>
#include <common/log.h>
#include <common/u8.h>

#include <sqlite3.h>

#include <any>
#include <cstddef>
#include <ctime>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void lunaticvibes::SqliteDeleter::operator()(sqlite3* db)
{
    sqlite3_close(db);
}

SQLite::SQLite(const char* path, std::string tag_, SQLite::OpenMode mode) : tag(std::move(tag_))
{
    LVF_ASSERT(sqlite3_threadsafe() == 1);

    int flags = 0;
    switch (mode)
    {
    case OpenMode::ReadWrite: flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE; break;
    case OpenMode::ReadOnly: flags = SQLITE_OPEN_READONLY; break;
    }
    sqlite3* db = nullptr;
    const int ret = sqlite3_open_v2(path, &db, flags, nullptr);
    if (ret != SQLITE_OK)
    {
        LOG_ERROR << "[sqlite3] sql failed to open db at path '" << path << "' (tag '" << tag << "'): [" << ret << "] "
                  << errmsg();
    }
    _db.reset(db);

    exec("PRAGMA temp_store = memory");
    exec("PRAGMA mmap_size = 536870912"); // 512MB
}

SQLite::~SQLite() = default;

const char* SQLite::errmsg() const
{
    return sqlite3_errmsg(_db.get());
}

#ifndef NDEBUG

[[nodiscard]] static std::string any_to_str(const std::any& a)
{
    std::stringstream ss;
    if (a.type() == typeid(int))
        ss << std::any_cast<int>(a);
    else if (a.type() == typeid(bool))
        ss << std::any_cast<bool>(a);
    else if (a.type() == typeid(long long))
        ss << std::any_cast<long long>(a);
    else if (a.type() == typeid(unsigned))
        ss << std::any_cast<unsigned>(a);
    else if (a.type() == typeid(time_t))
        ss << std::any_cast<time_t>(a);
    else if (a.type() == typeid(double))
        ss << std::any_cast<double>(a);
    else if (a.type() == typeid(std::string))
        ss << "'" << std::any_cast<std::string>(a) << "'";
    else if (a.type() == typeid(std::string_view))
        ss << "'" << std::any_cast<std::string_view>(a) << "'";
    else if (a.type() == typeid(std::u8string))
        ss << "'" << lunaticvibes::s(std::any_cast<std::u8string>(a)) << "'";
    else if (a.type() == typeid(std::u8string_view))
        ss << "'" << lunaticvibes::s(std::any_cast<std::u8string_view>(a)) << "'";
    else if (a.type() == typeid(const char*))
        ss << "'" << std::any_cast<const char*>(a) << "'";
    else if (a.type() == typeid(nullptr))
        ss << "NULL";
    else
        lunaticvibes::assert_failed("any_to_str");
    return ss.str();
}

#endif // NDEBUG

// TODO: use std::variant instead of std::any
static void sql_bind_any(sqlite3_stmt* stmt, int i, const std::any& a)
{
    int ret = SQLITE_OK;
    if (a.type() == typeid(int))
        ret = sqlite3_bind_int(stmt, i, std::any_cast<int>(a));
    else if (a.type() == typeid(bool))
        ret = sqlite3_bind_int(stmt, i, static_cast<int>(std::any_cast<bool>(a)));
    else if (a.type() == typeid(long long))
        ret = sqlite3_bind_int64(stmt, i, std::any_cast<long long>(a));
    else if (a.type() == typeid(unsigned))
        ret = sqlite3_bind_int64(stmt, i, std::any_cast<unsigned>(a));
    else if (a.type() == typeid(time_t))
        ret = sqlite3_bind_int64(stmt, i, std::any_cast<time_t>(a));
    else if (a.type() == typeid(double))
        ret = sqlite3_bind_double(stmt, i, std::any_cast<double>(a));
    else if (a.type() == typeid(std::string))
    {
        const auto str = std::any_cast<std::string>(a);
        ret = sqlite3_bind_text(stmt, i, str.c_str(), static_cast<int>(str.length()), SQLITE_TRANSIENT);
    }
    else if (a.type() == typeid(std::string_view))
    {
        const auto str = std::any_cast<std::string_view>(a);
        ret = sqlite3_bind_text(stmt, i, str.data(), static_cast<int>(str.length()), SQLITE_TRANSIENT);
    }
    else if (a.type() == typeid(std::u8string))
    {
        const auto str = std::any_cast<std::u8string>(a);
        ret = sqlite3_bind_text(stmt, i, reinterpret_cast<const char*>(str.data()), static_cast<int>(str.length()),
                                SQLITE_TRANSIENT);
    }
    else if (a.type() == typeid(std::u8string_view))
    {
        const auto str = std::any_cast<std::u8string_view>(a);
        ret = sqlite3_bind_text(stmt, i, reinterpret_cast<const char*>(str.data()), static_cast<int>(str.length()),
                                SQLITE_TRANSIENT);
    }
    else if (a.type() == typeid(const char*))
        ret = sqlite3_bind_text(stmt, i, std::any_cast<const char*>(a), -1, SQLITE_TRANSIENT);
    else if (a.type() == typeid(nullptr))
        ret = sqlite3_bind_null(stmt, i);
    else
        lunaticvibes::assert_failed("[sqlite3] Unsupported bind type");

    if (ret != SQLITE_OK)
    {
        LOG_ERROR << "[sqlite3] Bind error";
    }
}
static void sql_bind_any(sqlite3_stmt* stmt, const std::initializer_list<std::any>& args)
{
    int i = 1;
    for (auto& a : args)
    {
        sql_bind_any(stmt, i++, a);
    }
}

std::vector<std::vector<std::any>> SQLite::query(const std::string_view zsql,
                                                 std::initializer_list<std::any> args) const
{
    sqlite3_stmt* stmt = nullptr;
    int ret = sqlite3_prepare_v3(_db.get(), zsql.data(), static_cast<int>(zsql.size()), 0, &stmt, nullptr);
    if (ret != 0)
    {
        LOG_ERROR << "[sqlite3] sql \"" << zsql << "\" prepare error: [" << ret << "] " << errmsg();
        return {};
    }

    const int columnCount = sqlite3_column_count(stmt);
    if (columnCount == 0)
    {
        LOG_ERROR << "[sqlite3] Query returns 0 columns";
        return {};
    }

    sql_bind_any(stmt, args);

    std::vector<std::vector<std::any>> out;
    while (true)
    {
        ret = sqlite3_step(stmt);
        if (ret != SQLITE_ROW)
        {
            if (ret == SQLITE_DONE)
                break;
            LOG_ERROR << "[sqlite3] SQL query step failed: " << errmsg();
            break;
        }
        auto& row = out.emplace_back();
        row.resize(columnCount);
        for (int i = 0; i < columnCount; ++i)
        {
            const int c = sqlite3_column_type(stmt, i);
            switch (c)
            {
            case SQLITE_INTEGER: row[i] = sqlite3_column_int64(stmt, i); break;
            case SQLITE_FLOAT: row[i] = sqlite3_column_double(stmt, i); break;
            case SQLITE_TEXT:
                row[i] = std::make_any<std::string>(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)),
                                                    static_cast<size_t>(sqlite3_column_bytes(stmt, i)));
                break;
            case SQLITE_BLOB: LOG_ERROR << "[sqlite3] row[" << i << "]: fetched unsupported type SQLITE_BLOB"; break;
            case SQLITE_NULL: break; // assume !row[i].has_value()
            default: LOG_ERROR << "[sqlite3] row[" << i << "]: unknown column type c=" << c; break;
            }
        }
    }

#ifndef NDEBUG
    std::stringstream ss;
    ss << "[sqlite3] " << tag << ": " << " query " << zsql;
    ss << " (args: ";
    for (auto& a : args)
    {
        ss << any_to_str(a) << ", ";
    }
    ss << ") result: " << out.size() << " rows";
    LOG_VERBOSE << ss.str();
#endif

    sqlite3_finalize(stmt);
    return out;
}

int SQLite::exec(const std::string_view zsql, std::initializer_list<std::any> args)
{
    sqlite3_stmt* stmt = nullptr;
    int ret = sqlite3_prepare_v3(_db.get(), zsql.data(), static_cast<int>(zsql.size()), 0, &stmt, nullptr);
    if (ret != 0)
    {
        LOG_ERROR << "[sqlite3] sql \"" << zsql << "\" prepare error: [" << ret << "] " << errmsg();
        return ret;
    }

    sql_bind_any(stmt, args);

    ret = sqlite3_step(stmt);

    if (ret != SQLITE_OK && ret != SQLITE_ROW && ret != SQLITE_DONE)
    {
        LOG_ERROR << "[sqlite3] " << tag << ": " << " exec " << zsql << ": " << errmsg();
        sqlite3_finalize(stmt);
        return ret;
    }

#ifndef NDEBUG
    std::stringstream ss;
    ss << "[sqlite3] " << tag << ": " << " exec " << zsql;
    ss << " (args: ";
    for (auto& a : args)
    {
        ss << any_to_str(a) << ", ";
    }
    ss << ")";
    if (ret != SQLITE_OK && ret != SQLITE_ROW && ret != SQLITE_DONE)
        LOG_ERROR << ss.str() << ": " << errmsg();
    else
        LOG_VERBOSE << ss.str();
#endif

    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

SQLite::Transaction::Transaction(SQLite& db) : _db(db)
{
    LVF_ASSERT(!db._isinTransaction && "Transaction()");
    db._isinTransaction = true;
    db.exec("BEGIN");
}

SQLite::Transaction::~Transaction()
{
    if (!_finished)
        rollback();
    _db._isinTransaction = false;
}

void SQLite::Transaction::commit()
{
    LVF_ASSERT(!_finished && "commit");
    exec("COMMIT");
    _finished = true;
}

void SQLite::Transaction::rollback()
{
    LVF_ASSERT(!_finished && "rollback");
    exec("ROLLBACK");
    _finished = true;
}

void SQLite::Transaction::exec(const std::string_view sql)
{
    sqlite3_stmt* stmt = nullptr;
    int ret = sqlite3_prepare_v3(_db._db.get(), sql.data(), static_cast<int>(sql.size()), 0, &stmt, nullptr);
    LVF_ASSERT(ret == SQLITE_OK && "sqlite3_prepare_v3");
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_OK && ret != SQLITE_ROW && ret != SQLITE_DONE)
    {
        LOG_ERROR << "[sqlite3] " << _db.tag << ": Transaction '" << sql << "' failed: " << _db.errmsg();
    }
    else
    {
        LOG_DEBUG << "[sqlite3] " << _db.tag << ": Transaction '" << sql << "' success";
    }
    sqlite3_finalize(stmt);
}

void SQLite::optimize()
{
    LOG_DEBUG << "[sqlite3] " << tag << ": optimize ";
    exec("PRAGMA optimize(0xfffe)");
}

bool SQLite::applyMigration(std::string_view name, const std::function<bool()>& migrate)
{
    LOG_DEBUG << "[sqlite3] Applying migration '" << name << "'";
    LVF_DEBUG_ASSERT(name.length() == std::string_view{"YYYYMMDDTHHMMSS"}.length());

    int ret = exec("CREATE TABLE IF NOT EXISTS __lvf_schema_migrations (name TEXT, date TEXT);");
    if (ret != SQLITE_OK)
    {
        lunaticvibes::verify_failed(
            (std::string{"[sqlite3] Creation of __lvf_schema_migrations failed: "} + errmsg()).c_str());
        return false;
    }

    auto didAlreadyMigrate = query("SELECT EXISTS (SELECT 1 FROM __lvf_schema_migrations WHERE name=?);", {name});
    LVF_DEBUG_ASSERT(didAlreadyMigrate.size() == 1 && didAlreadyMigrate[0].size() == 1);
    if (ANY_INT(didAlreadyMigrate[0][0]) == 1)
    {
        LOG_VERBOSE << "[sqlite3] Migration has already been applied";
        return true;
    }

    Transaction transaction(*this);

    if (!migrate())
    {
        LOG_ERROR << "[sqlite3] Migration '" << name << "' failed";
        return false;
    }

    ret = exec("INSERT INTO __lvf_schema_migrations(name, date) VALUES (?, datetime());", {name});
    LVF_ASSERT(ret == SQLITE_OK && "INSERT INTO __lvf_schema_migrations");

    transaction.commit();
    LOG_VERBOSE << "[sqlite3] Migration has been successfully applied";
    return true;
}

bool SQLite::isReadOnly() const
{
    const int ret = sqlite3_db_readonly(_db.get(), nullptr);
    // TODO: remove this check when SQLite can't construct invalid object.
    if (ret == -1)
    {
        LOG_ERROR << "[sqlite3] isReadOnly() error: " << errmsg();
        return true;
    }
    LVF_VERIFY(ret == 0 || ret == 1);
    return ret == 1;
}
