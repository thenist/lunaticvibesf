#pragma once

#include <atomic>
#include <functional>

#include <common/types.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <future>
using LooperHandler = HANDLE;

#else

#include <thread>
using LooperHandler = std::thread;

#endif // _WIN32

// Poll update function in background thread.
// Should be OS-specific to provide reasonable performance.
class AsyncLooper
{
protected:
    StringContent _tag;
    unsigned _rate;
    std::atomic<bool> _running = false;
    bool _inLoopBody = false;

    LooperHandler handler;

#ifdef _WIN32
protected:
    std::future<void> loopFuture;
    long long tStart = 0;
#else
    void _loopWithSleep();
#endif

public:
    AsyncLooper() = delete;
    AsyncLooper(StringContentView tag, std::function<void()>, unsigned rate_per_sec, bool single_inst = false);
    virtual ~AsyncLooper();
    void setRate(unsigned rate_per_sec);
    void loopStart();
    void loopEnd();
    [[nodiscard]] bool isRunning() const { return _running; }
    [[nodiscard]] unsigned getRate() const { return _rate; };

private:
    std::function<void()> _loopFunc;
    void run();
};
