#include "sprite.h"

#include <common/assert.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <game/skin/skin_lr2_animation.h>
#include <game/skin/skin_lr2_bargraph.h>
#include <game/skin/skin_lr2_number.h>

#include <cmath>
#include <cstdint>

using uint8_t = std::uint8_t;

bool lunaticvibes::isPanelOpen(int panelIdx)
{
    switch (panelIdx)
    {
    case -1: {
        bool panel = State::get(IndexSwitch::SELECT_PANEL1) || State::get(IndexSwitch::SELECT_PANEL2) ||
                     State::get(IndexSwitch::SELECT_PANEL3) || State::get(IndexSwitch::SELECT_PANEL4) ||
                     State::get(IndexSwitch::SELECT_PANEL5) || State::get(IndexSwitch::SELECT_PANEL6) ||
                     State::get(IndexSwitch::SELECT_PANEL7) || State::get(IndexSwitch::SELECT_PANEL8) ||
                     State::get(IndexSwitch::SELECT_PANEL9);
        return !panel;
    }
    case 0: return true;
    case 1: return State::get(IndexSwitch::SELECT_PANEL1);
    case 2: return State::get(IndexSwitch::SELECT_PANEL2);
    case 3: return State::get(IndexSwitch::SELECT_PANEL3);
    case 4: return State::get(IndexSwitch::SELECT_PANEL4);
    case 5: return State::get(IndexSwitch::SELECT_PANEL5);
    case 6: return State::get(IndexSwitch::SELECT_PANEL6);
    case 7: return State::get(IndexSwitch::SELECT_PANEL7);
    case 8: return State::get(IndexSwitch::SELECT_PANEL8);
    case 9: return State::get(IndexSwitch::SELECT_PANEL9);
    default: lunaticvibes::assert_failed("isPanelPressed");
    }
}

RenderParams& RenderParams::operator=(const MotionKeyFrameParams& rhs)
{
    rect.x = static_cast<float>(rhs.rect.x);
    rect.y = static_cast<float>(rhs.rect.y);
    rect.w = static_cast<float>(rhs.rect.w);
    rect.h = static_cast<float>(rhs.rect.h);

    accel = rhs.accel;
    color = rhs.color;
    blend = rhs.blend;
    filter = rhs.filter;
    angle = rhs.angle;
    center = rhs.center;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// virtual base class functions
SpriteBase::SpriteBase(const SpriteBuilder& builder)
    : _type(SpriteTypes::VIRTUAL), pTexture(builder.texture), srcLine(builder.srcLine),
      _current{0, MotionKeyFrameParams::CONSTANT, 0x00000000, BlendMode::NONE, false, 0, 0}
{
}

bool SpriteBase::updateMotion(const lunaticvibes::Time& rawTime)
{
    // Check if object is valid
    // Note that nullptr texture shall pass
    if (pTexture != nullptr && !pTexture->isLoaded())
        return false;

    // Check if frames are valid
    size_t frameCount = motionKeyFrames.size();
    if (frameCount < 1)
        return false;

    // Check if timer is valid
    lunaticvibes::Time time = State::get(motionStartTimer);
    if (time < 0 || time == TIMER_NEVER)
        return false;

    // Check if timer is 140
    if (motionStartTimer != IndexTimer::MUSIC_BEAT)
    {
        time = rawTime - time;
    }

    // Check if the sprite is not visible yet
    if (!drawn && motionKeyFrames[0].time > 0 && time.norm() < motionKeyFrames[0].time)
        return false;

    // Check if import time is valid
    if (time.norm() < 0)
        return false;

    // Check if loop target is valid
    lunaticvibes::Time endTime = lunaticvibes::Time(motionKeyFrames[frameCount - 1].time, false);
    if (motionLoopTo < 0 && time > endTime)
        return false;
    if (motionLoopTo > motionKeyFrames[frameCount - 1].time)
        time = motionKeyFrames[frameCount - 1].time;

    // crop time into valid section
    if (time > endTime)
    {
        if (endTime != motionLoopTo)
            time = lunaticvibes::Time((time - motionLoopTo).norm() % (endTime - motionLoopTo).norm() + motionLoopTo,
                                      false);
        else
            time = motionLoopTo;
    }

    // Check if specific time
    if (time == motionKeyFrames[frameCount - 1].time)
    {
        // exactly last frame
        _current = motionKeyFrames[frameCount - 1].param;
    }
    else if (frameCount == 1 || time.norm() <= motionKeyFrames[0].time)
    {
        // exactly first frame
        _current = motionKeyFrames[0].param;
    }
    else
    {
        // get keyFrame section (iterators)
        decltype(motionKeyFrames.begin()) keyFrameCurr, keyFrameNext;
        for (auto it = motionKeyFrames.begin(); it != motionKeyFrames.end(); ++it)
        {
            if (it->time <= time.norm())
                keyFrameCurr = it;
            else
                break;
        }
        keyFrameNext = keyFrameCurr;
        if (keyFrameCurr + 1 != motionKeyFrames.end())
            ++keyFrameNext;

        // Check if section period is 0
        auto keyFrameLength = keyFrameNext->time - keyFrameCurr->time;
        if (keyFrameLength == 0)
        {
            _current = keyFrameCurr->param;
        }
        else
        {
            auto animate = [multiplier = lunaticvibes::calc_animation_multiplier(
                                keyFrameCurr->time, keyFrameNext->time, time.norm(), keyFrameCurr->param.accel),
                            start = keyFrameCurr->time, end = keyFrameNext->time,
                            now = time.norm()](double to, double from) {
                return lunaticvibes::animate(from, to, start, end, now, multiplier);
            };

            _current.rect.x = static_cast<float>(animate(keyFrameNext->param.rect.x, keyFrameCurr->param.rect.x));
            _current.rect.y = static_cast<float>(animate(keyFrameNext->param.rect.y, keyFrameCurr->param.rect.y));
            _current.rect.w = static_cast<float>(animate(keyFrameNext->param.rect.w, keyFrameCurr->param.rect.w));
            _current.rect.h = static_cast<float>(animate(keyFrameNext->param.rect.h, keyFrameCurr->param.rect.h));
            _current.color.r = static_cast<uint8_t>(animate(keyFrameNext->param.color.r, keyFrameCurr->param.color.r));
            _current.color.g = static_cast<uint8_t>(animate(keyFrameNext->param.color.g, keyFrameCurr->param.color.g));
            _current.color.b = static_cast<uint8_t>(animate(keyFrameNext->param.color.b, keyFrameCurr->param.color.b));
            _current.color.a = static_cast<uint8_t>(animate(keyFrameNext->param.color.a, keyFrameCurr->param.color.a));
            _current.angle = animate(keyFrameNext->param.angle, keyFrameCurr->param.angle);
            _current.center = keyFrameCurr->param.center;
            _current.blend = keyFrameCurr->param.blend;
            _current.filter = keyFrameCurr->param.filter;
        }
    }

    return true;
}

bool SpriteBase::update(const lunaticvibes::Time& t)
{
    _draw = updateMotion(t);

    if (_draw)
        drawn = true;
    return _draw;
}

void SpriteBase::setMotionStartTimer(IndexTimer t)
{
    motionStartTimer = t;
}

void SpriteBase::appendMotionKeyFrame(const MotionKeyFrame& f)
{
    motionKeyFrames.push_back(f);
}

void SpriteBase::setMotionLoopTo(int time)
{
    motionLoopTo = time;
}

void SpriteBase::adjustAfterUpdate(int x, int y, int w, int h)
{
    _current.rect.x += x - w;
    _current.rect.y += y - h;
    _current.rect.w += w * 2;
    _current.rect.h += h * 2;
}

////////////////////////////////////////////////////////////////////////////////
// Static

SpriteStatic::SpriteStatic(const SpriteStaticBuilder& builder) : SpriteBase(builder)
{
    _type = SpriteTypes::STATIC;
    // Even if RECT_FULL - it can be used by the renderer if the texture can change the size.
    textureRect = builder.textureRect;
}

SpriteStatic::SpriteStatic(std::shared_ptr<Texture> texture, const Rect& texRect, int srcLine)
    : SpriteBase(SpriteTypes::STATIC, srcLine)
{
    pTexture = std::move(texture);
    // Even if RECT_FULL - it can be used by the renderer if the texture can change the size.
    textureRect = texRect;
}

void SpriteStatic::draw() const
{
    if (isHidden())
        return;

    if (_draw && pTexture->isLoaded())
        pTexture->draw(textureRect, _current.rect, _current.color, _current.blend, _current.filter, _current.angle,
                       _current.center);
}

////////////////////////////////////////////////////////////////////////////////
// Split

SpriteSelection::SpriteSelection(const SpriteSelectionBuilder& builder) : SpriteBase(builder)
{
    _type = SpriteTypes::SPLIT;
    textureSheetRows = builder.textureSheetRows;
    textureSheetCols = builder.textureSheetCols;

    if (textureSheetRows == 0 || textureSheetCols == 0)
    {
        textureRects.resize(0);
        return;
    }

    Rect rcGrid = builder.textureRect;
    if (textureSheetRows * textureSheetCols != 1)
    {
        if (pTexture && builder.textureRect == RECT_FULL)
            rcGrid = pTexture->getRect();
        rcGrid.w /= textureSheetCols;
        rcGrid.h /= textureSheetRows;
    }
    // else NOTE: avoid using texture's rect in simpler case to preserve RECT_FULL in case the underlying texture can
    // change size (e.g. dynamic texture).

    if (!builder.textureSheetVerticalIndexing)
    {
        // Horizontal first
        for (unsigned r = 0; r < textureSheetRows; ++r)
            for (unsigned c = 0; c < textureSheetCols; ++c)
                textureRects.emplace_back(rcGrid.x + rcGrid.w * c, rcGrid.y + rcGrid.h * r, rcGrid.w, rcGrid.h);
    }
    else
    {
        // Vertical first
        for (unsigned c = 0; c < textureSheetCols; ++c)
            for (unsigned r = 0; r < textureSheetRows; ++r)
                textureRects.emplace_back(rcGrid.x + rcGrid.w * c, rcGrid.y + rcGrid.h * r, rcGrid.w, rcGrid.h);
    }
}

void SpriteSelection::draw() const
{
    if (isHidden())
        return;

    if (_draw && pTexture->isLoaded())
        pTexture->draw(textureRects[selectionIndex], _current.rect, _current.color, _current.blend, _current.filter,
                       _current.angle, _current.center);
}

void SpriteSelection::updateSelection(size_t frame)
{
    selectionIndex = frame < textureRects.size() ? frame : textureRects.size() - 1;
}

bool SpriteSelection::update(const lunaticvibes::Time& t)
{
    return SpriteBase::update(t);
}

////////////////////////////////////////////////////////////////////////////////
// Animated

SpriteAnimated::SpriteAnimated(const SpriteAnimatedBuilder& builder) : SpriteSelection(builder)
{
    _type = SpriteTypes::ANIMATED;
    animationFrames = builder.animationFrameCount;
    animationStartTimer = builder.animationTimer;

    if (textureRects.empty() || animationFrames == 0)
        return;

    selections = textureSheetRows * textureSheetCols / animationFrames;
    animationDurationPerLoop = builder.animationDurationPerLoop;
}

bool SpriteAnimated::update(const lunaticvibes::Time& t)
{
    if (SpriteSelection::update(t))
    {
        long long timerAnim = State::get(animationStartTimer);
        if (timerAnim > 0 && timerAnim != TIMER_NEVER)
            updateAnimation(t - lunaticvibes::Time(timerAnim));

        return true;
    }
    return false;
}

void SpriteAnimated::updateAnimation(const lunaticvibes::Time& time)
{
    if (textureRects.empty())
        return;
    if (animationDurationPerLoop == static_cast<unsigned>(-1))
        return;

    if (double timeEachFrame = static_cast<double>(animationDurationPerLoop) / animationFrames; timeEachFrame >= 1.0)
    {
        auto animFrameTime = (time.norm() >= 0) ? (time.norm() % animationDurationPerLoop) : 0;
        animationFrameIndex = static_cast<size_t>(std::floor(animFrameTime / timeEachFrame));
    }
}

void SpriteAnimated::draw() const
{
    if (isHidden())
        return;

    if (_draw && animationFrameIndex < textureRects.size() && pTexture != nullptr && pTexture->isLoaded())
    {
        pTexture->draw(textureRects[selectionIndex * animationFrames + animationFrameIndex], _current.rect,
                       _current.color, _current.blend, _current.filter, _current.angle, _current.center);
    }
}
