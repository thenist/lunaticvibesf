#pragma once

#include <limits>

namespace lunaticvibes
{

struct RectF
{
    float x;
    float y;
    float w;
    float h;

    constexpr RectF(int zero = 0) { x = y = w = h = 0.; };
    constexpr RectF(float w, float h) : x(0), y(0), w(w), h(h) {};
    constexpr RectF(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {};

    static constexpr float FULL_WIDTH = std::numeric_limits<float>::max();
    static constexpr float FULL_HEIGHT = std::numeric_limits<float>::max();

    constexpr RectF operator+(const RectF& rhs) const
    {
        RectF r = *this;
        r.x += rhs.x;
        r.y += rhs.y;

        if (r.w == FULL_WIDTH || rhs.w == FULL_WIDTH)
            r.w = FULL_WIDTH;
        else
            r.w += rhs.w;

        if (r.h == FULL_HEIGHT || rhs.h == FULL_HEIGHT)
            r.h = FULL_HEIGHT;
        else
            r.h += rhs.h;

        return r;
    }

    constexpr RectF operator*(const double& rhs) const
    {
        RectF r = *this;
        r.x *= static_cast<int>(rhs);
        r.y *= static_cast<int>(rhs);
        if (r.w != FULL_WIDTH)
            r.w *= rhs;
        if (r.h != FULL_HEIGHT)
            r.h *= rhs;
        return r;
    }

    constexpr bool operator==(const RectF& rhs) const { return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h; }
    constexpr bool operator!=(const RectF& rhs) const { return !(*this == rhs); }
};

inline static constexpr RectF RECTF_FULL = RectF(0, 0, RectF::FULL_WIDTH, RectF::FULL_HEIGHT);

} // namespace lunaticvibes

using lunaticvibes::RectF;
using lunaticvibes::RECTF_FULL;
