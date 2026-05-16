#include <common/beat.h>
#include <game/runtime/generic_info.h>

#include <gtest/gtest.h>

TEST(GenericInfo, UpdateBeforeInputFrameTimeDoesNotCrash)
{
    lunaticvibes::g_update_generic_info(lunaticvibes::Time{});
}
