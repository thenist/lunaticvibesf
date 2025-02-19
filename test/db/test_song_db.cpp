#include "common/hash.h"
#include <gtest/gtest.h>

#include <db/db_song.h>

static constexpr auto&& IN_MEMORY_DB_PATH = ":memory:";

TEST(SongDb, SongInserting)
{
    SongDB song_db{IN_MEMORY_DB_PATH};

    static const HashMD5 chart_hash{"4257da068c0c860e8556100f07fb94bf"};

    // Insert.
    {
        song_db.addSubFolder("bms");
        song_db.waitLoadingFinish();
        song_db.prepareCache();
        const auto charts = song_db.findChartByHash(chart_hash, false);
        ASSERT_EQ(charts.size(), 1);
        const auto& chart = charts[0];
        EXPECT_EQ(chart->fileName, "10k.bms");
    }

    // Refresh.
    {
        song_db.addSubFolder("bms");
        song_db.waitLoadingFinish();
        song_db.prepareCache();
        const auto charts = song_db.findChartByHash(chart_hash, true);
        ASSERT_EQ(charts.size(), 1);
        const auto& chart = charts[0];
        EXPECT_EQ(chart->fileName, "10k.bms");
    }
}
