#include "sprite.h"

#include <game/skin/skin_lr2_button_callbacks.h>

SpriteButton::SpriteButton(const SpriteButtonBuilder& builder) : SpriteOption(builder)
{
    _type = SpriteTypes::BUTTON;
    clickableOnPanel = builder.clickableOnPanel;
    plusonlyDelta = builder.plusonlyDelta;
    _buttonInd = builder.buttonInd;
}

bool SpriteButton::OnClick(int x, int y)
{
    if (!_draw)
        return false;

    if (clickableOnPanel < -1 || clickableOnPanel > 9)
        return false;
    if (!lunaticvibes::isPanelOpen(clickableOnPanel))
        return false;

    if (plusonlyDelta == 0)
    {
        if (y >= _current.rect.y && y < _current.rect.y + _current.rect.h)
        {
            int w_opt = _current.rect.w / 2;
            if (x >= _current.rect.x && x < _current.rect.x + w_opt)
            {
                lr2skin::button::invoke(_buttonInd, -1);
                return true;
            }
            else if (x >= _current.rect.x + w_opt && x < _current.rect.x + _current.rect.w)
            {
                lr2skin::button::invoke(_buttonInd, 1);
                return true;
            }
        }
    }
    else
    {
        if (x >= _current.rect.x && x < _current.rect.x + _current.rect.w && y >= _current.rect.y &&
            y < _current.rect.y + _current.rect.h)
        {
            lr2skin::button::invoke(_buttonInd, plusonlyDelta);
            return true;
        }
    }
    return false;
}
