#include <common/encoding.h>
#include <common/types.h>
#include <common/utils.h>

#include <gtest/gtest.h>

#include <fstream>

TEST(Encoding, CanDetermineFileEncoding)
{
    auto getFileEncoding = [](const Path& path) -> eFileEncoding {
        std::ifstream fs(path);
        if (fs.fail())
            return eFileEncoding::LATIN1;
        return lunaticvibes::getFileEncoding(fs);
    };

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

TEST(Encoding, Utf8ToUtf32)
{
    auto utf8_to_utf32 = [](const std::string& str) {
        std::u32string out;
        lunaticvibes::utf8_to_utf32(str, out);
        return out;
    };
    EXPECT_EQ(utf8_to_utf32("aA\nあ"), U"aA\nあ");
}

TEST(Encoding, ShiftJisToUtf8)
{
    std::string out;
    lunaticvibes::to_utf8("\x82\xa0", eFileEncoding::SHIFT_JIS, out);
    EXPECT_EQ(out, "\xe3\x81\x82");
}
