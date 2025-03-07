#include "sprite.h"

#include <random>

#include <game/skin/skin_lr2_number.h>

////////////////////////////////////////////////////////////////////////////////
// Number

SpriteNumber::SpriteNumber(const SpriteNumberBuilder& builder) : SpriteAnimated(builder)
{
    _type = SpriteTypes::NUMBER;
    alignType = builder.align;
    numInd = builder.numInd;
    maxDigits = builder.maxDigits;
    hideLeadingZeros = builder.hideLeadingZeros;

    // invalid num type guard
    // numberType = NumberType(numRows * numCols);
    if (animationFrames != 0)
        numberType = NumberType(textureSheetRows * textureSheetCols / animationFrames);
    else
        numberType = NumberType(0);

    digitNumber.resize(maxDigits);
    digitOutRect.resize(maxDigits);
}

bool SpriteNumber::update(const lunaticvibes::Time& t)
{
    if (maxDigits == 0)
        return false;
    if (numberType == 0)
        return false;

    if (SpriteAnimated::update(t))
    {
        updateNumberByInd();
        updateNumberRect();
        return true;
    }
    return false;
}

void SpriteNumber::updateNumber(int n)
{
    if (n == INT_MIN)
        n = 0;

    bool positive = n >= 0;
    int zeroIdx = -1;
    auto digits = static_cast<unsigned>(digitNumber.size());
    switch (numberType)
    {
    case NUM_TYPE_NORMAL: zeroIdx = -1; break;
    case NUM_TYPE_BLANKZERO: zeroIdx = NUM_BZERO; break;
    case NUM_TYPE_FULL:
        zeroIdx = positive ? NUM_FULL_BZERO_POS : NUM_FULL_BZERO_NEG;
        digits--;
        break;
    }

    // reset by zeroIdx to prevent unexpected glitches
    for (auto& d : digitNumber)
        d = zeroIdx;

    if (n == 0)
    {
        digitNumber[0] = 0;
        digitCount = 1;
    }
    else
    {
        digitCount = 0;
        int abs_n = std::abs(n);
        for (unsigned i = 0; abs_n && i < digits; ++i)
        {
            ++digitCount;
            unsigned digit = abs_n % 10;
            abs_n /= 10;
            if (numberType == NUM_TYPE_FULL && !positive)
                digit += 12;
            digitNumber[i] = digit;
        }
    }

    // symbol
    switch (numberType)
    {
    case NUM_TYPE_NORMAL:
        // Handled above.
    case NUM_TYPE_BLANKZERO:
        // ?
        break;
    /*
    case NUM_SYMBOL:
    {
        _digit[_sDigit.size() - 1] = positive ? NUM_SYMBOL_PLUS : NUM_SYMBOL_MINUS;
        break;
    }
    */
    case NUM_TYPE_FULL: {
        switch (alignType)
        {

        case NUM_ALIGN_RIGHT:
            if (!hideLeadingZeros || digitCount == maxDigits)
                digitCount = maxDigits - 1;
            digitNumber[digitCount++] = positive ? NUM_FULL_PLUS : NUM_FULL_MINUS;
            break;

        case NUM_ALIGN_LEFT:
        case NUM_ALIGN_CENTER:
            if (digitCount == maxDigits)
                --digitCount;
            digitNumber[digitCount++] = positive ? NUM_FULL_PLUS : NUM_FULL_MINUS;
            break;
        }
        break;
    }
    }
}

void SpriteNumber::updateNumberByInd()
{

    static std::random_device rd;
    int n;
    switch (numInd)
    {
    case IndexNumber::RANDOM: n = rd(); break;
    case IndexNumber::ZERO: n = 0; break;
    default: n = lunaticvibes::get_number(numInd); break;
    }
    updateNumber(n);
}

void SpriteNumber::updateNumberRect()
{
    switch (alignType)
    {
    case NUM_ALIGN_RIGHT: {
        RectF offset{_current.rect.w * (maxDigits - 1), 0, 0, 0};
        for (size_t i = 0; i < maxDigits; ++i)
        {
            digitOutRect[i] = _current.rect + offset;
            offset.x -= _current.rect.w;
        }
        break;
    }

    case NUM_ALIGN_LEFT: {
        RectF offset{_current.rect.w * (digitCount - 1), 0, 0, 0};
        for (size_t i = 0; i < digitCount; ++i)
        {
            digitOutRect[i] = _current.rect + offset;
            offset.x -= _current.rect.w;
        }
        break;
    }

    case NUM_ALIGN_CENTER: {
        RectF offset{0, 0, 0, 0};
        if (hideLeadingZeros)
            offset.x = static_cast<int>(std::floor(_current.rect.w * 0.5 * (digitCount - 1)));
        else
            offset.x = static_cast<int>(std::floor(_current.rect.w * (0.5 * (maxDigits + digitCount) - 1)));
        for (size_t i = 0; i < digitCount; ++i)
        {
            digitOutRect[i] = _current.rect + offset;
            offset.x -= _current.rect.w;
        }
        break;
    }
    }
}

void SpriteNumber::appendMotionKeyFrame(const MotionKeyFrame& f)
{
    motionKeyFrames.push_back(f);
}

void SpriteNumber::draw() const
{
    if (isHidden())
        return;

    if (pTexture->isLoaded() && _draw)
    {
        size_t max = 0;
        switch (alignType)
        {
        case NUM_ALIGN_RIGHT: max = hideLeadingZeros ? digitCount : maxDigits; break;
        case NUM_ALIGN_LEFT:
        case NUM_ALIGN_CENTER: max = digitCount; break;
        default: break;
        }
        for (size_t i = 0; i < max; ++i)
        {
            if (digitNumber[i] != -1)
                pTexture->draw(textureRects[animationFrameIndex * selections + digitNumber[i]], digitOutRect[i],
                               _current.color, _current.blend, _current.filter, _current.angle);
        }
    }
}

void SpriteNumber::adjustAfterUpdate(int x, int y, int w, int h)
{
    SpriteBase::adjustAfterUpdate(x, y, w, h);
    for (auto& d : digitOutRect)
    {
        d.x += x - w;
        d.y += y - h;
        d.w += w * 2;
        d.h += h * 2;
    }
}
