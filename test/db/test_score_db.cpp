#include <gtest/gtest.h>

#include <db/db_lr2_score.h>
#include <db/db_score.h>

static constexpr auto&& IN_MEMORY_DB_PATH = ":memory:";

TEST(ScoreDb, ChartScoreUpdating)
{
    static const HashMD5 hash = md5("deadbeef");

    ScoreDB score_db{IN_MEMORY_DB_PATH};

    EXPECT_EQ(score_db.getChartScoreBMS(hash), nullptr);
    EXPECT_EQ(score_db.getStats().pgreat, 0);

    ScoreBMS score1;
    score1.exscore = 16;
    score1.lamp = ScoreBMS::Lamp::NOPLAY;
    score1.pgreat = 5;
    score1.great = 12;
    score1.good = 32;
    score1.bad = 69;
    score1.kpoor = 84;
    score1.miss = 58;
    score1.bp = 1469;
    score1.combobreak = 1385;
    score1.replayFileName = "score1";
    {
        score_db.insertChartScoreBMS(hash, score1);
        const auto pb = score_db.getChartScoreBMS(hash);
        ASSERT_NE(pb, nullptr);
        EXPECT_EQ(pb->exscore, score1.exscore);
        EXPECT_EQ(pb->lamp, score1.lamp);
        EXPECT_EQ(pb->pgreat, score1.pgreat);
        EXPECT_EQ(pb->great, score1.great);
        EXPECT_EQ(pb->good, score1.good);
        EXPECT_EQ(pb->bad, score1.bad);
        EXPECT_EQ(pb->kpoor, score1.kpoor);
        EXPECT_EQ(pb->miss, score1.miss);
        EXPECT_EQ(pb->bp, score1.bp);
        EXPECT_EQ(pb->combobreak, score1.combobreak);
        EXPECT_EQ(pb->replayFileName, score1.replayFileName);
        const auto stats = score_db.getStats();
        EXPECT_EQ(stats.pgreat, 5);
        EXPECT_EQ(stats.play_count, 1);
        EXPECT_EQ(stats.clear_count, 1);
    }

    ScoreBMS score2;
    score2.final_combo = 5;
    score2.play_time = lunaticvibes::Time{12340};
    score2.exscore = 17;
    score2.lamp = ScoreBMS::Lamp::FAILED;
    score2.pgreat = 6;
    score2.great = 11;
    score2.good = 32;
    score2.bad = 69;
    score2.kpoor = 84;
    score2.miss = 58;
    score2.bp = 1469;
    score2.combobreak = 1385;
    score2.replayFileName = "score2";
    {
        score_db.insertChartScoreBMS(hash, score2);
        const auto pb = score_db.getChartScoreBMS(hash);
        ASSERT_NE(pb, nullptr);
        EXPECT_EQ(pb->exscore, score2.exscore);
        EXPECT_EQ(pb->lamp, score2.lamp);
        EXPECT_EQ(pb->pgreat, score2.pgreat);
        EXPECT_EQ(pb->great, score2.great);
        EXPECT_EQ(pb->good, score2.good);
        EXPECT_EQ(pb->bad, score2.bad);
        EXPECT_EQ(pb->kpoor, score2.kpoor);
        EXPECT_EQ(pb->miss, score2.miss);
        EXPECT_EQ(pb->bp, score2.bp);
        EXPECT_EQ(pb->combobreak, score2.combobreak);
        EXPECT_EQ(pb->replayFileName, score2.replayFileName);
        const auto stats = score_db.getStats();
        EXPECT_EQ(stats.pgreat, score1.pgreat + score2.pgreat);
        EXPECT_EQ(stats.great, score1.great + score2.great);
        EXPECT_EQ(stats.good, score1.good + score2.good);
        EXPECT_EQ(stats.bad, score1.bad + score2.bad);
        EXPECT_EQ(stats.poor, score1.kpoor + score1.miss + score2.kpoor + score2.miss);
        EXPECT_EQ(stats.play_count, 2);
        EXPECT_EQ(stats.clear_count, 1);
        EXPECT_EQ(stats.running_combo, 5);
        EXPECT_EQ(stats.max_running_combo, 5);
        EXPECT_EQ(stats.playtime, 12340);
    }

    ScoreBMS score3;
    score3.exscore = 1;
    score3.lamp = ScoreBMS::Lamp::EASY;
    score3.pgreat = 2;
    score3.replayFileName = "score3";
    {
        score_db.insertChartScoreBMS(hash, score3);
        const auto pb = score_db.getChartScoreBMS(hash);
        ASSERT_NE(pb, nullptr);
        EXPECT_EQ(pb->exscore, score2.exscore);
        const auto stats = score_db.getStats();
        EXPECT_EQ(stats.pgreat, score1.pgreat + score2.pgreat + score3.pgreat);
        EXPECT_EQ(stats.play_count, 3);
        EXPECT_EQ(stats.clear_count, 2);
    }

    // Cache reloading works.
    {
        score_db.preloadScore();
        const auto pb = score_db.getChartScoreBMS(hash);
        ASSERT_NE(pb, nullptr);
        EXPECT_EQ(pb->exscore, 17);
    }
}

TEST(ScoreDb, CourseScoreUpdating)
{
    static const HashMD5 hash = md5("deadbeef");

    ScoreDB score_db{IN_MEMORY_DB_PATH};
    ScoreBMS score;
    score.exscore = 1;
    score.lamp = ScoreBMS::Lamp::EASY;
    score.pgreat = 2;

    EXPECT_EQ(score_db.getCourseScoreBMS(hash), nullptr);
    EXPECT_EQ(score_db.getStats().pgreat, 0);

    score_db.updateCourseScoreBMS(hash, score);
    EXPECT_EQ(score_db.getCourseScoreBMS(hash)->exscore, 1);
    EXPECT_EQ(score_db.getStats().pgreat, 0);

    score.exscore = 2;
    score_db.updateCourseScoreBMS(hash, score);
    EXPECT_EQ(score_db.getCourseScoreBMS(hash)->exscore, 2);
    EXPECT_EQ(score_db.getStats().pgreat, 0);

    score.exscore = 1;
    score_db.updateCourseScoreBMS(hash, score);
    EXPECT_EQ(score_db.getCourseScoreBMS(hash)->exscore, 2);
    EXPECT_EQ(score_db.getStats().pgreat, 0);

    // Cache reloading works.
    score_db.preloadScore();
    ASSERT_NE(score_db.getCourseScoreBMS(hash), nullptr);
    EXPECT_EQ(score_db.getCourseScoreBMS(hash)->exscore, 2);
}

TEST(ScoreDb, ChartScoreDeleting)
{
    static const HashMD5 hash = md5("deadbeef");

    ScoreDB score_db{IN_MEMORY_DB_PATH};
    ScoreBMS score;
    score.exscore = 1;

    EXPECT_EQ(score_db.getChartScoreBMS(hash), nullptr);

    // And also test for old DB table. Same scores themselves are never supposed to be in both.
    score_db.updateLegacyChartScoreBMS(hash, score);
    score_db.insertChartScoreBMS(hash, score);
    ASSERT_NE(score_db.getChartScoreBMS(hash), nullptr);
    EXPECT_EQ(score_db.getChartScoreBMS(hash)->exscore, 1);

    score_db.deleteAllChartScoresBMS(hash);
    EXPECT_EQ(score_db.getChartScoreBMS(hash), nullptr);
    // Not just in-memory cache that was invalidated.
    EXPECT_EQ(score_db.fetchCachedPbBMS(hash), nullptr);

    // Not just cache that was invalidated.
    score_db.rebuildBmsPbCache();
    EXPECT_EQ(score_db.getChartScoreBMS(hash), nullptr);
}

TEST(ScoreDb, CourseScoreDeleting)
{
    static const HashMD5 hash = md5("deadbeef");

    ScoreDB score_db{IN_MEMORY_DB_PATH};
    ScoreBMS score;
    score.exscore = 1;

    EXPECT_EQ(score_db.getCourseScoreBMS(hash), nullptr);

    score_db.updateCourseScoreBMS(hash, score);
    EXPECT_NE(score_db.getCourseScoreBMS(hash), nullptr);
    EXPECT_EQ(score_db.getCourseScoreBMS(hash)->exscore, 1);

    score_db.deleteCourseScoreBMS(hash);
    EXPECT_EQ(score_db.getCourseScoreBMS(hash), nullptr);
    // Not just in-memory cache that was invalidated.
    EXPECT_EQ(score_db.fetchCachedPbBMS(hash), nullptr);

    // Not just cache that was invalidated.
    score_db.rebuildBmsPbCache();
    EXPECT_EQ(score_db.getChartScoreBMS(hash), nullptr);
}

TEST(ScoreDb, ImportFromLr2)
{
    {
        lunaticvibes::Lr2ScoreDb _("/does/not/exist");
    }

    lunaticvibes::Lr2ScoreDb lr2_db(lunaticvibes::Lr2ScoreDb::InMemoryRwTag{});
    ScoreDB score_db{IN_MEMORY_DB_PATH};
    score_db.importScores(lr2_db);
    auto pbp = score_db.fetchCachedPbBMS(HashMD5{"deadbeefdeadbeefdeadbeefdeadbeef"});
    EXPECT_NE(pbp, nullptr);

    const auto& pb = *pbp;

    // struct ScoreBase values.
    EXPECT_EQ(pb.notes, 992);
    EXPECT_EQ(pb.score, 0);
    EXPECT_DOUBLE_EQ(pb.rate, static_cast<double>(386 * 2 + 336) / (992 * 2) * 100);
    EXPECT_EQ(pb.fast, 0);
    EXPECT_EQ(pb.slow, 0);
    EXPECT_EQ(pb.first_max_combo, 0);
    EXPECT_EQ(pb.final_combo, 0);
    EXPECT_EQ(pb.maxcombo, 172);
    EXPECT_EQ(pb.addtime, 0);
    EXPECT_EQ(pb.playcount, 0);
    EXPECT_EQ(pb.clearcount, 0);
    // EXPECT_EQ(pb.reserved[1], {0});
    // EXPECT_EQ(pb.reservedlf[2], {0.0});
    EXPECT_EQ(pb.replayFileName, "");

    // struct ScoreBMS values.
    EXPECT_EQ(pb.play_time, 0);
    EXPECT_EQ(pb.exscore, 386 * 2 + 336);
    EXPECT_EQ(pb.lamp, ScoreBMS::Lamp::FAILED);
    EXPECT_EQ(pb.pgreat, 386);
    EXPECT_EQ(pb.great, 336);
    EXPECT_EQ(pb.good, 60);
    EXPECT_EQ(pb.bad, 33);
    EXPECT_EQ(pb.kpoor, 64);
    EXPECT_EQ(pb.miss, 0);
    EXPECT_EQ(pb.bp, 262);
    EXPECT_EQ(pb.combobreak, 0);
    EXPECT_EQ(pb.rival_win, 3);
    EXPECT_EQ(pb.rival_rate, 0);
    EXPECT_EQ(pb.rival_lamp, ScoreBMS::Lamp::NOPLAY);
}

TEST(ScoreDb, ImportDanFromLr2)
{
    GTEST_SKIP() << "TODO: ImportDanFromLr2";
}
