#include "db_lr2_score.h"

#include <common/assert.h>
#include <common/log.h>
#include <db/db_conn.h>

#include <any>
#include <functional>
#include <vector>

static constexpr auto&& IN_MEMORY_DB_PATH = ":memory:";

lunaticvibes::Lr2ScoreDb::Lr2ScoreDb(const char* path) : SQLite(path, "Lr2ScoreDb", SQLite::OpenMode::ReadOnly)
{
    LVF_VERIFY(isReadOnly());
    LOG_INFO << "[Lr2ScoreDb] Opening database: " << path;
}

lunaticvibes::Lr2ScoreDb::Lr2ScoreDb(InMemoryRwTag) : SQLite(IN_MEMORY_DB_PATH, "Lr2ScoreDb")
{
    LVF_VERIFY(!isReadOnly());

    static constexpr auto&& LR2_CREATE_TABLE_SCORE =
        "CREATE TABLE score(hash TEXT primary key,clear INTEGER,perfect INTEGER,great INTEGER,good INTEGER,bad "
        "INTEGER,poor INTEGER,totalnotes INTEGER,maxcombo INTEGER,minbp INTEGER,playcount INTEGER,clearcount "
        "INTEGER,failcount INTEGER,rank INTEGER,rate INTEGER,clear_db INTEGER,op_history INTEGER,scorehash TEXT,ghost "
        "TEXT,clear_sd INTEGER,clear_ex INTEGER,op_best INTEGER, rseed INTEGER, complete INTEGER)";
    exec(LR2_CREATE_TABLE_SCORE);

    exec("INSERT INTO "
         "score(hash,clear,perfect,great,good,bad,poor,totalnotes,maxcombo,minbp,playcount,clearcount,"
         "failcount,rank,rate,clear_db,op_history,scorehash,ghost,clear_sd,clear_ex,op_best,rseed,complete)"
         "VALUES('deadbeefdeadbeefdeadbeefdeadbeef',1,386,336,60,33,64,992,172,262,1,0,1,5,55,0,0,'"
         "feedfeedfeedfeedfeedfeedfeedfeed',null,0,0,0,16215,0)");
}

void lunaticvibes::Lr2ScoreDb::proc(const std::function<void(const Lr2Score& score)>& cb)
{
    Lr2Score score;
    // oh no, query() returns a vector. Well, at least this interface less allocating.
    for (const std::vector<std::any>& row : query(
             "SELECT hash, clear, perfect, great, good, bad, poor, totalnotes, maxcombo, minbp, playcount, clearcount, "
             "failcount, rank, rate, clear_db, op_history, scorehash, ghost, clear_sd, clear_ex, op_best, rseed, "
             "complete "
             "FROM score"))
    {
        LVF_DEBUG_ASSERT(row.size() == 24);
        score.hash = ANY_STR(row[0]);
        if (score.hash.size() != 32)
        {
            LOG_INFO << "[Lr2ScoreDb] Skipping dan score: " << score.hash;
            continue;
        }
        score.clear = ANY_INT(row[1]);
        score.perfect = ANY_INT(row[2]);
        score.great = ANY_INT(row[3]);
        score.good = ANY_INT(row[4]);
        score.bad = ANY_INT(row[5]);
        score.poor = ANY_INT(row[6]);
        score.totalnotes = ANY_INT(row[7]);
        score.maxcombo = ANY_INT(row[8]);
        score.minbp = ANY_INT(row[9]);
        score.playcount = ANY_INT(row[10]);
        score.clearcount = ANY_INT(row[11]);
        score.failcount = ANY_INT(row[12]);
        score.rank = ANY_INT(row[13]);
        score.rate = ANY_INT(row[14]);
        score.clear_db = ANY_INT(row[15]);
        score.op_history = ANY_INT(row[16]);
        score.scorehash = ANY_STR(row[17]);
        if (row[18].has_value()) // ghost is sometimes NULL
            score.ghost = ANY_STR(row[18]);
        score.clear_sd = ANY_INT(row[19]);
        score.clear_ex = ANY_INT(row[20]);
        score.op_best = ANY_INT(row[21]);
        score.rseed = ANY_INT(row[22]);
        score.complete = ANY_INT(row[23]);
        cb(score);
    }
}
