// NOTE: when working with BMS file hashes, make sure to check them out with CRLF endings.
// =======================================================================================

#include <common/chartformat/chartformat_bms.h>
#include <common/encoding.h>
#include <common/hash.h>
#include <common/types.h>
#include <common/utils.h>

#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <gtest/gtest.h>

using lunaticvibes::parser_bms::JudgeDifficulty;

static bool ExpectNotePosition(const ChartFormatBMS& bms, LaneCode area, int ch, int bar, int res,
                               const std::vector<int>& segments)
{
    const auto& lane = bms.getLane(area, ch, bar);
    unsigned maxres = lane.resolution * res;

    auto it1 = lane.notes.begin();
    auto it2 = segments.begin();
    for (size_t i = 0; i < segments.size(); ++i, ++it1, ++it2)
    {
        if ((maxres * it1->segment / lane.resolution) != (*it2 * (maxres / res)))
            return false;
    }
    return true;
};
TEST(tBMS, FolderNotExist)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bbb/asdlfkjasdlfkjsdalgjsdalgjasd.bms"));
    EXPECT_EQ(bms->isLoaded(), false);
}

TEST(tBMS, FileNotExist)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/asdlfkjasdlfkjsdalgjsdalgjasd.bms"));
    EXPECT_EQ(bms->isLoaded(), false);
}

TEST(tBMS, Utf8FilePath)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>(PathFromUTF8("bms/5k_世界.bms")));
    ASSERT_EQ(bms->isLoaded(), true);
    EXPECT_EQ(bms->title, "ザ・ワールド");
}

TEST(tBMS, MetaBasic)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/bgm32.bms"));
    ASSERT_EQ(bms->isLoaded(), true);

    EXPECT_EQ(bms->fileHash.hexdigest(), "c396f76b2de46425ed22b196230173f0");
    EXPECT_EQ(bms->haveNote, false);
    EXPECT_EQ(bms->notes_total, 0);
    EXPECT_EQ(bms->notes_key_ln, 0);
    EXPECT_EQ(bms->raw_rank, -1);
    EXPECT_EQ(bms->rank, std::nullopt);
}

TEST(tBMS, MetaReadme)
{
    std::stringstream ss;
    ChartFormatBMS bms{ss, eFileEncoding::UTF8, 0};
    bms.fileName = "bms/__whatever.bms";
    bms.absolutePath = u8"bms/__whatever.bms"_p;
    ASSERT_EQ(bms.isLoaded(), true);

    EXPECT_TRUE(bms.checkHasReadme());
    static const std::vector<std::pair<std::string, std::string>> expected_readme_files{
        {"readme_euckr.crlf.txt", "안녕\n불고기\n"},
        {"readme_sjis.crlf.txt", "桃太郎は桃を食べた。\nと少年が思った、\n"},
        {"readme_utf8.crlf.txt", "Матрёшка.\nВодка.\n"},
    };
    std::vector<std::pair<std::string, std::string>> files;
    EXPECT_EQ((files = bms.getReadmeFiles()), expected_readme_files);
}

TEST(tBMS, MetaNoFile)
{
    ChartFormatBMSMeta bms;
    bms.fileName = "bms.bms";
    bms.absolutePath = "/does/not/exist";

    EXPECT_FALSE(bms.checkHasReadme());
    EXPECT_TRUE(bms.getReadmeFiles().empty());
}

TEST(tBMS, RandomKeyCount)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    // Intentionally random seed. Who knows if default '0' would always trigger what was the #IF bug.
    const auto seed = std::random_device{}();
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/random_5k_7k.bme", seed));
    ASSERT_EQ(bms->isLoaded(), true);
    EXPECT_EQ(bms->gamemode, 7);
}

TEST(tBMS, RankInvalidParsedCorrectly)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/rank_invalid.bms"));
    ASSERT_EQ(bms->isLoaded(), true);
    EXPECT_EQ(bms->fileHash.hexdigest(), "7f119b50d038161cfc96767edeea76ec");
    EXPECT_EQ(bms->raw_rank, 9);
    EXPECT_EQ(bms->rank, std::nullopt);
}

TEST(tBMS, RankVeryEasyParsedCorrectly)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/rank_veryeasy.bms"));
    ASSERT_EQ(bms->isLoaded(), true);
    EXPECT_EQ(bms->fileHash.hexdigest(), "333c659406a1cb4312e76b447f5ee0cc");
    EXPECT_EQ(bms->raw_rank, 4);
    EXPECT_EQ(bms->rank, JudgeDifficulty::NORMAL);
}

TEST(tBMS, RankVeryHardParsedCorrectly)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/rank_hard.bms"));
    ASSERT_EQ(bms->isLoaded(), true);
    EXPECT_EQ(bms->fileHash.hexdigest(), "180d007b193e1367838eda5d5d10dd4f");
    EXPECT_EQ(bms->raw_rank, 1);
    EXPECT_EQ(bms->rank, JudgeDifficulty::HARD);
}

TEST(tBMS, MetreChange)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/bar.bms"));
    ASSERT_EQ(bms->isLoaded(), true);

    ASSERT_EQ(bms->lastBarIdx, 3);
    EXPECT_EQ(bms->metres[0], Metre(4, 4));
    EXPECT_EQ(bms->metres[1], Metre(8, 4));
    EXPECT_EQ(bms->metres[2], Metre(3, 4));
    EXPECT_EQ(bms->metres[3], Metre(4, 4));
}

TEST(tBMS, BpmChange)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/bpm.bms"));
    ASSERT_EQ(bms->isLoaded(), true);

    ASSERT_EQ(bms->lastBarIdx, 3);
    EXPECT_FLOAT_EQ(bms->bpm, 150.000000);
    EXPECT_TRUE(bms->haveBPMChange);
    EXPECT_FLOAT_EQ(bms->maxBPM, 300.000000);
    EXPECT_FLOAT_EQ(bms->minBPM, 0.100000);
    EXPECT_EQ(bms->getLane(bms::LaneCode::BPM, 0, 0).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::BPM, 0, 0).notes.begin()->value, 175);
    EXPECT_EQ(bms->getLane(bms::LaneCode::BPM, 0, 1).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::BPM, 0, 1).notes.begin()->value, 200);
    EXPECT_EQ(bms->getLane(bms::LaneCode::BPM, 0, 1).notes.rbegin()->value, 250);
    ASSERT_EQ(bms->getLane(bms::LaneCode::EXBPM, 0, 2).notes.size(), 2);
    EXPECT_FLOAT_EQ(bms->exBPM[bms->getLane(bms::LaneCode::EXBPM, 0, 2).notes.begin()->value], 300.0);
    EXPECT_FLOAT_EQ(bms->exBPM[bms->getLane(bms::LaneCode::EXBPM, 0, 2).notes.rbegin()->value], 0.1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::BPM, 0, 3).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::BPM, 0, 3).notes.begin()->value, 150);
}

TEST(tBMS, stop)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/stop.bms"));
    ASSERT_EQ(bms->isLoaded(), true);

    EXPECT_EQ(bms->getLane(bms::LaneCode::STOP, 0, 1).notes.size(), 3);
    auto it = bms->getLane(bms::LaneCode::STOP, 0, 1).notes.begin();
    EXPECT_EQ(bms->stop[it->value], 48);
    it++;
    EXPECT_EQ(bms->stop[it->value], 96);
    it++;
    EXPECT_EQ(bms->stop[it->value], 192);
}

TEST(tBMS, Note5k)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/5k.bms"));
    ASSERT_EQ(bms->isLoaded(), true);

    // note count
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 0, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 1, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 2, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 3, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 4, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 5, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 0, 2).notes.size(), 4);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 1, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 2, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 3, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 4, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 5, 2).notes.size(), 1);

    // note position
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 1, 1, 4, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 2, 1, 4, std::vector<int>{1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 3, 1, 4, std::vector<int>{2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 5, 2, 4, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 0, 2, 4, std::vector<int>{0, 1, 2, 3}));
}
TEST(tBMS, Note7k)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/7k.bme"));
    ASSERT_EQ(bms->isLoaded(), true);

    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 0, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 1, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 2, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 3, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 4, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 5, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 6, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 7, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 0, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 1, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 2, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 3, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 4, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 5, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 6, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 7, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 0, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 1, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 2, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 3, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 4, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 5, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 6, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 7, 1).notes.size(), 0);

    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 0, 1, 8, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 1, 1, 8, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 2, 1, 8, std::vector<int>{1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 3, 1, 8, std::vector<int>{2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 4, 1, 8, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 5, 1, 8, std::vector<int>{4}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 6, 1, 8, std::vector<int>{5}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 7, 1, 8, std::vector<int>{6}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 0, 2, 8, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 1, 2, 8, std::vector<int>{1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 2, 2, 8, std::vector<int>{2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 3, 2, 8, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 4, 2, 8, std::vector<int>{4}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 5, 2, 8, std::vector<int>{5}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 6, 2, 8, std::vector<int>{6}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 7, 2, 8, std::vector<int>{7}));
}
TEST(tBMS, Note9kMixedChannels)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/9k_mixed_channels.pms"));
    ASSERT_EQ(bms->isLoaded(), true);
    EXPECT_EQ(bms->fileHash, HashMD5{"646ed39249f11e2e579fd52a10ec174a"});
    EXPECT_EQ(bms->gamemode, 9);

    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 0, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 1, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 2, 1).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 3, 1).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 4, 1).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 5, 1).notes.size(), 4);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 6, 1).notes.size(), 3);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 7, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 8, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 9, 1).notes.size(), 0);

    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 0, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 1, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 2, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 3, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 4, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 5, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 6, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 7, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 8, 2).notes.size(), 0);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 9, 2).notes.size(), 0);
}
TEST(tBMS, Note10k)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/10k.bms"));
    ASSERT_EQ(bms->isLoaded(), true);

    // note count
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 0, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 1, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 2, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 3, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 4, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 5, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 1, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 2, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 3, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 4, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 5, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 0, 2).notes.size(), 1);

    // note position
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 0, 1, 8, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 1, 2, 8, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 2, 2, 8, std::vector<int>{1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 3, 2, 8, std::vector<int>{2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 4, 2, 8, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 5, 2, 8, std::vector<int>{4}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 1, 1, 8, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 2, 1, 8, std::vector<int>{1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 3, 1, 8, std::vector<int>{2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 4, 1, 8, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 5, 1, 8, std::vector<int>{4}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 0, 2, 8, std::vector<int>{0}));
}
TEST(tBMS, Note14k)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/14k.bme"));
    ASSERT_EQ(bms->isLoaded(), true);

    // note count
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 0, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 1, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 2, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 3, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 4, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 5, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 6, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE1, 7, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 1, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 2, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 3, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 4, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 5, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 6, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 7, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTE2, 0, 2).notes.size(), 1);

    // note position
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 0, 1, 8, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 1, 2, 8, std::vector<int>{1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 2, 2, 8, std::vector<int>{2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 3, 2, 8, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 4, 2, 8, std::vector<int>{4}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 5, 2, 8, std::vector<int>{5}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 6, 2, 8, std::vector<int>{6}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE1, 7, 2, 8, std::vector<int>{7}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 1, 1, 8, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 2, 1, 8, std::vector<int>{1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 3, 1, 8, std::vector<int>{2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 4, 1, 8, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 5, 1, 8, std::vector<int>{4}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 6, 1, 8, std::vector<int>{5}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 7, 1, 8, std::vector<int>{6}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTE2, 0, 2, 8, std::vector<int>{0}));
}

TEST(tBMS, ln)
{
    std::shared_ptr<ChartFormatBMS> bms = nullptr;
    ASSERT_NO_THROW(bms = std::make_shared<ChartFormatBMS>("bms/ln.bme"));
    ASSERT_EQ(bms->isLoaded(), true);

    // note count
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 1, 1).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 2, 1).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 3, 1).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 4, 1).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 4, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 5, 2).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 6, 2).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 7, 2).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 1, 2).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 1, 3).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 2, 3).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 3, 3).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 4, 3).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 5, 3).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 5, 4).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 6, 4).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 7, 4).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN1, 0, 4).notes.size(), 2);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 0, 4).notes.size(), 1);
    EXPECT_EQ(bms->getLane(bms::LaneCode::NOTELN2, 0, 5).notes.size(), 1);

    // note position
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 1, 1, 4, std::vector<int>{0, 1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 2, 1, 4, std::vector<int>{1, 2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 3, 1, 4, std::vector<int>{2, 3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 4, 1, 4, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 4, 2, 4, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 5, 2, 4, std::vector<int>{0, 1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 6, 2, 4, std::vector<int>{1, 2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 7, 2, 4, std::vector<int>{2, 3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 1, 2, 4, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 1, 3, 4, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 2, 3, 4, std::vector<int>{0, 1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 3, 3, 4, std::vector<int>{1, 2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 4, 3, 4, std::vector<int>{2, 3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 5, 3, 4, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 5, 4, 4, std::vector<int>{0}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 6, 4, 4, std::vector<int>{0, 1}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 7, 4, 4, std::vector<int>{1, 2}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN1, 0, 4, 4, std::vector<int>{2, 3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 0, 4, 4, std::vector<int>{3}));
    EXPECT_TRUE(ExpectNotePosition(*bms, bms::LaneCode::NOTELN2, 0, 5, 4, std::vector<int>{0}));
}

TEST(tBMS, ImplicitDifficulty)
{
    auto get_difficulty = [](auto&& text) {
        std::stringstream ss;
        ss << text;
        return ChartFormatBMS(ss, eFileEncoding::UTF8, 0).difficulty;
    };

    EXPECT_EQ(get_difficulty(""), 2);
    EXPECT_EQ(get_difficulty("#DIFFICULTY 4"), 4);
    EXPECT_EQ(get_difficulty("#DIFFICULTY 666"), 666); // ?
    EXPECT_EQ(get_difficulty("#DIFFICULTY bogus"), 0); // ?
    EXPECT_EQ(get_difficulty("#TITLE tesTeAsyTest"), 1);
    EXPECT_EQ(get_difficulty("#TITLE testNormalTest"), 2);
    EXPECT_EQ(get_difficulty("#TITLE testHARDtest"), 3);
    EXPECT_EQ(get_difficulty("#TITLE testINSANEtest"), 4);
    EXPECT_EQ(get_difficulty("#TITLE testINSANEtest\n#DIFFICULTY 1"), 1);
    EXPECT_EQ(get_difficulty("#SUBTITLE testINSANEtest"), 4);
    EXPECT_EQ(get_difficulty("#TITLE test [InSaNe]"), 4);
}

TEST(tBMS, RandomInHeader)
{
    ChartFormatBMS bms("bms/random_in_header.bms");
    ASSERT_EQ(bms.isLoaded(), true);

    EXPECT_TRUE(bms.haveRandom);
    EXPECT_EQ(bms.notes_total, 1);
}
