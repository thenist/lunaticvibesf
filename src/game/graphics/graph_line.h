#pragma once

#include <game/graphics/color.h>
#include <game/graphics/point.h>

namespace lunaticvibes
{

// Thick line wrapper
struct GraphLineDrawer
{
    int _width = 1;

    GraphLineDrawer() = default;
    explicit GraphLineDrawer(int width) : _width(width) {}

    void draw(Point p1, Point p2, Color c) const;
};

} // namespace lunaticvibes
