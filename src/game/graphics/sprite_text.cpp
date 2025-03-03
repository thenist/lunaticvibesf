#include "sprite.h"

#include <common/assert.h>
#include <common/sysutil.h>
#include <game/graphics/font.h>

#include <cmath>
#include <string_view>

SpriteText::SpriteText(const SpriteTextBuilder& builder) : SpriteBase(builder)
{
    _type = SpriteTypes::TEXT;
    pFont = builder.font;
    textInd = builder.textInd;
    align = builder.align;
    textHeight = builder.ptsize * 3 / 2;
    textColor = builder.color;
    editable = builder.editable;
    _lvf_use_readme_line = builder.lvf_use_readme_line;
}

bool SpriteText::update(const lunaticvibes::Time& t)
{
    _draw = updateMotion(t);
    return _draw;
}

void SpriteText::update_on_main(const lunaticvibes::Time& t)
{
    updateText();
}

void SpriteText::updateText()
{
    if (!_draw)
        return;

    State::get(textInd, _textBuf);
    updateTextTexture(_textBuf, _current.color);
    updateTextRect();
}

void SpriteText::updateTextTexture(std::string_view text, const Color& c)
{
    LVF_DEBUG_ASSERT(IsMainThread());

    if (!pFont || !pFont->isLoaded())
        return;

    if (pTexture != nullptr && _text == text && textColor == c)
        return;

    if (text.empty() || c.a == 0)
    {
        pTexture = nullptr;
        _draw = false;
        return;
    }

    _text = text;
    textColor = c;

    pTexture = pFont->build_texture(_text.c_str(), c);
    if (pTexture)
    {
        textureRect = pTexture->getRect();
        _draw = true;
    }
    else
    {
        _draw = false;
    }
}

void SpriteText::updateTextRect()
{
    // fitting
    if (textureRect.h == 0)
        return;
    const double sizeFactor = static_cast<double>(_current.rect.h) / textureRect.h;
    LVF_DEBUG_ASSERT(!std::isnan(sizeFactor));
    int text_w = static_cast<int>(std::round(textureRect.w * sizeFactor));
    switch (align)
    {
    case TEXT_ALIGN_LEFT: break;
    case TEXT_ALIGN_CENTER: _current.rect.x -= text_w / 2; break;
    case TEXT_ALIGN_RIGHT: _current.rect.x -= text_w; break;
    }
    _current.rect.w = text_w;
}

bool SpriteText::OnClick(int x, int y)
{
    if (!editable)
        return false;
    if (_current.rect.x <= x && x < _current.rect.x + _current.rect.w && _current.rect.y <= y &&
        y < _current.rect.y + _current.rect.h)
        return true;
    return false;
}

void SpriteText::startEditing(bool clear)
{
    if (isEditing())
        return;
    editing = true;
    textBeforeEdit = State::get(textInd);
    textAfterEdit = (clear ? "" : _text);
    lunaticvibes::window::startTextInput(_current.rect, textAfterEdit,
                                         std::bind_front(&SpriteText::updateTextWhileEditing, this));
}

void SpriteText::stopEditing(bool modify)
{
    if (!isEditing())
        return;
    lunaticvibes::window::stopTextInput();
    editing = false;
    State::set(textInd, (modify ? textAfterEdit : textBeforeEdit));
    pushMainThreadTask(std::bind_front(&SpriteText::updateText, this));
}

void SpriteText::updateTextWhileEditing(const std::string& text)
{
    textAfterEdit = text;
    State::set(textInd, text + "|");
    pushMainThreadTask(std::bind_front(&SpriteText::updateText, this));
}

void SpriteText::draw() const
{
    if (isHidden())
        return;

    if (_draw && pTexture && pTexture->isLoaded())
    {
        pTexture->draw(textureRect, _current.rect, _current.color, _current.blend, _current.filter, _current.angle,
                       _current.center);
    }
}
