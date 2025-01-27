#include "state.h"

#include "common/types.h"

State& State::inst()
{
    static State state;
    return state;
};

State::State()
{
    gNumbers.setDefault(IndexNumber::ZERO, 0);
    gSwitches.setDefault(IndexSwitch::_FALSE, false);
    gSwitches.setDefault(IndexSwitch::_TRUE, true);
    gSliders.setDefault(IndexSlider::ZERO, 0);
    gTexts.setDefault(IndexText::INVALID, "");
    gTimers.setDefault(IndexTimer::_NEVER, TIMER_NEVER);

    gNumbers.reset();
    gSwitches.reset();
    gSliders.reset();
    gTexts.reset();
    gTimers.reset();
}

bool State::set(IndexNumber ind, int val)
{
    return inst().gNumbers.set(ind, val);
}

int State::get(IndexNumber ind)
{
    return inst().gNumbers.get(ind);
}

bool State::set(IndexOption ind, unsigned val)
{
    return inst().gOptions.set(ind, val);
}

unsigned State::get(IndexOption ind)
{
    return inst().gOptions.get(ind);
}

bool State::set(IndexSlider ind, Ratio val)
{
    return inst().gSliders.set(ind, val);
}

double State::get(IndexSlider ind)
{
    return inst().gSliders.get(ind);
}

bool State::set(IndexSwitch ind, bool val)
{
    return inst().gSwitches.set(ind, val);
}

bool State::get(IndexSwitch ind)
{
    return inst().gSwitches.get(ind);
}

bool State::set(IndexText ind, std::string_view val)
{
    return inst().gTexts.set(ind, val);
}

std::string State::get(IndexText ind)
{
    return inst().gTexts.get(ind);
}

void State::get(IndexText ind, std::string& out)
{
    inst().gTexts.get(ind, out);
}

bool State::set(IndexTimer ind, long long val)
{
    return inst().gTimers.set(ind, val);
}

long long State::get(IndexTimer ind)
{
    return inst().gTimers.get(ind);
}

void State::resetTimer()
{
    long long customizeTimer = get(IndexTimer::_SCENE_CUSTOMIZE_START);
    inst().gTimers.reset();
    set(IndexTimer::_SCENE_CUSTOMIZE_START, customizeTimer);
}
