#include "generic_info.h"

#include <common/assert.h>
#include <common/sysutil.h>
#include <game/runtime/state.h>

#include <atomic>
#include <ctime>

static constexpr auto NS_IN_MS = 1'000'000LL;
static std::atomic<unsigned long long> s_frametimes[2]{0};

static int fps_from_frame_time(unsigned long long frame_time_ns)
{
    if (frame_time_ns == 0)
        return 0;
    return static_cast<int>(NS_IN_MS * 1000 / frame_time_ns);
}

void lunaticvibes::g_feed_frametime(FrameTimeType type, const lunaticvibes::Time& t)
{
    if (t.hres() <= 0)
        return;

    auto idx = static_cast<int>(type);
    constexpr auto smoothing = 0.9f;
    const auto current = s_frametimes[idx].load(std::memory_order_relaxed);
    s_frametimes[idx].store(static_cast<unsigned long long>((current * smoothing) + (t.hres() * (1.f - smoothing))),
                            std::memory_order_relaxed);
}

void lunaticvibes::g_update_generic_info(const lunaticvibes::Time& t)
{
    State::set(IndexNumber::FPS,
               fps_from_frame_time(s_frametimes[static_cast<int>(FrameTimeType::Fps)].load(std::memory_order_relaxed)));
    State::set(IndexNumber::INPUT_DETECT_FPS,
               fps_from_frame_time(s_frametimes[static_cast<int>(FrameTimeType::Input)].load(std::memory_order_relaxed)));

    constexpr auto millisec_in_sec = 1000;
    const std::time_t time_t_t = t.norm() / millisec_in_sec;
    tm d_;
    auto d = lunaticvibes::safe_localtime(&time_t_t, &d_);
    if (d)
    {
        State::set(IndexNumber::DATE_YEAR, d->tm_year + 1900);
        State::set(IndexNumber::DATE_MON, d->tm_mon + 1);
        State::set(IndexNumber::DATE_DAY, d->tm_mday);
        State::set(IndexNumber::DATE_HOUR, d->tm_hour);
        State::set(IndexNumber::DATE_MIN, d->tm_min);
        State::set(IndexNumber::DATE_SEC, d->tm_sec);
    }
}
