#include "sprite.h"

namespace lunaticvibes
{
bool isPanelOpen(int panelIdx);
} // namespace lunaticvibes

SpriteOnMouse::SpriteOnMouse(const SpriteOnMouseBuilder& builder) : SpriteAnimated(builder)
{
    _type = SpriteTypes::ONMOUSE;
    visibleOnPanel = builder.visibleOnPanel;
    mouseArea = builder.mouseArea;
}

bool SpriteOnMouse::update(const lunaticvibes::Time& t)
{
    if (!lunaticvibes::isPanelOpen(visibleOnPanel))
        return false;
    return SpriteAnimated::update(t);
}

void SpriteOnMouse::OnMouseMove(int x, int y)
{
    if (_draw)
    {
        int bx = _current.rect.x + mouseArea.x;
        int by = _current.rect.y + mouseArea.y;
        if (x < bx || x > bx + mouseArea.w)
            _draw = false;
        if (y < by || y > by + mouseArea.h)
            _draw = false;
    }
}
