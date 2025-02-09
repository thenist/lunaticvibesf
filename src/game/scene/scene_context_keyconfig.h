#pragma once

#include <common/keymap.h>
#include <common/types.h>

#include <array>
#include <utility>

struct KeyConfigContextParams
{
    GameModeKeys keys;
    std::pair<Input::Pad, int> selecting = {Input::Pad::K11, 0};
    bool modeChanged = false;
    bool skinHasAbsAxis = false;
    std::array<double, Input::Pad::KEY_COUNT> bargraphForce;
};
