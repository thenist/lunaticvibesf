#include <gtest/gtest.h>

#include <random>
#include <vector>

#include <common/str_utils.h>
#include <game/sound/soundset_lr2.h>

TEST(Lr2SoundSet, HeaderInformationParsing)
{
    {
        SoundSetLR2 ss;
        EXPECT_TRUE(ss.parseHeader(
            {"#INFORMATION", "10", "SoundSetName", "SoundSetMaker", "LR2files\\Sound\\SoundSetThumbnail.png"}));
        EXPECT_EQ(ss.maker, "SoundSetMaker");
        EXPECT_EQ(ss.name, "SoundSetName");
        EXPECT_EQ(ss.getThumbnailPath(), u8"./LR2files/Sound/SoundSetThumbnail.png"_p);
    }
    EXPECT_FALSE(SoundSetLR2{}.parseHeader({}));
    EXPECT_FALSE(SoundSetLR2{}.parseHeader({"#INFORMATION"}));
    EXPECT_FALSE(SoundSetLR2{}.parseHeader({"#INFORMATION", ""}));
    EXPECT_FALSE(SoundSetLR2{}.parseHeader({"#INFORMATION", "11", ""}));
    EXPECT_FALSE(SoundSetLR2{}.parseHeader({"#INFORMATION", "", ""}));
    EXPECT_FALSE(SoundSetLR2{}.parseHeader({"#INFORMATION", "", "", "", "", "", ""}));
}

TEST(Lr2SoundSet, HeaderCustomFileParsing)
{
    // Not crashing on missing fields and bad values.
    EXPECT_FALSE(SoundSetLR2{}.parseHeader({"#CUSTOMFILE", ""}));
    EXPECT_FALSE(SoundSetLR2{}.parseHeader({"#CUSTOMFILE", "", ""}));
    EXPECT_TRUE(SoundSetLR2{}.parseHeader({"#CUSTOMFILE", "", "", ""}));
    EXPECT_TRUE(SoundSetLR2{}.parseHeader({"#CUSTOMFILE", "", "", "", ""}));
    EXPECT_TRUE(SoundSetLR2{}.parseHeader({"#CUSTOMFILE", "", "", "", "", ""}));
}

TEST(Lr2SoundSet, BodyParsing)
{
    // Not crashing on missing fields and bad values.
    EXPECT_FALSE(SoundSetLR2{}.parseBody({}));
    EXPECT_FALSE(SoundSetLR2{}.parseBody({"#SELECT"}));
    EXPECT_TRUE(SoundSetLR2{}.parseBody({"#SELECT", "somepath"}));
    EXPECT_TRUE(SoundSetLR2{}.parseBody({"#SELECT", "somepath", ""}));
}

TEST(Lr2SoundSet, StaticOptionValues)
{
    SoundSetLR2 ss;

    // Values mostly from default LR2 soundset.
    EXPECT_TRUE(ss.parseHeader({"#INFORMATION", "10", "", "", ""}));
    EXPECT_TRUE(ss.parseBody({"#SELECT", R"(LR2files\Sound\lol\select.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#DECIDE", R"(LR2files\Sound\lol\decide.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#EXSELECT", R"(LR2files\Sound\lol\exselect.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#EXDECIDE", R"(LR2files\Sound\lol\exdecide.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#FOLDER_OPEN", R"(LR2files\Sound\lr2\f-open.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#FOLDER_CLOSE", R"(LR2files\Sound\lr2\f-close.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#PANEL_OPEN", R"(LR2files\Sound\lr2\o-open.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#PANEL_CLOSE", R"(LR2files\Sound\lr2\o-close.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#OPTION_CHANGE", R"(LR2files\Sound\lr2\o-change.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#DIFFICULTY", R"(LR2files\Sound\lr2\difficulty.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#SCREENSHOT", R"(LR2files\Sound\lr2\screenshot.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#CLEAR", R"(LR2files\Sound\lr2\clear.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#FAIL", R"(LR2files\Sound\lr2\fail.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#STOP", R"(LR2files\Sound\lr2\playstop.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#MINE", R"(LR2files\Sound\lr2\mine.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#SCRATCH", R"(LR2files\Sound\lr2\scratch.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#COURSECLEAR", R"(LR2files\Sound\lr2\course_clear.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#COURSEFAIL", R"(LR2files\Sound\lr2\course_fail.wav)", ""}));
    // Only first values should be used.
    // This is without a leading hash sign in default LR2 sound set, but some sound sets like Seraphic's add it.
    EXPECT_FALSE(ss.parseBody({"SELECT", R"(LR2files\Bgm\LR2 ver sta\select.wav)"}));
    EXPECT_FALSE(ss.parseBody({"DECIDE", R"(LR2files\Bgm\LR2 ver sta\decide.wav)"}));
    EXPECT_FALSE(ss.parseBody({"EXSELECT", R"(LR2files\Bgm\LR2 ver sta\exselect.wav)"}));
    EXPECT_FALSE(ss.parseBody({"EXDECIDE", R"(LR2files\Bgm\LR2 ver sta\exdecide.wav)"}));
    EXPECT_FALSE(ss.parseBody({"#SELECT", R"(LR2files\Bgm\asdf\select.wav)"}));
    EXPECT_FALSE(ss.parseBody({"#DECIDE", R"(LR2files\Bgm\asdf\decide.wav)"}));
    EXPECT_FALSE(ss.parseBody({"#FOLDER_OPEN", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#FOLDER_CLOSE", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#PANEL_OPEN", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#PANEL_CLOSE", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#OPTION_CHANGE", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#DIFFICULTY", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#SCREENSHOT", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#CLEAR", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#FAIL", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#STOP", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#MINE", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#SCRATCH", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#COURSECLEAR", R"(LR2files\Sound\lol.wav)", ""}));
    EXPECT_FALSE(ss.parseBody({"#COURSEFAIL", R"(LR2files\Sound\lol.wav)", ""}));

    EXPECT_EQ(ss.getPathBGMSelect(), u8"./LR2files/Sound/lol/select.wav"_p);
    EXPECT_EQ(ss.getPathBGMDecide(), u8"./LR2files/Sound/lol/decide.wav"_p);

    EXPECT_EQ(ss.getPathSoundOpenFolder(), u8"./LR2files/Sound/lr2/f-open.wav"_p);
    EXPECT_EQ(ss.getPathSoundCloseFolder(), u8"./LR2files/Sound/lr2/f-close.wav"_p);
    EXPECT_EQ(ss.getPathSoundOpenPanel(), u8"./LR2files/Sound/lr2/o-open.wav"_p);
    EXPECT_EQ(ss.getPathSoundClosePanel(), u8"./LR2files/Sound/lr2/o-close.wav"_p);
    EXPECT_EQ(ss.getPathSoundOptionChange(), u8"./LR2files/Sound/lr2/o-change.wav"_p);
    EXPECT_EQ(ss.getPathSoundDifficultyChange(), u8"./LR2files/Sound/lr2/difficulty.wav"_p);
    EXPECT_EQ(ss.getPathSoundScreenshot(), u8"./LR2files/Sound/lr2/screenshot.wav"_p);
    EXPECT_EQ(ss.getPathBGMResultClear(), u8"./LR2files/Sound/lr2/clear.wav"_p);
    EXPECT_EQ(ss.getPathBGMResultFailed(), u8"./LR2files/Sound/lr2/fail.wav"_p);
    EXPECT_EQ(ss.getPathSoundFailed(), u8"./LR2files/Sound/lr2/playstop.wav"_p);
    EXPECT_EQ(ss.getPathSoundLandmine(), u8"./LR2files/Sound/lr2/mine.wav"_p);
    EXPECT_EQ(ss.getPathSoundScratch(), u8"./LR2files/Sound/lr2/scratch.wav"_p);
    EXPECT_EQ(ss.getPathBGMCourseClear(), u8"./LR2files/Sound/lr2/course_clear.wav"_p);
    EXPECT_EQ(ss.getPathBGMCourseFailed(), u8"./LR2files/Sound/lr2/course_fail.wav"_p);
}

TEST(Lr2SoundSet, CustomFile)
{
    // .1 and .2 for testing "EndlessCirculation PEN.2" and "EndlessCirculation PEN.3".
    // Two globs for testing "Undertale-Deltarune".
    SoundSetLR2 ss;
    EXPECT_TRUE(ss.parseHeader({"#INFORMATION", "10", "", "", ""}));
    EXPECT_TRUE(ss.parseHeader({"#CUSTOMFILE", "SELECT BGM", R"(lr2soundset\bgm\*)", "defaultnotdefault"}));
    EXPECT_TRUE(ss.setCustomFileOptionForBodyParsing("SELECT BGM", "mybgm.1"));
    EXPECT_FALSE(ss.setCustomFileOptionForBodyParsing("SELECT BGM", "not a thing"));
    EXPECT_FALSE(ss.setCustomFileOptionForBodyParsing("NOSUCHOPTION", "something"));
    EXPECT_TRUE(ss.parseBody({"#SELECT", R"(lr2soundset\bgm\*\*.wav)", ""}));
    ASSERT_EQ(ss.getCustomizeOptionCount(), 1);
    {
        auto opt = ss.getCustomizeOptionInfo(0);
        EXPECT_EQ(opt.id, 0);
        EXPECT_EQ(opt.displayName, "SELECT BGM");
        EXPECT_EQ(opt.internalName, "FILE_SELECT BGM");
        EXPECT_EQ(opt.defaultEntry, 0);
        EXPECT_EQ(lunaticvibes::join(',', opt.entries), "mybgm.1,mybgm.2,RANDOM");
    }
    EXPECT_EQ(ss.getPathBGMSelect(), u8"lr2soundset/bgm/mybgm.1/mybgm.1.wav"_p);
}

TEST(Lr2SoundSet, CustomFileDefaultEntry)
{
    SoundSetLR2 ss;
    EXPECT_TRUE(ss.parseHeader({"#INFORMATION", "10", "", "", ""}));
    EXPECT_TRUE(ss.parseHeader({"#CUSTOMFILE", "SELECT BGM", R"(lr2soundset\bgm\*)", "mybgm.2"}));
    EXPECT_FALSE(ss.setCustomFileOptionForBodyParsing("SELECT BGM", "not a thing"));
    EXPECT_TRUE(ss.parseBody({"#SELECT", R"(lr2soundset\bgm\*\*.wav)", ""}));
    ASSERT_EQ(ss.getCustomizeOptionCount(), 1);
    {
        auto opt = ss.getCustomizeOptionInfo(0);
        EXPECT_EQ(opt.id, 0);
        EXPECT_EQ(opt.displayName, "SELECT BGM");
        EXPECT_EQ(opt.internalName, "FILE_SELECT BGM");
        EXPECT_EQ(opt.defaultEntry, 1);
        EXPECT_EQ(lunaticvibes::join(',', opt.entries), "mybgm.1,mybgm.2,RANDOM");
    }
    EXPECT_EQ(ss.getPathBGMSelect(), u8"lr2soundset/bgm/mybgm.2/mybgm.2.wav"_p);
}

TEST(Lr2SoundSet, CustomFileRandomOption)
{
    SoundSetLR2 ss{std::mt19937{1337}};
    EXPECT_TRUE(ss.parseHeader({"#INFORMATION", "10", "", "", ""}));
    EXPECT_TRUE(ss.parseHeader({"#CUSTOMFILE", "SELECT BGM", R"(lr2soundset\bgm\*)", "mybgm.2"}));
    EXPECT_TRUE(ss.setCustomFileOptionForBodyParsing("SELECT BGM", "RANDOM"));
    EXPECT_TRUE(ss.parseBody({"#SELECT", R"(lr2soundset\bgm\*\*.wav)", ""}));
    ASSERT_EQ(ss.getCustomizeOptionCount(), 1);
    {
        auto opt = ss.getCustomizeOptionInfo(0);
        EXPECT_EQ(opt.id, 0);
        EXPECT_EQ(opt.displayName, "SELECT BGM");
        EXPECT_EQ(opt.internalName, "FILE_SELECT BGM");
        EXPECT_EQ(opt.defaultEntry, 1);
        EXPECT_EQ(lunaticvibes::join(',', opt.entries), "mybgm.1,mybgm.2,RANDOM");
    }
    EXPECT_EQ(ss.getPathBGMSelect(), u8"lr2soundset/bgm/mybgm.1/mybgm.1.wav"_p);
}

TEST(Lr2SoundSet, CustomFileRandomOptionNoFiles)
{
    SoundSetLR2 ss;
    EXPECT_TRUE(ss.parseHeader({"#INFORMATION", "10", "", "", ""}));
    EXPECT_TRUE(ss.parseHeader({"#CUSTOMFILE", "SELECT BGM", R"(no\such\path\*)", "RANDOM"}));
    EXPECT_TRUE(ss.setCustomFileOptionForBodyParsing("SELECT BGM", "RANDOM"));
    EXPECT_TRUE(ss.parseBody({"#SELECT", R"(no\such\path\*.wav)", ""}));
    ASSERT_EQ(ss.getCustomizeOptionCount(), 1);
    {
        auto opt = ss.getCustomizeOptionInfo(0);
        EXPECT_EQ(opt.id, 0);
        EXPECT_EQ(opt.displayName, "SELECT BGM");
        EXPECT_EQ(opt.internalName, "FILE_SELECT BGM");
        EXPECT_EQ(opt.defaultEntry, 0);
        EXPECT_EQ(lunaticvibes::join(',', opt.entries), "RANDOM");
    }
    EXPECT_EQ(ss.getPathBGMSelect(), u8""_p);
}

TEST(Lr2SoundSet, MissingCustomFileRandomFallbackNotFound)
{
    SoundSetLR2 ss;
    EXPECT_TRUE(ss.parseHeader({"#INFORMATION", "10", "", "", ""}));
    EXPECT_TRUE(ss.parseBody({"#SELECT", R"(no\such\path\*.wav)", ""}));
    ASSERT_EQ(ss.getCustomizeOptionCount(), 0);
    EXPECT_EQ(ss.getPathBGMSelect(), u8""_p);
}

TEST(Lr2SoundSet, MissingCustomFileRandomFallback)
{
    // Commented out lines is what should ideally be in this test, but using several globs in fallback is not
    // implemented.
    SoundSetLR2 ss{std::mt19937{1337}};
    EXPECT_TRUE(ss.parseHeader({"#INFORMATION", "10", "", "", ""}));
    // EXPECT_TRUE(ss.parseBody({"#SELECT", R"(lr2soundset\bgm\*\*.wav)", ""}));
    EXPECT_TRUE(ss.parseBody({"#SELECT", R"(lr2soundset\bgm\*)", ""}));
    ASSERT_EQ(ss.getCustomizeOptionCount(), 0);
    // EXPECT_EQ(ss.getPathBGMSelect(), "lr2soundset/bgm/mybgm.1/mybgm.1.wav"_p);
    EXPECT_EQ(ss.getPathBGMSelect(), u8"lr2soundset/bgm/mybgm.1"_p);
}
