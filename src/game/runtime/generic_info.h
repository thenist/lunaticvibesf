#pragma once

#include <common/beat.h>
#include <common/types.h>

#include <atomic>

inline std::atomic<unsigned> gFrameCount[2]{0};
enum lunaticvibesFrameCountIndex : uint8_t
{
    FRAMECOUNT_IDX_FPS = 0,
    FRAMECOUNT_IDX_INPUT = 1,
};

namespace lunaticvibes
{

constexpr unsigned global_generic_info_update_rate = 2;
void update_global_generic_info(const lunaticvibes::Time&);

} // namespace lunaticvibes
