#include <gmock/gmock.h>

#include <common/chartformat/chartformat.h>
#include <common/types.h>

TEST(chartformat, FileFormat)
{
    EXPECT_EQ(analyzeChartType(u8"bms/fileテスト.bms"_p), eChartFormat::BMS);
    EXPECT_EQ(analyzeChartType(u8"bms/file.bms"_p), eChartFormat::BMS);
    EXPECT_EQ(analyzeChartType(u8"bms/file.bme"_p), eChartFormat::BMS);
    EXPECT_EQ(analyzeChartType(u8"bms/file.bml"_p), eChartFormat::BMS);
    EXPECT_EQ(analyzeChartType(u8"bms/file.pms"_p), eChartFormat::BMS);
    EXPECT_EQ(analyzeChartType(u8"bms/file.bmson"_p), eChartFormat::BMSON);
    EXPECT_EQ(analyzeChartType(u8"bms/readme_utf8.crlf.txt"_p), eChartFormat::UNKNOWN);
    EXPECT_EQ(analyzeChartType(u8"bms/file.テスト"_p), eChartFormat::UNKNOWN);
    EXPECT_EQ(analyzeChartType(u8"bms/.bms"_p), eChartFormat::UNKNOWN);
}

TEST(chartformat, FormatReadme)
{
    EXPECT_EQ(ChartFormatBase::formatReadmeText({{"important.txt", "sobaudonramen"}}),
              "important.txt\n\nsobaudonramen\n");
    EXPECT_EQ(ChartFormatBase::formatReadmeText({{"", "a"}, {"あ.txt", "あ\nｗ"}}),
              "1/2 \n\na\n2/2 あ.txt\n\nあ\nｗ\n");
}
