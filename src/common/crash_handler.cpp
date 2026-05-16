#include "crash_handler.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cwchar>
#include <exception>
#include <iterator>
#include <mutex>
#include <string>
#include <string_view>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <DbgHelp.h>
#include <Windows.h>
#include <csignal>
#include <cstdio>
#include <crtdbg.h>
#endif

namespace lunaticvibes
{

#ifdef _WIN32

namespace
{

constexpr wchar_t CRASH_DIR[] = L"crashdumps";
constexpr wchar_t CRASH_LOG[] = L"crashdumps\\crash.log";

std::once_flag install_once;
std::atomic_flag handling_crash = ATOMIC_FLAG_INIT;
std::atomic_bool wrote_exception_dump = false;

void ensure_crash_dir()
{
    CreateDirectoryW(CRASH_DIR, nullptr);
}

void append_crash_log(std::string_view message)
{
    ensure_crash_dir();

    SYSTEMTIME st{};
    GetLocalTime(&st);

    char line[4096]{};
    const int len = std::snprintf(line, sizeof(line), "%04u-%02u-%02u %02u:%02u:%02u.%03u [%lu] %.*s\r\n",
                                  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                                  GetCurrentThreadId(), static_cast<int>(message.size()), message.data());
    if (len <= 0)
        return;

    const DWORD bytes_to_write = static_cast<DWORD>(std::min<int>(len, sizeof(line) - 1));
    HANDLE file = CreateFileW(CRASH_LOG, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;

    DWORD bytes_written = 0;
    WriteFile(file, line, bytes_to_write, &bytes_written, nullptr);
    CloseHandle(file);
}

const char* exception_name(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION: return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
    default: return "UNKNOWN_EXCEPTION";
    }
}

bool is_fatal_exception(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_DATATYPE_MISALIGNMENT:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_STACK_OVERFLOW: return true;
    default: return false;
    }
}

void make_dump_path(wchar_t* path, size_t path_size)
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::swprintf(path, path_size, L"%s\\lvf_%04u%02u%02u_%02u%02u%02u_%03u_pid%lu_tid%lu.dmp", CRASH_DIR,
                  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                  GetCurrentProcessId(), GetCurrentThreadId());
}

void write_minidump(EXCEPTION_POINTERS* exception_pointers)
{
    ensure_crash_dir();

    wchar_t path[MAX_PATH]{};
    make_dump_path(path, std::size(path));

    HANDLE file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        append_crash_log("minidump: failed to create dump file");
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION exception_info{};
    exception_info.ThreadId = GetCurrentThreadId();
    exception_info.ExceptionPointers = exception_pointers;
    exception_info.ClientPointers = FALSE;

    const auto dump_type = static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithDataSegs |
                                                      MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo |
                                                      MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData);
    const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, dump_type,
                                      exception_pointers ? &exception_info : nullptr, nullptr, nullptr);
    CloseHandle(file);

    char message[512]{};
    if (ok)
    {
        std::snprintf(message, sizeof(message), "minidump: wrote %ls", path);
    }
    else
    {
        std::snprintf(message, sizeof(message), "minidump: failed for %ls, error=%lu", path, GetLastError());
    }
    append_crash_log(message);
}

void log_exception_record(const char* source, EXCEPTION_POINTERS* exception_pointers)
{
    const auto* record = exception_pointers ? exception_pointers->ExceptionRecord : nullptr;
    const DWORD code = record ? record->ExceptionCode : 0;
    const void* address = record ? record->ExceptionAddress : nullptr;

    char message[512]{};
    std::snprintf(message, sizeof(message), "%s: code=0x%08lx (%s), address=%p", source, code, exception_name(code),
                  address);
    append_crash_log(message);

    if (record && code == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
    {
        std::snprintf(message, sizeof(message), "%s: access violation op=%llu target=0x%llx", source,
                      static_cast<unsigned long long>(record->ExceptionInformation[0]),
                      static_cast<unsigned long long>(record->ExceptionInformation[1]));
        append_crash_log(message);
    }
}

void write_exception_dump_once(const char* source, EXCEPTION_POINTERS* exception_pointers)
{
    if (wrote_exception_dump.exchange(true))
    {
        char message[128]{};
        std::snprintf(message, sizeof(message), "%s: exception dump already written", source);
        append_crash_log(message);
        return;
    }

    log_exception_record(source, exception_pointers);
    write_minidump(exception_pointers);
}

LONG CALLBACK vectored_exception_handler(EXCEPTION_POINTERS* exception_pointers)
{
    const auto* record = exception_pointers ? exception_pointers->ExceptionRecord : nullptr;
    if (record && is_fatal_exception(record->ExceptionCode))
        write_exception_dump_once("vectored exception", exception_pointers);

    return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* exception_pointers)
{
    if (!handling_crash.test_and_set())
    {
        write_exception_dump_once("unhandled exception", exception_pointers);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

void terminate_handler()
{
    if (!handling_crash.test_and_set())
    {
        append_crash_log("std::terminate called");
        if (auto exception = std::current_exception())
        {
            try
            {
                std::rethrow_exception(exception);
            }
            catch (const std::exception& e)
            {
                append_crash_log(std::string{"std::terminate exception: "} + e.what());
            }
            catch (...)
            {
                append_crash_log("std::terminate exception: non-standard exception");
            }
        }
        write_minidump(nullptr);
    }
    TerminateProcess(GetCurrentProcess(), 3);
}

void invalid_parameter_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
    if (!handling_crash.test_and_set())
    {
        append_crash_log("invalid parameter handler called");
        write_minidump(nullptr);
    }
    TerminateProcess(GetCurrentProcess(), 4);
}

void purecall_handler()
{
    if (!handling_crash.test_and_set())
    {
        append_crash_log("pure virtual function call handler called");
        write_minidump(nullptr);
    }
    TerminateProcess(GetCurrentProcess(), 5);
}

void signal_handler(int signal)
{
    if (!handling_crash.test_and_set())
    {
        char message[128]{};
        std::snprintf(message, sizeof(message), "signal handler called: signal=%d", signal);
        append_crash_log(message);
        if (wrote_exception_dump.load())
            append_crash_log("signal handler: exception dump already written");
        else
            write_minidump(nullptr);
    }
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(128 + signal));
}

} // namespace

void InstallCrashHandler()
{
    std::call_once(install_once, [] {
        ensure_crash_dir();
        append_crash_log("crash handler installed");
        AddVectoredExceptionHandler(1, vectored_exception_handler);
        SetUnhandledExceptionFilter(unhandled_exception_filter);
        std::set_terminate(terminate_handler);
        _set_invalid_parameter_handler(invalid_parameter_handler);
        _set_purecall_handler(purecall_handler);
        std::signal(SIGABRT, signal_handler);
        std::signal(SIGFPE, signal_handler);
        std::signal(SIGILL, signal_handler);
        std::signal(SIGSEGV, signal_handler);
        std::signal(SIGTERM, signal_handler);
    });
}

void CrashBreadcrumb(std::string_view message)
{
    append_crash_log(std::string{"breadcrumb: "} + std::string{message});
}

#else

void InstallCrashHandler() {}

void CrashBreadcrumb(std::string_view) {}

#endif

} // namespace lunaticvibes
