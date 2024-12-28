#pragma once

#include <ctime>
#include <functional>
#include <future>
#include <string>

#include <stdint.h>

#include "common/types.h"

void SetThreadAsMainThread();
int64_t GetCurrentThreadID();
bool IsMainThread();
void SetThreadName(const char* name);
[[noreturn]] void panic(const char* title, const char* msg);
Path GetExecutablePath();

void setWindowHandle(void* handle);
void getWindowHandle(void* handle);

bool IsWindowForeground();
void SetWindowForeground(bool foreground);

void pushMainThreadTask(std::function<void()> f);
void doMainThreadTask();
void StopHandleMainThreadTask();
bool CanHandleMainThreadTask();

template <typename T> inline T pushAndWaitMainThreadTask(std::function<T()> f)
{
    if (CanHandleMainThreadTask())
    {
        std::promise<T> taskPromise;
        pushMainThreadTask([&]() { taskPromise.set_value(f()); });
        std::future<T> taskFuture = taskPromise.get_future();
        taskFuture.wait();
        return taskFuture.get();
    }
    else
    {
        return f();
    }
}
template <> inline void pushAndWaitMainThreadTask(std::function<void()> f)
{
    if (CanHandleMainThreadTask())
    {
        std::promise<void> taskPromise;
        pushMainThreadTask([&]() {
            f();
            taskPromise.set_value();
        });
        std::future<void> taskFuture = taskPromise.get_future();
        taskFuture.wait();
        return taskFuture.get();
    }
}
template <typename T, typename... Arg> inline T pushAndWaitMainThreadTask(std::function<T(Arg...)> f, Arg... arg)
{
    if (CanHandleMainThreadTask())
        return pushAndWaitMainThreadTask<T>(std::bind_front(f, arg...));
    return T();
}

// Unix epoch time.
long long getFileTimeNow();
// Unix epoch time.
long long getFileLastWriteTime(const Path& p);

enum class Languages
{
    EN,
    JP,
    KR,
    ZHCN,
    ZHTW,
    ZHHK,
    // tbd
};
Path getSysFontPath(std::string* faceName = nullptr, int* faceIndex = nullptr, Languages lang = Languages::EN);
Path getSysMonoFontPath(std::string* faceName = nullptr, int* faceIndex = nullptr, Languages lang = Languages::EN);

// Thread-safe strerror().
//
// This uses whatever safe implementation of strerror() target system
// has.
const char* safe_strerror(int errnum, char* buffer, size_t buffer_length);
// Wrapper over the other safe_strerror() that returns an owned string.
std::string safe_strerror(int errnum);

namespace lunaticvibes
{

// Open link, file or a folder.
bool open(const std::string& link);
// buf MUST be at least 26 bytes.
const char* safe_ctime(const std::time_t* timep, char* buf);
const tm* safe_gmtime(const std::time_t* timep, tm* result);
const tm* safe_localtime(const std::time_t* timep, tm* result);
time_t localtime_utc_offset();

} // namespace lunaticvibes
