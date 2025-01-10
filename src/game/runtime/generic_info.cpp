#include "generic_info.h"

#include <common/sysutil.h>
#include <game/runtime/state.h>

#include <ctime>

void lunaticvibes::update_global_generic_info(const lunaticvibes::Time& t)
{
    constexpr unsigned rate = lunaticvibes::global_generic_info_update_rate;
    State::set(IndexNumber::FPS, gFrameCount[FRAMECOUNT_IDX_FPS] * rate);
    gFrameCount[FRAMECOUNT_IDX_FPS] = 0;
    State::set(IndexNumber::INPUT_DETECT_FPS, gFrameCount[FRAMECOUNT_IDX_INPUT] * rate);
    gFrameCount[FRAMECOUNT_IDX_INPUT] = 0;

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
