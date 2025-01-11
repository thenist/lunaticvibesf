#pragma once

#include <common/beat.h>
#include <common/types.h>

namespace lunaticvibes
{

enum class FrameTimeType : uint8_t
{
    Fps,
    Input,
};

void g_feed_frametime(FrameTimeType, const lunaticvibes::Time&);
void g_update_generic_info(const lunaticvibes::Time&);

} // namespace lunaticvibes
