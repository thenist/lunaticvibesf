#include "sprite.h"

#include <algorithm>
#include <cmath>

#include <common/assert.h>
#include <common/sysutil.h>
#include <game/skin/skin_lr2_bargraph.h>
#include <game/skin/skin_lr2_number.h>

SpriteSlider::SpriteSlider(const SpriteSliderBuilder& builder) : SpriteAnimated(builder)
{
    _type = SpriteTypes::SLIDER;
    dir = builder.sliderDirection;
    sliderInd = builder.sliderInd;
    valueRange = builder.sliderRange;
    _callback = builder.callOnChanged;
}

void SpriteSlider::updateVal(double v)
{
    value = v;
}

void SpriteSlider::updateValByInd()
{
    updateVal(State::get(sliderInd));
}

void SpriteSlider::updatePos()
{
    LVF_DEBUG_ASSERT(!std::isnan(value));
    int pos_diff = static_cast<int>(std::floor((valueRange - 1) * value));
    switch (dir)
    {
    case SliderDirection::DOWN:
        minValuePos = _current.rect.y + _current.rect.h / 2;
        _current.rect.y += pos_diff;
        break;
    case SliderDirection::UP:
        minValuePos = _current.rect.y + _current.rect.h / 2;
        _current.rect.y -= pos_diff;
        break;
    case SliderDirection::RIGHT:
        minValuePos = _current.rect.x + _current.rect.w / 2;
        _current.rect.x += pos_diff;
        break;
    case SliderDirection::LEFT:
        minValuePos = _current.rect.x + _current.rect.w / 2;
        _current.rect.x -= pos_diff;
        break;
    }
}

bool SpriteSlider::update(const lunaticvibes::Time& t)
{
    if (SpriteAnimated::update(t))
    {
        updateValByInd();
        updatePos();
        return true;
    }
    return false;
}

bool SpriteSlider::OnClick(int x, int y)
{
    if (!_draw)
        return false;
    if (valueRange == 0)
        return false;

    bool inRange = false;
    switch (dir)
    {
    case SliderDirection::UP:
        if (_current.rect.x <= x && x < _current.rect.x + _current.rect.w && minValuePos - valueRange <= y &&
            y <= minValuePos)
        {
            inRange = true;
        }
        break;
    case SliderDirection::DOWN:
        if (_current.rect.x <= x && x < _current.rect.x + _current.rect.w && minValuePos <= y &&
            y <= minValuePos + valueRange)
        {
            inRange = true;
        }
        break;
    case SliderDirection::LEFT:
        if (_current.rect.y <= y && y < _current.rect.y + _current.rect.h && minValuePos - valueRange <= x &&
            x <= minValuePos)
        {
            inRange = true;
        }
        break;
    case SliderDirection::RIGHT:
        if (_current.rect.y <= y && y < _current.rect.y + _current.rect.h && minValuePos <= x &&
            x <= minValuePos + valueRange)
        {
            inRange = true;
        }
        break;
    }
    if (inRange)
    {
        OnDrag(x, y);
        return true;
    }
    return false;
}

bool SpriteSlider::OnDrag(int x, int y)
{
    if (!_draw)
        return false;
    if (valueRange == 0)
        return false;

    double val = 0.0;
    switch (dir)
    {
    case SliderDirection::UP: val = double(minValuePos - y) / valueRange; break;
    case SliderDirection::RIGHT: val = double(x - minValuePos) / valueRange; break;
    case SliderDirection::DOWN: val = double(y - minValuePos) / valueRange; break;
    case SliderDirection::LEFT: val = double(minValuePos - x) / valueRange; break;
    }
    val = std::clamp(val, 0.0, 1.0);
    if (std::abs(value - val) > 0.000001) // this should be enough
    {
        value = val;
        _callback(value);
        return true;
    }
    return false;
}
