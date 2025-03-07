#include "sprite.h"

#include <game/skin/skin_lr2_number.h>

#include <cmath>

SpriteGaugeGrid::SpriteGaugeGrid(const SpriteGaugeGridBuilder& builder) : SpriteAnimated(builder)
{
    _type = SpriteTypes::GAUGE;
    gridSizeW = builder.dx;
    gridSizeH = builder.dy;
    totalGrids = builder.gridCount;
    minValue = builder.gaugeMin;
    maxValue = builder.gaugeMax;
    numInd = builder.numInd;

    flashing.resize(totalGrids, false);
    setGaugeType(GaugeType::GROOVE);
}

void SpriteGaugeGrid::setFlashType(SpriteGaugeGrid::FlashType t)
{
    flashType = t;
}

void SpriteGaugeGrid::setGaugeType(SpriteGaugeGrid::GaugeType ty)
{
    gaugeType = ty;
    switch (gaugeType)
    {
    case GaugeType::ASSIST_EASY:
        lightFailGridType = NORMAL_LIGHT;
        darkFailGridType = NORMAL_DARK;
        lightClearGridType = CLEAR_LIGHT;
        darkClearGridType = CLEAR_DARK;
        failGrids = static_cast<unsigned short>(std::floor(0.6 * totalGrids));
        break;

    case GaugeType::GROOVE:
        lightFailGridType = NORMAL_LIGHT;
        darkFailGridType = NORMAL_DARK;
        lightClearGridType = CLEAR_LIGHT;
        darkClearGridType = CLEAR_DARK;
        failGrids = static_cast<unsigned short>(std::floor(0.8 * totalGrids));
        break;

    case GaugeType::SURVIVAL:
        lightFailGridType = CLEAR_LIGHT;
        darkFailGridType = CLEAR_DARK;
        lightClearGridType = CLEAR_LIGHT;
        darkClearGridType = CLEAR_DARK;
        failGrids = 1;
        break;

    case GaugeType::EX_SURVIVAL:
        if (textureRects.size() > EXHARD_LIGHT)
        {
            lightFailGridType = EXHARD_LIGHT;
            darkFailGridType = EXHARD_DARK;
            lightClearGridType = EXHARD_LIGHT;
            darkClearGridType = EXHARD_DARK;
        }
        else
        {
            lightFailGridType = CLEAR_LIGHT;
            darkFailGridType = CLEAR_DARK;
            lightClearGridType = CLEAR_LIGHT;
            darkClearGridType = CLEAR_DARK;
        }
        failGrids = 1;
        break;
    default: break;
    }

    lunaticvibes::Time t(1);

    // set FailRect
    updateSelection(lightFailGridType);
    SpriteAnimated::update(t);
    lightFailRectIdxOffset = static_cast<unsigned>(selectionIndex * animationFrames);
    updateSelection(darkFailGridType);
    SpriteAnimated::update(t);
    darkFailRectIdxOffset = static_cast<unsigned>(selectionIndex * animationFrames);

    // set ClearRect
    updateSelection(lightClearGridType);
    SpriteAnimated::update(t);
    lightClearRectIdxOffset = static_cast<unsigned>(selectionIndex * animationFrames);
    updateSelection(darkClearGridType);
    SpriteAnimated::update(t);
    darkClearRectIdxOffset = static_cast<unsigned>(selectionIndex * animationFrames);
}

void SpriteGaugeGrid::updateVal(unsigned v)
{
    value = totalGrids * (v - minValue) / (maxValue - minValue);
}

void SpriteGaugeGrid::updateValByInd()
{
    updateVal(lunaticvibes::get_number(numInd));
}

bool SpriteGaugeGrid::update(const lunaticvibes::Time& t)
{
    if (SpriteAnimated::update(t))
    {
        updateValByInd();
        switch (flashType)
        {
        case FlashType::NONE:
            for (unsigned i = 0; i < value; ++i)
                flashing[i] = true;
            for (unsigned i = value; i < totalGrids; ++i)
                flashing[i] = false;
            break;

        case FlashType::CLASSIC:
            for (unsigned i = 0; i < value; ++i)
                flashing[i] = true;
            if (value - 3 >= 0 && value - 3 < totalGrids && t.norm() / 17 % 2)
                flashing[value - 3] = false; // -3 grid: 17ms, per 2 units (1 0 1 0)
            if (value - 2 >= 0 && value - 2 < totalGrids && t.norm() / 17 % 4)
                flashing[value - 2] = false; // -2 grid: 17ms, per 4 units (1 0 0 0)
            for (unsigned i = value; i < totalGrids; ++i)
                flashing[i] = false;
            break;

        default: break;
        }
        return true;
    }
    return false;
}

void SpriteGaugeGrid::draw() const
{
    if (isHidden())
        return;

    if (_draw && pTexture != nullptr && pTexture->isLoaded())
    {
        RectF r = _current.rect;
        auto grid_val = static_cast<unsigned>(failGrids - 1);
        const Rect clear_tex_flashing = textureRects[lightClearRectIdxOffset + animationFrameIndex];
        const Rect clear_tex = textureRects[darkClearRectIdxOffset + animationFrameIndex];
        const Rect fail_tex_flashing = textureRects[lightFailRectIdxOffset + animationFrameIndex];
        const Rect fail_tex = textureRects[darkFailRectIdxOffset + animationFrameIndex];
        for (unsigned i = 0; i < grid_val; ++i)
        {
            pTexture->draw(flashing[i] ? fail_tex_flashing : fail_tex, r, _current.color, _current.blend,
                           _current.filter, _current.angle);
            r.x += gridSizeW;
            r.y += gridSizeH;
        }
        for (unsigned i = grid_val; i < totalGrids; ++i)
        {
            pTexture->draw(flashing[i] ? clear_tex_flashing : clear_tex, r, _current.color, _current.blend,
                           _current.filter, _current.angle);
            r.x += gridSizeW;
            r.y += gridSizeH;
        }
    }
}
