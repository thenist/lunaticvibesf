#include <game/graphics/graph_line.h>

#include <game/graphics/graphics.h>

#include <SDL2_gfxPrimitives.h>

void lunaticvibes::GraphLineDrawer::draw(Point p1, Point p2, Color c) const
{
    const int ss = lunaticvibes::window::graphics_get_supersample_level();
    thickLineRGBA(gFrameRenderer, static_cast<Sint16>(p1.x) * ss, static_cast<Sint16>(p1.y) * ss,
                  static_cast<Sint16>(p2.x) * ss, static_cast<Sint16>(p2.y) * ss, _width * ss, c.r, c.g, c.b, c.a);
    SDL_SetRenderDrawColor(gFrameRenderer, 0, 0, 0, 255);
}
