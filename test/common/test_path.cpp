#include <gtest/gtest.h>

#include <common/types.h>
#include <common/u8.h>
#include <common/utils.h>

TEST(Paths, ResolvesLr2PathsCorrectlty)
{
    auto conv = [](const std::string& a, const Path& b) { return lunaticvibes::u8str(convertLR2Path(a, b)); };

// fs::path::is_absolute only works for native paths, tested on libstdc++ with GCC 13.2.1 and some MSVC version.
#ifdef _WIN32
    EXPECT_EQ(conv("D:\\Games\\LunaticVibes", "C:\\something\\somewhere"), "C:\\something\\somewhere");
    EXPECT_EQ(conv("D:\\Games\\LunaticVibes", R"(D:\Games\LunaticVibes\LR2files\Config\config.xml)"),
              R"(D:\Games\LunaticVibes\LR2files\Config\config.xml)");
#else
    EXPECT_EQ(conv("/home/me/lv", "/etc/passwd"), "/etc/passwd");
    EXPECT_EQ(conv("/opt/lv", "/opt/lv/LR2files/Config/config.xml"), "/opt/lv/LR2files/Config/config.xml");
#endif // _WIN32

    EXPECT_EQ(conv("/home/me/lv", u8" テスト "), " テスト ");

    EXPECT_EQ(conv("/home/me/lv", ""), "");
    EXPECT_EQ(conv("/home/me/lv", "."), ".");
    EXPECT_EQ(conv("/home/me/lv", " "), " ");
    EXPECT_EQ(conv("/home/me/lv", "  "), "  ");
    EXPECT_EQ(conv("D:\\Games\\LunaticVibes", ""), "");
    EXPECT_EQ(conv("D:\\Games\\LunaticVibes", "."), ".");
    EXPECT_EQ(conv("D:\\Games\\LunaticVibes", " "), " ");
    EXPECT_EQ(conv("D:\\Games\\LunaticVibes", "  "), "  ");

    EXPECT_EQ(conv("/home/me/lv", "\" "), " ");
    EXPECT_EQ(conv("/home/me/lv", " \""), " ");
    EXPECT_EQ(conv("/home/me/lv", R"(""""""""""")"), "");
#ifdef _WIN32
    EXPECT_EQ(conv("D:\\Games\\LunaticVibes", R"(.\LR2files\Theme\EndlessCirculation\Play\parts\Combo\*.png""")"),
              R"(D:\Games\LunaticVibes\LR2files\Theme\EndlessCirculation\Play\parts\Combo\*.png)");
#else
    EXPECT_EQ(conv("/home/me/lv", R"(.\LR2files\Theme\EndlessCirculation\Play\parts\Combo\*.png""")"),
              "/home/me/lv/LR2files/Theme/EndlessCirculation/Play/parts/Combo/*.png");
#endif // _WIN32
}
