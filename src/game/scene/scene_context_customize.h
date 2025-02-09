#pragma once

#include <common/types.h>

#include <cstddef>

using std::size_t;

struct CustomizeContextParams
{
    SkinType mode;
    bool modeUpdate = false;

    int skinDir = 0;

    bool optionUpdate = false;
    size_t optionIdx;
    int optionDir = 0;

    bool optionDragging = false;
};
