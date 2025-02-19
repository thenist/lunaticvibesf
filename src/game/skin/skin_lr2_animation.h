#pragma once

#include <common/assert.h>
#include <game/graphics/sprite.h>

namespace lunaticvibes
{

[[nodiscard]] inline constexpr double calc_animation_multiplier(double start, double end, double now,
                                                                MotionKeyFrameParams::accelType type)
{
    const double tmp = (now - start) / (end - start);
    switch (type)
    {
    case MotionKeyFrameParams::CONSTANT: return tmp;
    case MotionKeyFrameParams::ACCEL: return tmp * tmp * tmp;
    case MotionKeyFrameParams::DECEL: return 1.0 - ((1.0 - tmp) * (1.0 - tmp) * (1.0 - tmp));
    case MotionKeyFrameParams::DISCONTINUOUS: return 0.0;
    }
    lunaticvibes::assert_failed("calc_animation_multiplier");
};

[[nodiscard]] inline constexpr double animate(double from, double to, double start, double end, double now,
                                              double multiplier)
{
    if (from == to)
        return from;
    if (end >= now && start <= now && end > start)
        return from * (1.0 - multiplier) + to * multiplier;
    if (start < now)
        return to;
    return from;
}

} // namespace lunaticvibes
