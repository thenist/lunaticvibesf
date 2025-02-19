#include <sstream>
#include <utility>

#include <gtest/gtest.h>

#include "common/coursefile/lr2crs.h"
#include "common/hash.h"

TEST(Lr2crs, ParsesSuccessfully)
{
    static constexpr long long add_time{12345};

    std::stringstream ss;
    ss << R"(<?xml version="1.0" encoding="shift_jis"?>
<!--曲は上から順に登録されます。-->
<courselist>

	<course>
		<title>GENOSIDE 2018 段位認定 発狂初段</Title>
		<line>7</line>
		<hash>00000000002000000000000000005190f872dd65dd08638b06d80470a3233fb91b72e8f6439a698e78f94be16470b7892371263af3b0d644fba62526c9f494818a8a6c2f3511eb0876a6c9a2027d7bbe</hash>
		<type>2</type>
	</course>

</courselist>)";

    CourseLr2crs lr2crs("test.lr2crs", add_time, std::move(ss));
    EXPECT_EQ(lr2crs.addTime, add_time);
    ASSERT_EQ(lr2crs.courses.size(), 1);
    const auto& course = lr2crs.courses[0];
    EXPECT_EQ(course.title, "GENOSIDE 2018 段位認定 発狂初段");
    EXPECT_EQ(course.line, 7);
    EXPECT_EQ(course.type, CourseLr2crs::Course::COURSE_GRADE);
    EXPECT_EQ(course.hashTop, "00000000002000000000000000005190");
    ASSERT_EQ(course.chartHash.size(), 4);
    EXPECT_EQ(course.chartHash[0], HashMD5{"f872dd65dd08638b06d80470a3233fb9"});
    EXPECT_EQ(course.chartHash[1], HashMD5{"1b72e8f6439a698e78f94be16470b789"});
    EXPECT_EQ(course.chartHash[2], HashMD5{"2371263af3b0d644fba62526c9f49481"});
    EXPECT_EQ(course.chartHash[3], HashMD5{"8a8a6c2f3511eb0876a6c9a2027d7bbe"});
    EXPECT_EQ(course.getCourseHash(), HashMD5{"fe4c64fd329a55bc2499be2014a1f13a"});
}
