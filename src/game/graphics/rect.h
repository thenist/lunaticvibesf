#pragma once

namespace lunaticvibes
{

struct Rect
{
    int x;
    int y;
    int w;
    int h;

    constexpr Rect(int zero = 0) : x(0), y(0), w(0), h(0) {}
    constexpr Rect(int w1, int h1) : x(0), y(0), w(w1), h(h1) {}
    constexpr Rect(int x1, int y1, int w1, int h1) : x(x1), y(y1), w(w1), h(h1) {}

    static constexpr int FULL_WIDTH = std::numeric_limits<int>::max();
    static constexpr int FULL_HEIGHT = std::numeric_limits<int>::max();

    constexpr Rect operator+(const Rect& rhs) const
    {
        Rect r = *this;
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

    constexpr Rect operator*(const double& rhs) const
    {
        Rect r = *this;
        r.x *= static_cast<int>(rhs);
        r.y *= static_cast<int>(rhs);
        if (r.w != FULL_WIDTH)
            r.w *= rhs;
        if (r.h != FULL_HEIGHT)
            r.h *= rhs;
        return r;
    }

    constexpr bool operator==(const Rect& rhs) const { return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h; }
    constexpr bool operator!=(const Rect& rhs) const { return !(*this == rhs); }
};

inline static constexpr Rect RECT_FULL = Rect(0, 0, Rect::FULL_WIDTH, Rect::FULL_HEIGHT);

} // namespace lunaticvibes

using lunaticvibes::Rect;
using lunaticvibes::RECT_FULL;
