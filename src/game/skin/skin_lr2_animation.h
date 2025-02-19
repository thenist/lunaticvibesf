#pragma once

#include <common/assert.h>
#include <game/graphics/sprite.h>

namespace lunaticvibes
{

// Formula matches LR2.
// Keyword: gradient
[[nodiscard]] inline constexpr double grad(int dst, int src, double t)
{
    return (src == dst) ? src : (dst * t + src * (1.0 - t));
}

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

} // namespace lunaticvibes
