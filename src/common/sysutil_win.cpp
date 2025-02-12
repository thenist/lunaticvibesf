#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include "sysutil.h"

#include <common/utils.h>

#include <VersionHelpers.h>
#include <shellapi.h>
#include <windows.h>

#include <chrono>
#include <cstdio>
#include <filesystem>

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;     // Must be 0x1000.
    LPCSTR szName;    // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void SetThreadNameWin32(DWORD dwThreadID, const char* threadName)
{
    // TODO: revert c2b4c595 Remove SetThreadDescription() call? #win

    // change VS debugger thread name
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322) // bare except and empty catch body
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#pragma warning(pop)
}

void SetThreadName(const char* name)
{
    SetThreadNameWin32(GetCurrentThreadId(), name);
}

Path GetExecutablePath()
{
    char fullpath[256] = {0};
    if (!GetModuleFileNameA(nullptr, fullpath, sizeof(fullpath)))
        return {};
    return fs::path(fullpath).parent_path();
}

static HWND hwnd = nullptr;
void setWindowHandle(void* handle)
{
    hwnd = *(HWND*)handle;
}
void getWindowHandle(void* handle)
{
    *(HWND*)handle = hwnd;
}

long long getFileLastWriteTime(const Path& p)
{
    constexpr auto TO_UNIX_EPOCH = 11644473600;
    return std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time(p).time_since_epoch()).count() -
           TO_UNIX_EPOCH;
}

const char* safe_strerror(int errnum, char* buffer, size_t buffer_length)
{
    strerror_s(buffer, buffer_length, errnum);
    return buffer;
}

bool lunaticvibes::open(const std::string& link)
{
    // TODO: check UTF-8 compatibility. #win
    auto res = reinterpret_cast<INT_PTR>(ShellExecute(nullptr, "open", link.c_str(), nullptr, nullptr, SW_SHOWDEFAULT));
    // > .. can be cast only to an INT_PTR
    // > If the function succeeds, it returns a value greater than 32.
    return res > 32;
}

const char* lunaticvibes::safe_ctime(const std::time_t* timep, char* buf)
{
    errno_t err = ctime_s(buf, 26, timep);
    if (err)
        return "";
    return buf;
};

const tm* lunaticvibes::safe_gmtime(const std::time_t* timep, tm* result)
{
    errno_t err = gmtime_s(result, timep);
    if (err)
        return nullptr;
    return result;
};

const tm* lunaticvibes::safe_localtime(const std::time_t* timep, tm* result)
{
    errno_t err = localtime_s(result, timep);
    if (err)
        return nullptr;
    return result;
};

#endif
