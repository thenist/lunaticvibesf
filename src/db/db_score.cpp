#include "db_score.h"

#include <common/assert.h>
#include <common/hash.h>
#include <common/log.h>
#include <common/types.h>
#include <db/db_conn.h>
#include <db/db_lr2_score.h>

#include <algorithm>
#include <any>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <stdint.h>

static bool convert_score_bms(ScoreBMS& out, const std::vector<std::any>& in)
{
    static constexpr size_t SCORE_BMS_PARAM_COUNT = 21;
    if (in.size() < SCORE_BMS_PARAM_COUNT)
        return false;

    // md5 = ANY_STR(in.at(0));
    out.notes = ANY_INT(in.at(1));
    out.score = ANY_INT(in.at(2));
    out.rate = ANY_REAL(in.at(3));
    out.fast = ANY_INT(in.at(4));
    out.slow = ANY_INT(in.at(5));
    out.maxcombo = ANY_INT(in.at(6));
    out.addtime = ANY_INT(in.at(7));
    out.playcount = ANY_INT(in.at(8));
    out.clearcount = ANY_INT(in.at(9));
    out.exscore = ANY_INT(in.at(10));
    out.lamp = static_cast<ScoreBMS::Lamp>(ANY_INT(in.at(11)));
    out.pgreat = ANY_INT(in.at(12));
    out.great = ANY_INT(in.at(13));
    out.good = ANY_INT(in.at(14));
    out.bad = ANY_INT(in.at(15));
    out.kpoor = ANY_INT(in.at(16));
    out.miss = ANY_INT(in.at(17));
    out.bp = ANY_INT(in.at(18));
    out.combobreak = ANY_INT(in.at(19));
    out.replayFileName = ANY_STR(in.at(20));

    out.play_time = lunaticvibes::Time{0};
    return true;
}

ScoreDB::ScoreDB(const char* path) : SQLite(path, "SCORE")
{
    initTables();
}

void ScoreDB::deleteLegacyScoreBMS(const char* tableName, const HashMD5& hash)
{
    int ret;

    const auto hashStr = hash.hexdigest();

    char sqlbuf[128] = {0};
    snprintf(static_cast<char*>(sqlbuf), sizeof(sqlbuf) - 1, "DELETE FROM %s WHERE md5=?", tableName);
    ret = exec(static_cast<char*>(sqlbuf), {hashStr});
    if (ret != SQLITE_OK)
    {
        lunaticvibes::verify_failed(("[ScoreDB] Delete legacy score: " + std::string{errmsg()}).c_str());
        return;
    }

    cache[tableName].erase(hashStr);
}

std::shared_ptr<ScoreBMS> ScoreDB::getScoreBMS(const char* tableName, const HashMD5& hash) const
{
    return cache[tableName][hash.hexdigest()];
}

void ScoreDB::updateLegacyScoreBMS(const char* tableName, const HashMD5& hash, const ScoreBMS& score)
{
    int ret;

    const std::string hashStr = hash.hexdigest();

    auto pRecord = getScoreBMS(tableName, hash);
    if (pRecord)
    {
        auto record = *pRecord;

        if (score.exscore > record.exscore)
        {
            record.rate = score.rate;
            record.fast = score.fast;
            record.slow = score.slow;
            record.exscore = score.exscore;
            record.pgreat = score.pgreat;
            record.great = score.great;
            record.good = score.good;
            record.bad = score.bad;
            record.kpoor = score.kpoor;
            record.miss = score.miss;
            record.combobreak = score.combobreak;
            record.replayFileName = score.replayFileName;
        }
        else if (score.exscore == record.exscore)
        {
            if (score.maxcombo > record.maxcombo || score.bp < record.bp ||
                static_cast<int>(score.lamp) > static_cast<int>(record.lamp))
                record.replayFileName = score.replayFileName;
        }

        record.bp = std::min(record.bp, score.bp);
        record.clearcount = std::max(record.clearcount, score.clearcount);
        record.maxcombo = std::max(record.maxcombo, score.maxcombo);
        record.notes = score.notes;
        record.playcount = std::max(record.playcount, score.playcount);
        record.score = std::max(record.score, score.score);

        if (static_cast<int>(score.lamp) > static_cast<int>(record.lamp))
        {
            record.lamp = score.lamp;
        }

        char sqlbuf[224] = {0};
        sprintf(sqlbuf,
                "UPDATE %s SET "
                "notes=?,score=?,rate=?,fast=?,slow=?,maxcombo=?,addtime=?,pc=?,clearcount=?,exscore=?,lamp=?,"
                "pgreat=?,great=?,good=?,bad=?,bpoor=?,miss=?,bp=?,cb=?,replay=? WHERE md5=?",
                tableName);
        ret = exec(sqlbuf, {record.notes,
                            record.score,
                            record.rate,
                            record.fast,
                            record.slow,
                            record.maxcombo,
                            score.addtime,
                            record.playcount,
                            record.clearcount,
                            record.exscore,
                            static_cast<int>(record.lamp),
                            record.pgreat,
                            record.great,
                            record.good,
                            record.bad,
                            record.kpoor,
                            record.miss,
                            record.bp,
                            record.combobreak,
                            record.replayFileName,
                            hashStr});
    }
    else
    {
        char sqlbuf[224] = {0};
        sprintf(sqlbuf,
                "INSERT INTO %s(md5,notes,score,rate,fast,slow,maxcombo,addtime,pc,clearcount,exscore,lamp,"
                "pgreat,great,good,bad,bpoor,miss,bp,cb,replay) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                tableName);
        ret = exec(sqlbuf, {hashStr,          score.notes,      score.score,
                            score.rate,       score.fast,       score.slow,
                            score.maxcombo,   score.addtime,    score.playcount,
                            score.clearcount, score.exscore,    static_cast<int>(score.lamp),
                            score.pgreat,     score.great,      score.good,
                            score.bad,        score.kpoor,      score.miss,
                            score.bp,         score.combobreak, score.replayFileName});
    }
    if (ret != SQLITE_OK)
        lunaticvibes::verify_failed(("[ScoreDB] Upserting legacy score: " + std::string{errmsg()}).c_str());

    char sqlbuf[96] = {0};
    sprintf(sqlbuf, "SELECT * FROM %s WHERE md5=?", tableName);
    auto result = query(sqlbuf, {hashStr});
    LVF_DEBUG_ASSERT(!result.empty());
    const auto& r = result[0];
    auto scoreRefetched = std::make_shared<ScoreBMS>();
    convert_score_bms(*scoreRefetched, r);
    cache[tableName].insert_or_assign(hashStr, std::move(scoreRefetched));
}

void ScoreDB::deleteAllChartScoresBMS(const HashMD5& hash)
{
    deleteLegacyScoreBMS("score_bms", hash);

    const std::string hash_str = hash.hexdigest();

    int ret = exec("DELETE FROM score_history_bms WHERE md5=?", {hash_str});
    if (ret != SQLITE_OK)
        lunaticvibes::verify_failed(
            ("[ScoreDB] Delete score from score_history_bms: " + std::string{errmsg()}).c_str());

    ret = exec("DELETE FROM score_cache_bms WHERE md5=?", {hash_str});
    if (ret != SQLITE_OK)
        lunaticvibes::verify_failed(("[ScoreDB] Delete score from score_cache_bms: " + std::string{errmsg()}).c_str());

    cache["score_bms"].erase(hash_str);
}

std::shared_ptr<ScoreBMS> ScoreDB::getChartScoreBMS(const HashMD5& hash) const
{
    return getScoreBMS("score_bms", hash);
}

void ScoreDB::updateLegacyChartScoreBMS(const HashMD5& hash, const ScoreBMS& score)
{
    updateLegacyScoreBMS("score_bms", hash, score);
    rebuildBmsPbCache();
}

void ScoreDB::insertChartScoreBMS(const HashMD5& hash, const ScoreBMS& score)
{
    saveChartScoreBmsToHistory(hash, score);
    updateCachedChartPbBms(hash, score);
    updateStats(score);
}

static double calculate_rate(int exscore, int notes)
{
    return static_cast<double>(exscore) / static_cast<double>(notes * 2) * 100;
}

std::vector<std::pair<HashMD5, ScoreBMS>> ScoreDB::fetchCachedPbBMSImpl(const std::string& sql_where,
                                                                        std::initializer_list<std::any> params) const
{
    std::vector<std::pair<HashMD5, ScoreBMS>> out;
    const auto raw_scores = query("SELECT md5, notes, score, fast, slow, maxcombo, addtime, exscore, lamp, pgreat, "
                                  "great, good, bad, bpoor, miss, bp, cb, playedtime, replay FROM score_cache_bms " +
                                      sql_where,
                                  params);
    for (const auto& raw_score : raw_scores)
    {
        LVF_DEBUG_ASSERT(raw_score.size() == 19);

        ScoreBMS score;
        score.notes = ANY_INT(raw_score[1]);
        score.score = ANY_INT(raw_score[2]);
        score.fast = ANY_INT(raw_score[3]);
        score.slow = ANY_INT(raw_score[4]);
        score.maxcombo = ANY_INT(raw_score[5]);
        score.addtime = ANY_INT(raw_score[6]);
        score.exscore = ANY_INT(raw_score[7]);
        score.lamp = static_cast<ScoreBMS::Lamp>(ANY_INT(raw_score[8]));
        score.pgreat = ANY_INT(raw_score[9]);
        score.great = ANY_INT(raw_score[10]);
        score.good = ANY_INT(raw_score[11]);
        score.bad = ANY_INT(raw_score[12]);
        score.kpoor = ANY_INT(raw_score[13]);
        score.miss = ANY_INT(raw_score[14]);
        score.bp = ANY_INT(raw_score[15]);
        score.combobreak = ANY_INT(raw_score[16]);
        score.play_time = ANY_INT(raw_score[17]);
        score.replayFileName = ANY_STR(raw_score[18]);
        score.rate = calculate_rate(score.exscore, score.notes);
        out.emplace_back(ANY_STR(raw_score[0]), std::move(score));
    }
    return out;
}

std::shared_ptr<ScoreBMS> ScoreDB::fetchCachedPbBMS(const HashMD5& hash) const
{
    const auto scores = fetchCachedPbBMSImpl("WHERE md5 = ?", {hash.hexdigest()});
    if (scores.empty())
        return nullptr;
    LVF_DEBUG_ASSERT(scores.size() == 1);
    return std::make_shared<ScoreBMS>(scores[0].second);
}

void ScoreDB::saveChartScoreBmsToHistory(const HashMD5& hash, const ScoreBMS& score)
{
    const auto lamp = static_cast<int>(score.lamp);
    const auto play_time = score.play_time.norm();
    const int ret = exec("INSERT INTO score_history_bms"
                         "(md5,notes,score,fast,slow,maxcombo,addtime,exscore,lamp,pgreat,great,good,bad,bpoor,miss,bp,"
                         "cb,playedtime,replay) "
                         "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                         {hash.hexdigest(), score.notes, score.score, score.fast, score.slow, score.maxcombo,
                          score.addtime, score.exscore, lamp, score.pgreat, score.great, score.good, score.bad,
                          score.kpoor, score.miss, score.bp, score.combobreak, play_time, score.replayFileName});
    if (ret != SQLITE_OK)
        lunaticvibes::verify_failed(("[ScoreDB] Save score: " + std::string{errmsg()}).c_str());
}

void ScoreDB::updateCachedChartPbBms(const HashMD5& hash, const ScoreBMS& score)
{
    const std::string hashStr = hash.hexdigest();

    if (const std::shared_ptr<ScoreBMS> pRecord = fetchCachedPbBMS(hash); pRecord)
    {
        auto record = *pRecord;

        if (score.exscore > record.exscore)
        {
            record.rate = calculate_rate(score.exscore, score.notes);
            record.fast = score.fast;
            record.slow = score.slow;
            record.exscore = score.exscore;
            record.pgreat = score.pgreat;
            record.great = score.great;
            record.good = score.good;
            record.bad = score.bad;
            record.kpoor = score.kpoor;
            record.miss = score.miss;
            record.combobreak = score.combobreak;
            record.replayFileName = score.replayFileName;
        }
        else if (score.exscore == record.exscore)
        {
            if (score.maxcombo > record.maxcombo || score.bp < record.bp ||
                static_cast<int>(score.lamp) > static_cast<int>(record.lamp))
                record.replayFileName = score.replayFileName;
        }

        record.bp = std::min(record.bp, score.bp);
        record.clearcount += score.clearcount;
        record.maxcombo = std::max(record.maxcombo, score.maxcombo);
        record.notes = score.notes;
        record.playcount += score.playcount;
        record.score = std::max(record.score, score.score);

        if (static_cast<int>(score.lamp) > static_cast<int>(record.lamp))
        {
            record.lamp = score.lamp;
        }

        record.play_time += score.play_time;

        const auto play_time = record.play_time.norm();
        exec("UPDATE score_cache_bms SET "
             "notes=?,score=?,fast=?,slow=?,maxcombo=?,addtime=?,pc=?,clearcount=?,exscore=?,lamp=?,pgreat=?,"
             "great=?,good=?,bad=?,bpoor=?,miss=?,bp=?,cb=?,playedtime=?,replay=? WHERE md5=?",
             {record.notes,   record.score,     record.fast,       record.slow,    record.maxcombo,
              record.addtime, record.playcount, record.clearcount, record.exscore, static_cast<int>(record.lamp),
              record.pgreat,  record.great,     record.good,       record.bad,     record.kpoor,
              record.miss,    record.bp,        record.combobreak, play_time,      record.replayFileName,
              hashStr});
        cache["score_bms"].insert_or_assign(hashStr, std::make_shared<ScoreBMS>(std::move(record)));
    }
    else
    {
        const auto play_time = score.play_time.norm();
        exec("INSERT INTO score_cache_bms"
             "(md5,notes,score,fast,slow,maxcombo,addtime,pc,clearcount,exscore,lamp,pgreat,great,good,bad,bpoor,"
             "miss,bp,cb,playedtime,replay) "
             "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
             {hashStr,
              score.notes,
              score.score,
              score.fast,
              score.slow,
              score.maxcombo,
              score.addtime,
              score.playcount,
              score.clearcount,
              score.exscore,
              static_cast<int>(score.lamp),
              score.pgreat,
              score.great,
              score.good,
              score.bad,
              score.kpoor,
              score.miss,
              score.bp,
              score.combobreak,
              play_time,
              score.replayFileName});
        cache["score_bms"].insert_or_assign(hashStr, std::make_shared<ScoreBMS>(score));
    }
}

static bool convertHistoryScoreBms(ScoreBMS& out, const std::vector<std::any>& score) noexcept
try
{
    if (score.size() < 19)
        return false;
    // md5 = ANY_STR(score[0]);
    out.notes = ANY_INT(score[1]);
    out.score = ANY_INT(score[2]);
    out.fast = ANY_INT(score[3]);
    out.slow = ANY_INT(score[4]);
    out.maxcombo = ANY_INT(score[5]);
    out.addtime = ANY_INT(score[6]);
    out.exscore = ANY_INT(score[7]);
    out.lamp = static_cast<ScoreBMS::Lamp>(ANY_INT(score[8]));
    out.pgreat = ANY_INT(score[9]);
    out.great = ANY_INT(score[10]);
    out.good = ANY_INT(score[11]);
    out.bad = ANY_INT(score[12]);
    out.kpoor = ANY_INT(score[13]);
    out.miss = ANY_INT(score[14]);
    out.bp = ANY_INT(score[15]);
    out.combobreak = ANY_INT(score[16]);
    out.play_time = ANY_INT(score[17]);
    out.replayFileName = ANY_STR(score[18]);

    out.rate = static_cast<double>(out.exscore) / static_cast<double>(out.notes * 2) * 100;
    return true;
}
catch (const std::exception& e)
{
    lunaticvibes::verify_failed(("convertHistoryScoreBms: " + std::string{e.what()}).c_str());
    return false;
}

static ScoreBMS::Lamp from_lr2(int clear)
{
    using L = ScoreBMS::Lamp;
    switch (clear)
    {
    // Also course plays?
    case 0x0: return L::NOPLAY;
    case 0x1: return L::FAILED;
    case 0x2: return L::EASY;
    case 0x3: return L::NORMAL;
    // Also good-attack when not FC?
    case 0x4: return L::HARD;
    case 0x5: return L::FULLCOMBO;
    default:
        lunaticvibes::verify_failed(
            ("[ScoreDB] Unknown LR2 clear=" + std::to_string(clear) + "; Report this!").c_str());
        return L::NOPLAY;
    }
}

void ScoreDB::importScores(lunaticvibes::Lr2ScoreDb& lr2_db)
{
    ScoreBMS score;
    lr2_db.proc([this, &score](const lunaticvibes::Lr2Score& lr2_score) {
        // score.ghost = lr2_score.ghost;
        // score.hash = lr2_score.hash;
        // score.scorehash = lr2_score.scorehash;
        score.bad = lr2_score.bad;
        score.lamp = from_lr2(lr2_score.clear);
        // score.clear_db = lr2_score.clear_db;
        // score.clear_ex = lr2_score.clear_ex;
        // score.clear_sd = lr2_score.clear_sd;
        score.clearcount = lr2_score.clearcount;
        // score.complete = lr2_score.complete; // ?
        // score.failcount = lr2_score.failcount;
        score.good = lr2_score.good;
        score.great = lr2_score.great;
        score.maxcombo = lr2_score.maxcombo;
        score.bp = lr2_score.minbp;
        // score.op_best = lr2_score.op_best;
        // score.op_history = lr2_score.op_history;
        score.pgreat = lr2_score.perfect;
        score.playcount = lr2_score.playcount;
        score.kpoor = lr2_score.poor; // wrong?
        // score.rank = lr2_score.rank; // ?
        // lr2_score.rate - calculated within score saving.
        // score.rseed = lr2_score.rseed; // no
        score.notes = lr2_score.totalnotes;

        score.exscore = score.pgreat * 2 + score.great;
        updateCachedChartPbBms(HashMD5{lr2_score.hash}, score);
    });
    LOG_INFO << "[ScoreDB] LR2 import done";
}

bool ScoreDB::isBmsPbCacheEmpty()
{
    const auto resp = query("SELECT EXISTS (SELECT 1 FROM score_cache_bms)");
    LVF_DEBUG_ASSERT(resp.size() == 1);
    return ANY_INT(resp[0][0]) == 0;
}

void ScoreDB::rebuildBmsPbCache()
{
    LOG_DEBUG << "[ScoreDB] Asked to rebuild BMS score PB cache";
    Transaction transaction(*this);
    exec("DELETE FROM score_cache_bms");
    for (const auto& raw_score : query("SELECT * FROM score_bms ORDER BY addtime"))
    {
        auto score = std::make_shared<ScoreBMS>();
        const std::string md5 = ANY_STR(raw_score[0]);
        convert_score_bms(*score, raw_score);
        updateCachedChartPbBms(HashMD5{md5}, *score);
    }
    for (const auto& raw_score : query("SELECT "
                                       "md5,notes,score,fast,slow,maxcombo,addtime,exscore,lamp,pgreat,great,good,bad,"
                                       "bpoor,miss,bp,cb,playedtime,replay "
                                       "FROM score_history_bms "
                                       "ORDER BY addtime"))
    {
        auto score = std::make_shared<ScoreBMS>();
        const std::string md5 = ANY_STR(raw_score[0]);
        convertHistoryScoreBms(*score, raw_score);
        updateCachedChartPbBms(HashMD5{md5}, *score);
    }
    transaction.commit();
    preloadScore();
}

void ScoreDB::deleteCourseScoreBMS(const HashMD5& hash)
{
    deleteLegacyScoreBMS("score_course_bms", hash);
}

std::shared_ptr<ScoreBMS> ScoreDB::getCourseScoreBMS(const HashMD5& hash) const
{
    return getScoreBMS("score_course_bms", hash);
}

void ScoreDB::updateCourseScoreBMS(const HashMD5& hash, const ScoreBMS& score)
{
    updateLegacyScoreBMS("score_course_bms", hash, score);
}

void ScoreDB::preloadScore()
{
    cache.clear();
    for (const auto& r : query("SELECT * FROM score_course_bms"))
    {
        auto score = std::make_shared<ScoreBMS>();
        convert_score_bms(*score, r);
        cache["score_course_bms"].insert_or_assign(ANY_STR(r[0]), std::move(score));
    }
    for (auto&& [hash, score] : fetchCachedPbBMSImpl({}, {}))
    {
        cache["score_bms"].insert_or_assign(hash.hexdigest(), std::make_shared<ScoreBMS>(std::move(score)));
    }
}

lunaticvibes::OverallStats ScoreDB::getStats()
{
    lunaticvibes::OverallStats stats;
    const auto stats_cache_all = query("SELECT play_count, clear_count, pgreat, great, good, bad, poor, running_combo, "
                                       "max_running_combo, playtime FROM stats");
    LVF_DEBUG_ASSERT(stats_cache_all.size() == 1);
    const auto& stats_cache = stats_cache_all[0];
    stats.play_count = ANY_INT(stats_cache[0]);
    stats.clear_count = ANY_INT(stats_cache[1]);
    stats.pgreat = ANY_INT(stats_cache[2]);
    stats.great = ANY_INT(stats_cache[3]);
    stats.good = ANY_INT(stats_cache[4]);
    stats.bad = ANY_INT(stats_cache[5]);
    stats.poor = ANY_INT(stats_cache[6]);
    stats.running_combo = ANY_INT(stats_cache[7]);
    stats.max_running_combo = ANY_INT(stats_cache[8]);
    stats.playtime = ANY_INT(stats_cache[9]);
    return stats;
}

void ScoreDB::updateStats(const ScoreBMS& score)
{
    int ret;
    auto stats = getStats();
    stats.play_count += 1;
    // NOTE: in course plays, NOPLAY is a clear, and FAILED is a fail.
    stats.clear_count += static_cast<int64_t>(score.lamp != ScoreBMS::Lamp::FAILED);
    stats.pgreat += score.pgreat;
    stats.great += score.great;
    stats.good += score.good;
    stats.bad += score.bad;
    stats.poor += score.kpoor + score.miss;
    const auto first_running_combo = stats.running_combo + score.first_max_combo;
    stats.running_combo = score.combobreak == 0 ? stats.running_combo + score.maxcombo : score.final_combo;
    stats.max_running_combo = std::max(
        {stats.max_running_combo, first_running_combo, static_cast<int64_t>(score.maxcombo), stats.running_combo});
    stats.playtime += score.play_time.norm();

    ret = exec("UPDATE stats SET play_count=?, clear_count=?, pgreat=?, great=?, good=?, bad=?, poor=?, "
               "running_combo=?, max_running_combo=?, playtime=? WHERE id = 573",
               {stats.play_count, stats.clear_count, stats.pgreat, stats.great, stats.good, stats.bad, stats.poor,
                stats.running_combo, stats.max_running_combo, stats.playtime});
    if (ret != SQLITE_OK)
        lunaticvibes::verify_failed(("[ScoreDB] Write new stats: " + std::string{errmsg()}).c_str());
}

void ScoreDB::initTables()
{
    // TODO: rename to like 'legacy_pbs_bms'.
    // TODO: remove default values.
    // TODO: remove 'rate'.
    // TODO: rename 'pc' column.
    // FIXME: score algorithm version.
    static constexpr auto&& create_score_bms_sql = R"(
        CREATE TABLE IF NOT EXISTS score_bms (
            md5 TEXT PRIMARY KEY UNIQUE NOT NULL,
            notes INTEGER NOT NULL,
            score INTEGER NOT NULL,
            rate REAL NOT NULL,
            fast INTEGER NOT NULL,
            slow INTEGER NOT NULL,
            maxcombo INTEGER NOT NULL DEFAULT 0,
            addtime INTEGER NOT NULL DEFAULT 0,
            pc INTEGER NOT NULL DEFAULT 0,
            clearcount INTEGER NOT NULL DEFAULT 0,
            exscore INTEGER NOT NULL,
            lamp INTEGER NOT NULL,
            pgreat INTEGER NOT NULL,
            great INTEGER NOT NULL,
            good INTEGER NOT NULL,
            bad INTEGER NOT NULL,
            bpoor INTEGER NOT NULL,
            miss INTEGER NOT NULL,
            bp INTEGER NOT NULL,
            cb INTEGER NOT NULL,
            replay TEXT
        )
    )";
    if (exec(create_score_bms_sql) != SQLITE_OK)
        lunaticvibes::assert_failed(("[ScoreDB] Failed to create table 'score_bms': " + std::string{errmsg()}).c_str());

    static constexpr auto&& create_score_history_bms_sql = R"(
        CREATE TABLE IF NOT EXISTS score_history_bms (
            md5 TEXT NOT NULL,
            notes INTEGER NOT NULL,
            score INTEGER NOT NULL,
            fast INTEGER NOT NULL,
            slow INTEGER NOT NULL,
            maxcombo INTEGER NOT NULL,
            addtime INTEGER NOT NULL,
            exscore INTEGER NOT NULL,
            lamp INTEGER NOT NULL,
            pgreat INTEGER NOT NULL,
            great INTEGER NOT NULL,
            good INTEGER NOT NULL,
            bad INTEGER NOT NULL,
            bpoor INTEGER NOT NULL,
            miss INTEGER NOT NULL,
            bp INTEGER NOT NULL,
            cb INTEGER NOT NULL,
            playedtime INTEGER NOT NULL,
            replay TEXT PRIMARY KEY UNIQUE NOT NULL
        )
    )";
    if (exec(create_score_history_bms_sql) != SQLITE_OK)
        lunaticvibes::assert_failed(
            ("[ScoreDB] Failed to create table 'score_history_bms': " + std::string{errmsg()}).c_str());

    // Combined score_bms and score_history_bms.
    static constexpr auto&& create_score_cache_bms_sql = R"(
        CREATE TABLE IF NOT EXISTS score_cache_bms (
            md5 TEXT PRIMARY KEY UNIQUE NOT NULL,
            notes INTEGER NOT NULL,
            score INTEGER NOT NULL,
            fast INTEGER NOT NULL,
            slow INTEGER NOT NULL,
            maxcombo INTEGER NOT NULL,
            addtime INTEGER NOT NULL,
            pc INTEGER NOT NULL,
            clearcount INTEGER NOT NULL,
            exscore INTEGER NOT NULL,
            lamp INTEGER NOT NULL,
            pgreat INTEGER NOT NULL,
            great INTEGER NOT NULL,
            good INTEGER NOT NULL,
            bad INTEGER NOT NULL,
            bpoor INTEGER NOT NULL,
            miss INTEGER NOT NULL,
            bp INTEGER NOT NULL,
            cb INTEGER NOT NULL,
            playedtime INTEGER NOT NULL,
            replay TEXT NOT NULL
        )
    )";
    if (exec(create_score_cache_bms_sql) != SQLITE_OK)
        lunaticvibes::assert_failed(
            ("[ScoreDB] Failed to create table 'score_cache_bms': " + std::string{errmsg()}).c_str());

    static constexpr auto&& create_score_course_bms_sql = R"(
        CREATE TABLE IF NOT EXISTS score_course_bms (
            md5 TEXT PRIMARY KEY UNIQUE NOT NULL,
            notes INTEGER NOT NULL,
            score INTEGER NOT NULL,
            rate REAL NOT NULL,
            fast INTEGER NOT NULL,
            slow INTEGER NOT NULL,
            maxcombo INTEGER NOT NULL DEFAULT 0,
            addtime INTEGER NOT NULL DEFAULT 0,
            pc INTEGER NOT NULL DEFAULT 0,
            clearcount INTEGER NOT NULL DEFAULT 0,
            exscore INTEGER NOT NULL,
            lamp INTEGER NOT NULL,
            pgreat INTEGER NOT NULL,
            great INTEGER NOT NULL,
            good INTEGER NOT NULL,
            bad INTEGER NOT NULL,
            bpoor INTEGER NOT NULL,
            miss INTEGER NOT NULL,
            bp INTEGER NOT NULL,
            cb INTEGER NOT NULL,
            replay TEXT
        )
    )";
    if (exec(create_score_course_bms_sql) != SQLITE_OK)
        lunaticvibes::assert_failed(
            ("[ScoreDB] Failed to create table 'score_course_bms': " + std::string{errmsg()}).c_str());

    static constexpr auto&& create_stats_sql = R"(
        CREATE TABLE IF NOT EXISTS stats (
            id INTEGER PRIMARY KEY CHECK (id = 573), -- Only one row, ever.
            play_count INTEGER NOT NULL, clear_count INTEGER NOT NULL,
            pgreat INTEGER NOT NULL,
            great INTEGER NOT NULL,
            good INTEGER NOT NULL,
            bad INTEGER NOT NULL,
            poor INTEGER NOT NULL,
            running_combo INTEGER NOT NULL, max_running_combo INTEGER NOT NULL,
            playtime INTEGER NOT NULL
        )
    )";
    if (exec(create_stats_sql) != SQLITE_OK)
        lunaticvibes::assert_failed(("[ScoreDB] Failed to create table 'stats': " + std::string{errmsg()}).c_str());

    static constexpr auto&& init_stats_sql = R"(
        INSERT OR IGNORE INTO stats (
             id,
             play_count,
             clear_count,
             pgreat,
             great,
             good,
             bad,
             poor,
             running_combo,
             max_running_combo,
             playtime
         )
         VALUES (573, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    )";
    if (exec(init_stats_sql) != SQLITE_OK)
        lunaticvibes::assert_failed(("[ScoreDB] Failed to initialize table 'stats': " + std::string{errmsg()}).c_str());
}
