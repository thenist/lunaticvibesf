
#pragma once

namespace lunaticvibes
{

struct Point
{
    double x = 0;
    double y = 0;

    constexpr Point(int zero = 0) {}
    constexpr Point(double x, double y) : x(x), y(y) {}
    constexpr Point operator+(const Point& rhs) const { return {x + rhs.x, y + rhs.y}; }
    constexpr Point operator-(const Point& rhs) const { return {x - rhs.x, y - rhs.y}; }
    constexpr Point operator*(const double& rhs) const { return {x * rhs, y * rhs}; }
    constexpr bool operator==(const Point& rhs) const { return x == rhs.x && y == rhs.y; }
};

} // namespace lunaticvibes

using lunaticvibes::Point;
