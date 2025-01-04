#include "sprite.h"

SpriteCursor::SpriteCursor(const SpriteCursorBuilder& builder) : SpriteAnimated(builder)
{
    _type = SpriteTypes::MOUSE_CURSOR;
}

bool SpriteCursor::update(const lunaticvibes::Time& t)
{
    return SpriteAnimated::update(t);
}

void SpriteCursor::OnMouseMove(int x, int y)
{
    if (_draw)
    {
        _current.rect.x += x;
        _current.rect.y += y;
    }
}
