#pragma once

#include <algorithm>
#include <cstdint>

using uint8_t = std::uint8_t;
using uint32_t = std::uint32_t;

namespace lunaticvibes
{

struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

public:
    constexpr Color(uint32_t rgba = 0xffffffff)
        : r((rgba & 0xff000000) >> 24), g((rgba & 0x00ff0000) >> 16), b((rgba & 0x0000ff00) >> 8),
          a((rgba & 0x000000ff))
    {
    }

    constexpr Color(int r, int g, int b, int a)
        : r(std::clamp(r, 0, 255)), g(std::clamp(g, 0, 255)), b(std::clamp(b, 0, 255)), a(std::clamp(a, 0, 255))
    {
    }

    [[nodiscard]] constexpr uint32_t hex() const { return r << 24 | g << 16 | b << 8 | a; }

    constexpr Color operator+(const Color& rhs) const
    {
        Color c;
        c.r = (r + rhs.r <= 255) ? (r + rhs.r) : 255;
        c.g = (g + rhs.g <= 255) ? (g + rhs.g) : 255;
        c.b = (b + rhs.b <= 255) ? (b + rhs.b) : 255;
        c.a = (a + rhs.a <= 255) ? (a + rhs.a) : 255;
        return c;
    }

    constexpr Color operator*(const double& rhs) const
    {
        if (rhs < 0)
            return {0};
        Color c;
        c.r = (r * rhs <= 255) ? static_cast<uint8_t>(r * rhs) : 255;
        c.g = (g * rhs <= 255) ? static_cast<uint8_t>(g * rhs) : 255;
        c.b = (b * rhs <= 255) ? static_cast<uint8_t>(b * rhs) : 255;
        c.a = (a * rhs <= 255) ? static_cast<uint8_t>(a * rhs) : 255;
        return c;
    }

    constexpr Color operator*(const Color& rhs) const
    {
        if (hex() == 0xffffffff)
            return rhs;

        Color c;
        c.r = static_cast<uint8_t>(r * (rhs.r / 255.0));
        c.g = static_cast<uint8_t>(g * (rhs.g / 255.0));
        c.b = static_cast<uint8_t>(b * (rhs.b / 255.0));
        c.a = static_cast<uint8_t>(a * (rhs.a / 255.0));
        return c;
    }

    constexpr bool operator==(const Color& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; }
    constexpr bool operator!=(const Color& rhs) const { return !(*this == rhs); }
};

} // namespace lunaticvibes

using lunaticvibes::Color;
