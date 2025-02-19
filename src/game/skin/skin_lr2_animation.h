#pragma once

namespace lunaticvibes
{

// Formula matches LR2.
// Keyword: gradient
[[nodiscard]] inline constexpr double grad(int dst, int src, double t)
{
    return (src == dst) ? src : (dst * t + src * (1.0 - t));
}

} // namespace lunaticvibes
