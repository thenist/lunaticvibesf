#pragma once

#include <common/beat.h>

namespace lunaticvibes
{

struct NumberAnimation
{
    lunaticvibes::Time start{0};
    lunaticvibes::Time end{0};
    double from = 0.0;
    double to = 0.0;

    [[nodiscard]] double animate(const lunaticvibes::Time& now) const;
};

} // namespace lunaticvibes
