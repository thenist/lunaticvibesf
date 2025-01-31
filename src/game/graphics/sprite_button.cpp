#include "sprite.h"

SpriteButton::SpriteButton(const SpriteButtonBuilder& builder) : SpriteOption(builder)
{
    _type = SpriteTypes::BUTTON;
    clickableOnPanel = builder.clickableOnPanel;
    plusonlyDelta = builder.plusonlyDelta;
    callOnClick = builder.callOnClick;
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
        int w_opt = _current.rect.w / 2;
        if (y >= _current.rect.y && y < _current.rect.y + _current.rect.h)
        {
            if (x >= _current.rect.x && x < _current.rect.x + w_opt)
            {
                // minus
                callOnClick(-1);
                return true;
            }
            else if (x >= _current.rect.x + w_opt && x < _current.rect.x + _current.rect.w)
            {
                // plus
                callOnClick(1);
                return true;
            }
        }
    }
    else
    {
        if (x >= _current.rect.x && x < _current.rect.x + _current.rect.w && y >= _current.rect.y &&
            y < _current.rect.y + _current.rect.h)
        {
            // plusonly
            callOnClick(plusonlyDelta);
            return true;
        }
    }
    return false;
}
