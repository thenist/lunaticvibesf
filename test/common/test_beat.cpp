#include <common/beat.h>

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

using uint32_t = std::uint32_t;

TEST(Beat, TimeOperators)
{
    using lunaticvibes::Time;
    EXPECT_EQ(-Time{1}, Time{-1});
    EXPECT_EQ((Time{1} + Time{1}), Time{2});
    EXPECT_EQ((Time{2} - Time{1}), Time{1});
    EXPECT_EQ((Time{std::numeric_limits<uint32_t>().max(), true} * 4.0),
              (Time{static_cast<long long>(static_cast<double>(std::numeric_limits<uint32_t>().max()) * 4.0), true}));
    EXPECT_EQ((Time{1} += Time{1}), Time{2});
    EXPECT_EQ((Time{2} -= Time{1}), Time{1});
    EXPECT_LT(Time{0}, Time{1});
    EXPECT_GT(Time{1}, Time{0});
    EXPECT_LE(Time{0}, Time{0});
    EXPECT_LE(Time{0}, Time{1});
    EXPECT_GE(Time{1}, Time{0});
    EXPECT_GE(Time{1}, Time{1});
    EXPECT_EQ(Time{0}, Time{0});
    EXPECT_NE(Time{0}, Time{1});
}
