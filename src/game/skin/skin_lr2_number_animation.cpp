#include "skin_lr2_number_animation.h"

#include <common/beat.h>
#include <game/graphics/sprite.h>
#include <game/skin/skin_lr2_animation.h>

// Formula matches LR2.
// Keyword: gradient.
double lunaticvibes::NumberAnimation::animate(const lunaticvibes::Time& now) const
{
    if (from == to)
        return from;

    if (end >= now && start <= now && end > start)
        return grad(to, from,
                    calc_animation_multiplier(start.norm(), end.norm(), now.norm(), MotionKeyFrameParams::CONSTANT));

    if (start < now)
        return to;

    return from;
}
