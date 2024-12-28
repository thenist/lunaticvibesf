#pragma once

#include <any>
#include <functional>
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
typedef long long int sqlite_int64;
typedef unsigned long long int sqlite_uint64;
#endif
typedef sqlite_int64 sqlite3_int64;
typedef sqlite_uint64 sqlite3_uint64;

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

#define SQLITE_OK 0

class SQLite
{
private:
    mutable sqlite3* _db = nullptr;
    std::string tag;
    bool inTransaction = false;

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

private:
    void commitOrRollback(const std::string_view sql);

public:
    // TODO: RAII.
    void transactionStart();
    void commit();
    void rollback();

    void optimize();
    [[nodiscard]] const char* errmsg() const;
};
