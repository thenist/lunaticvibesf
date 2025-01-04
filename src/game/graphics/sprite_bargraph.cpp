#include "sprite.h"

#include <game/skin/skin_lr2_bargraph.h>

////////////////////////////////////////////////////////////////////////////////
// Bargraph

SpriteBargraph::SpriteBargraph(const SpriteBargraphBuilder& builder) : SpriteAnimated(builder)
{
    _type = SpriteTypes::BARGRAPH;
    dir = builder.barDirection;
    barInd = builder.barInd;
}

void SpriteBargraph::updateVal(Ratio v)
{
    value = v;
}

void SpriteBargraph::updateValByInd()
{
    updateVal(lunaticvibes::get_bargraph(barInd));
}

#pragma warning(push)
#pragma warning(disable : 4244)
void SpriteBargraph::updateSize()
{
    int tmp;
    switch (dir)
    {
    case BargraphDirection::DOWN: _current.rect.h *= value; break;
    case BargraphDirection::UP:
        tmp = _current.rect.h;
        _current.rect.h *= value;
        _current.rect.y += tmp - _current.rect.h;
        break;
    case BargraphDirection::RIGHT: _current.rect.w *= value; break;
    case BargraphDirection::LEFT:
        tmp = _current.rect.w;
        _current.rect.w *= value;
        _current.rect.x += tmp - _current.rect.w;
        break;
    }
}
#pragma warning(pop)

bool SpriteBargraph::update(const lunaticvibes::Time& t)
{
    if (SpriteAnimated::update(t))
    {
        updateValByInd();
        updateSize();
        return true;
    }
    return false;
}
