#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <common/types.h>
#include <common/u8.h>

struct sqlite3;

#if defined(_MSC_VER)
typedef __int64 sqlite_int64;
typedef unsigned __int64 sqlite_uint64;
#else
using sqlite_int64 = long long;
using sqlite_uint64 = unsigned long long;
#endif
using sqlite3_int64 = sqlite_int64;
using sqlite3_uint64 = sqlite_uint64;

inline long long ANY_INT(const std::any& a)
{
    return std::any_cast<sqlite3_int64>(a);
}
inline double ANY_REAL(const std::any& a)
{
    return std::any_cast<double>(a);
}
inline std::string ANY_STR(const std::any& a)
{
    return std::any_cast<std::string>(a);
}

namespace lunaticvibes
{

struct SqliteDeleter
{
    void operator()(sqlite3* conn);
};
using SqlitePtr = std::unique_ptr<sqlite3, SqliteDeleter>;

} // namespace lunaticvibes

#define SQLITE_OK 0

class SQLite
{
private:
    mutable lunaticvibes::SqlitePtr _db;
    std::string tag;
    bool _isinTransaction = false;

public:
    enum class OpenMode
    {
        ReadWrite,
        ReadOnly,
    };
    SQLite() = delete;
    SQLite(const char* path, std::string tag, OpenMode mode = OpenMode::ReadWrite);
    SQLite(const Path& path, std::string tag, OpenMode mode = OpenMode::ReadWrite)
        : SQLite(lunaticvibes::cs(path.u8string()), std::move(tag), mode)
    {
    }
    virtual ~SQLite();
    SQLite(const SQLite&) = delete;
    SQLite(SQLite&&) = delete;
    SQLite& operator=(const SQLite&) = delete;
    SQLite& operator=(SQLite&&) = delete;

protected:
    [[nodiscard]] std::vector<std::vector<std::any>> query(std::string_view stmt,
                                                           std::initializer_list<std::any> args = {}) const;
    int exec(std::string_view stmt, std::initializer_list<std::any> args = {});

    // Subsequent calls with same 'name' are no-op.
    // For 'name', use YYYYMMDDTHHMMSS format.
    // True on success.
    [[nodiscard]] bool applyMigration(std::string_view name, const std::function<bool()>& migrate);
    [[nodiscard]] bool isReadOnly() const;

public:
    class Transaction
    {
    public:
        Transaction(SQLite&);
        ~Transaction();

        void commit();
        void rollback();

        Transaction(const Transaction&) = delete;
        Transaction(Transaction&&) = delete;
        Transaction& operator=(const Transaction&) = delete;
        Transaction& operator=(Transaction&&) = delete;

    private:
        void exec(std::string_view sql);
        SQLite& _db;
        bool _finished = false;
    };

    void optimize();
    [[nodiscard]] const char* errmsg() const;
};
