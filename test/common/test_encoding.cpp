#include <fstream>

#include <gmock/gmock.h>

#include <common/encoding.h>
#include <common/types.h>
#include <common/utils.h>

TEST(Encoding, CanDetermineFileEncoding)
{
    EXPECT_EQ(getFileEncoding(u8"encoding/euc_kr.txt"_p), eFileEncoding::EUC_KR);
    EXPECT_EQ(getFileEncoding(u8"encoding/sjis.txt"_p), eFileEncoding::SHIFT_JIS);
    EXPECT_EQ(getFileEncoding(u8"encoding/utf8.txt"_p), eFileEncoding::UTF8);
}

// Not about 'Encoding' per se but sure.
TEST(Encoding, CanOpenUtf8FilePath)
{
    std::ifstream ifs(u8"encoding/クールネーム.txt"_p);
    ASSERT_FALSE(ifs.fail());
    std::string contents{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
    EXPECT_EQ(lunaticvibes::trim(contents), "it's so cool");
}
