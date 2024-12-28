#include "skin_lr2_number_animation.h"

#include <common/beat.h>

// Formula matches LR2.
// Keyword: gradient.
double lunaticvibes::NumberAnimation::animate(const lunaticvibes::Time& now) const
{
    if (from == to)
        return from;

    if (!((end < now) || (start > now) || (end <= start)))
    {
        const auto tmp = static_cast<double>((now - start).norm()) / (end - start).norm();
        return (1.0 - tmp) * from + tmp * to;
    }

    if (start < now)
        return to;

    return from;
}
