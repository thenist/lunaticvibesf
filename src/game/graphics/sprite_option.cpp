#include "sprite.h"

#include <game/runtime/index/option.h>
#include <game/runtime/index/switch.h>

SpriteOption::SpriteOption(const SpriteOptionBuilder& builder) : SpriteAnimated(builder)
{
    _type = SpriteTypes::OPTION;

    switch (builder.optionType)
    {
    case opType::UNDEF: break;

    case opType::OPTION:
        indType = opType::OPTION;
        ind.op = static_cast<IndexOption>(builder.optionInd);
        break;

    case opType::SWITCH:
        indType = opType::SWITCH;
        ind.sw = static_cast<IndexSwitch>(builder.optionInd);
        break;

    case opType::FIXED:
        indType = opType::FIXED;
        ind.fix = builder.optionInd;
        break;
    }
}

bool SpriteOption::setInd(opType type, unsigned ind)
{
    if (indType != opType::UNDEF)
        return false;
    switch (type)
    {
    case opType::UNDEF: return false;

    case opType::OPTION:
        indType = opType::OPTION;
        this->ind.op = static_cast<IndexOption>(ind);
        return true;

    case opType::SWITCH:
        indType = opType::SWITCH;
        this->ind.sw = static_cast<IndexSwitch>(ind);
        return true;

    case opType::FIXED:
        indType = opType::FIXED;
        this->ind.fix = ind;
        return true;
    }
    return false;
}

void SpriteOption::updateVal(unsigned v)
{
    value = v;
    updateSelection(v);
}

void SpriteOption::updateValByInd()
{
    switch (indType)
    {
    case opType::UNDEF: break;
    case opType::SWITCH: updateVal(State::get(ind.sw)); break;
    case opType::OPTION: updateVal(State::get(ind.op)); break;
    case opType::FIXED: updateVal(ind.fix); break;
    }
}

bool SpriteOption::update(const lunaticvibes::Time& t)
{
    if (SpriteAnimated::update(t))
    {
        updateValByInd();
        return true;
    }
    return false;
}
