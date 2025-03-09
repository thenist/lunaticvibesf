#include "sysutil.h"
#include "common/types.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <functional>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <utility>

#include <common/assert.h>
#include <common/in_test_mode.h>
#include <common/meta.h>

#include <tinyfiledialogs.h>

static std::shared_mutex s_main_thread_task_mutex;
static std::queue<std::move_only_function<void()>> s_main_thread_task_queue;
static bool s_main_thread_task_do_handle = true;

static std::atomic<bool> s_foreground = true;
bool IsWindowForeground()
{
    return s_foreground;
}
void SetWindowForeground(bool f)
{
    s_foreground = f;
}

void lunaticvibes::details::doPushMainThreadTask(std::move_only_function<void()> f)
{
    LVF_DEBUG_ASSERT(!IsMainThread());
    if (s_main_thread_task_do_handle)
    {
        std::unique_lock l(s_main_thread_task_mutex);
        s_main_thread_task_queue.emplace(std::move(f));
    }
}

void doMainThreadTask()
{
    std::shared_lock l(s_main_thread_task_mutex);
    while (!s_main_thread_task_queue.empty())
    {
        s_main_thread_task_queue.front()();
        s_main_thread_task_queue.pop();
    }
}

void StopHandleMainThreadTask()
{
    s_main_thread_task_do_handle = false;
}

bool CanHandleMainThreadTask()
{
    return s_main_thread_task_do_handle;
}

static thread_local bool s_is_main_thread = false;
void SetThreadAsMainThread()
{
    s_is_main_thread = true;
}
bool IsMainThread()
{
    return s_is_main_thread;
}

long long getFileTimeNow()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

Path getSysFontPath(std::string* faceName, int* faceIndex, Languages lang)
{
    Path p = Path(GAMEDATA_PATH) / "resources" / "fonts" / "NotoSansCJK-Regular.ttc";
    switch (lang)
    {
    case Languages::KR:
        if (faceName)
            *faceName = "Noto Sans CJK KR";
        if (faceIndex)
            *faceIndex = 1;
        break;
    case Languages::ZHCN:
        if (faceName)
            *faceName = "Noto Sans CJK SC";
        if (faceIndex)
            *faceIndex = 2;
        break;
    case Languages::ZHTW:
        if (faceName)
            *faceName = "Noto Sans CJK TC";
        if (faceIndex)
            *faceIndex = 3;
        break;
    case Languages::ZHHK:
        if (faceName)
            *faceName = "Noto Sans CJK HK";
        if (faceIndex)
            *faceIndex = 4;
        break;
    case Languages::EN:
    case Languages::JP:
    default:
        if (faceName)
            *faceName = "Noto Sans CJK JP";
        if (faceIndex)
            *faceIndex = 0;
        break;
    }
    return p;
}

Path getSysMonoFontPath(std::string* faceName, int* faceIndex, Languages lang)
{
    Path p = Path(GAMEDATA_PATH) / "resources" / "fonts" / "NotoSansCJK-Regular.ttc";
    switch (lang)
    {
    case Languages::KR:
        if (faceName)
            *faceName = "Noto Sans Mono CJK KR";
        if (faceIndex)
            *faceIndex = 6;
        break;
    case Languages::ZHCN:
        if (faceName)
            *faceName = "Noto Sans Mono CJK SC";
        if (faceIndex)
            *faceIndex = 7;
        break;
    case Languages::ZHTW:
        if (faceName)
            *faceName = "Noto Sans Mono CJK TC";
        if (faceIndex)
            *faceIndex = 8;
        break;
    case Languages::ZHHK:
        if (faceName)
            *faceName = "Noto Sans Mono CJK HK";
        if (faceIndex)
            *faceIndex = 9;
        break;
    case Languages::EN:
    case Languages::JP:
    default:
        if (faceName)
            *faceName = "Noto Sans Mono CJK JP";
        if (faceIndex)
            *faceIndex = 5;
        break;
    }
    return p;
}

std::string safe_strerror(int errnum)
{
    static constexpr size_t ERROR_DESCRIPTION_BUFFER_SIZE = 128;
    char error_description_buffer[ERROR_DESCRIPTION_BUFFER_SIZE] = {0};
    const char* error_description =
        safe_strerror(errnum, static_cast<char*>(error_description_buffer), ERROR_DESCRIPTION_BUFFER_SIZE);
    return {error_description};
}

time_t lunaticvibes::localtime_utc_offset()
{
    static const time_t seconds = 0;
    tm tmUtc{};
    tm tmLocal{};
    lunaticvibes::safe_gmtime(&seconds, &tmUtc);
    lunaticvibes::safe_localtime(&seconds, &tmLocal);
    return mktime(&tmUtc) - mktime(&tmLocal);
}

void panic(const char* title, const char* msg)
{
    // Avoid SIGABRT when running tests, Google Test doesn't catch it.
    if (lunaticvibes::in_test_mode())
        throw std::runtime_error{title + std::string{" "} + msg};
    (void)fprintf(stderr, "PANIC! [%s] %s\n", title, msg);
    tinyfd_messageBox(title, msg, "ok", "error", 0);
    abort();
}
