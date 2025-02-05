#include <game/scene/scene_result.h>

#include <common/assert.h>
#include <common/beat.h>
#include <common/play_modifiers.h>
#include <common/types.h>
#include <config/config_mgr.h>
#include <db/db_score.h>
#include <db/db_song.h>
#include <game/arena/arena_client.h>
#include <game/arena/arena_data.h>
#include <game/arena/arena_host.h>
#include <game/ruleset/ruleset.h>
#include <game/ruleset/ruleset_bms.h>
#include <game/runtime/index/option.h>
#include <game/scene/scene_context.h>
#include <game/sound/sound_mgr.h>
#include <game/sound/sound_sample.h>

#include <algorithm>
#include <filesystem>

static ScoreBMS::Lamp optionLampToBms(const Option::e_lamp_type lamp)
{
    switch (lamp)
    {
    case Option::e_lamp_type::LAMP_NOPLAY: return ScoreBMS::Lamp::NOPLAY;
    case Option::e_lamp_type::LAMP_FAILED: return ScoreBMS::Lamp::FAILED;
    case Option::e_lamp_type::LAMP_ASSIST: return ScoreBMS::Lamp::ASSIST;
    case Option::e_lamp_type::LAMP_EASY: return ScoreBMS::Lamp::EASY;
    case Option::e_lamp_type::LAMP_NORMAL: return ScoreBMS::Lamp::NORMAL;
    case Option::e_lamp_type::LAMP_HARD: return ScoreBMS::Lamp::HARD;
    case Option::e_lamp_type::LAMP_EXHARD: return ScoreBMS::Lamp::EXHARD;
    case Option::e_lamp_type::LAMP_FULLCOMBO: return ScoreBMS::Lamp::FULLCOMBO;
    case Option::e_lamp_type::LAMP_PERFECT: return ScoreBMS::Lamp::PERFECT;
    case Option::e_lamp_type::LAMP_MAX: return ScoreBMS::Lamp::MAX;
    }
    lunaticvibes::assert_failed("optionLampToBms");
}

SceneResult::SceneResult(const std::shared_ptr<SkinMgr>& skinMgr) : SceneBase(skinMgr, SkinType::RESULT, 1000)
{
    _type = SceneType::RESULT;

    _inputAvailable = INPUT_MASK_FUNC;

    if (gPlayContext.chartObj[PLAYER_SLOT_PLAYER] != nullptr)
    {
        _inputAvailable |= INPUT_MASK_1P;
    }

    if (gPlayContext.chartObj[PLAYER_SLOT_TARGET] != nullptr)
    {
        _inputAvailable |= INPUT_MASK_2P;
    }

    state = eResultState::DRAW;

    saveLampMax = ScoreBMS::Lamp::NOPLAY;
    if (!gPlayContext.isReplay)
    {
        const auto [saveScoreType, saveLampMaxType] = getSaveScoreType(false);
        saveScore = saveScoreType;
        saveLampMax = optionLampToBms(saveLampMaxType);
    }

    std::map<std::string, int> param;

    struct Params
    {
        Option::e_rank_type db_rank = Option::e_rank_type::RANK_NONE;
        Option::e_rank_type p1_rank = Option::e_rank_type::RANK_NONE;
        Option::e_rank_type p2_rank = Option::e_rank_type::RANK_NONE;
    } param_new;

    lamp[PLAYER_SLOT_PLAYER] = optionLampToBms(Option::LAMP_NOPLAY);
    lamp[PLAYER_SLOT_TARGET] = optionLampToBms(Option::LAMP_NOPLAY);
    if (gPlayContext.ruleset[PLAYER_SLOT_PLAYER])
    {
        gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->updateGlobals();

        // set options
        auto d1p = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->getData();
        param_new.p1_rank = Option::getRankType(d1p.total_acc);
        param["1pmaxcombo"] = d1p.maxCombo;

        if (auto pr = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); pr)
        {
            lamp[PLAYER_SLOT_PLAYER] = optionLampToBms(pr->calculateLamp());
            param["1pexscore"] = pr->getExScore();
            param["1pbp"] = pr->getJudgeCountEx(RulesetBMS::JUDGE_BP);
        }

        if (gPlayContext.ruleset[PLAYER_SLOT_TARGET])
        {
            gPlayContext.ruleset[PLAYER_SLOT_TARGET]->updateGlobals();

            auto d2p = gPlayContext.ruleset[PLAYER_SLOT_TARGET]->getData();
            param_new.p2_rank = Option::getRankType(d1p.total_acc);
            param["2pmaxcombo"] = d2p.maxCombo;

            if (auto pr = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_TARGET]); pr)
            {
                lamp[PLAYER_SLOT_TARGET] = optionLampToBms(pr->calculateLamp());
                param["2pexscore"] = pr->getExScore();
                param["2pbp"] = pr->getJudgeCountEx(RulesetBMS::JUDGE_BP);
            }
        }

        // TODO set chart info (total notes, etc.)
        auto chartLength = gPlayContext.chartObj[PLAYER_SLOT_PLAYER]->getTotalLength().norm() / 1000;
        param["min"] = int(chartLength / 60);
        param["sec"] = int(chartLength % 60);

        // compare to db record
        auto pScore = g_pScoreDB->getChartScoreBMS(gChartContext.hash);
        if (pScore)
        {
            param["dbexscore"] = pScore->exscore;
            param["dbexscorediff"] = param["1pexscore"] - pScore->exscore;
            param["newexscore"] = param["1pexscore"];
            param["newexscorediff"] = param["newexscore"] - pScore->exscore;
            param["dbmaxcombo"] = (int)pScore->maxcombo;
            param["newmaxcombo"] = param["1pmaxcombo"];
            param["newmaxcombodiff"] = param["newmaxcombo"] - pScore->maxcombo;
            param["dbbp"] = pScore->bp;
            param["newbp"] = param["1pbp"];
            param["newbpdiff"] = param["newbp"] - pScore->bp;
            param["dbrate"] = (int)(pScore->rate);
            param["dbrated2"] = (int)(pScore->rate * 100.0) % 100;
            param_new.db_rank = Option::getRankType(pScore->rate);

            param["updatedscore"] = pScore->exscore < param["1pexscore"];
            param["updatedmaxcombo"] = pScore->maxcombo < d1p.maxCombo;
            param["updatedbp"] = pScore->bp > param["1pbp"];
        }
        else if (saveScore)
        {
            param["dbexscorediff"] = param["1pexscore"];
            param["newexscore"] = param["1pexscore"];
            param["newexscorediff"] = param["newexscore"];
            param["newmaxcombo"] = param["1pmaxcombo"];
            param["newmaxcombodiff"] = param["newmaxcombo"];
            param["newbp"] = param["1pbp"];
            param["newbpdiff"] = param["newbp"];
            param["updatedscore"] = true;
            param["updatedmaxcombo"] = true;
            param["updatedbp"] = true;
        }

        if (!gPlayContext.isBattle)
        {
            if (State::get(IndexOption::PLAY_TARGET_TYPE) == Option::TARGET_MYBEST && gPlayContext.replayMybest)
            {
                param["2pexscore"] = param["dbexscore"];
            }
            else if (State::get(IndexOption::PLAY_TARGET_TYPE) == Option::TARGET_0)
            {
                param["2pexscore"] = 0;
            }
        }
        param["1ptarget"] = param["1pexscore"] - param["2pexscore"];
        param["2ptarget"] = param["2pexscore"] - param["1pexscore"];
        param["winlose"] = (param["1ptarget"] > 0) ? 1 : (param["1ptarget"] < 0) ? 2 : 0;
    }

    // save
    {
        State::set(IndexOption::RESULT_RANK_1P, param_new.p1_rank);
        State::set(IndexOption::RESULT_RANK_2P, param_new.p2_rank);
        State::set(IndexNumber::PLAY_1P_EXSCORE, param["1pexscore"]);
        State::set(IndexNumber::PLAY_2P_EXSCORE, param["2pexscore"]);
        State::set(IndexNumber::PLAY_1P_EXSCORE_DIFF, param["1ptarget"]);
        State::set(IndexNumber::PLAY_2P_EXSCORE_DIFF, param["2ptarget"]);
        State::set(IndexOption::RESULT_BATTLE_WIN_LOSE, param["winlose"]);

        State::set(IndexNumber::PLAY_MIN, param["min"]);
        State::set(IndexNumber::PLAY_SEC, param["sec"]);
        State::set(IndexNumber::PLAY_REMAIN_MIN, param["min"]);
        State::set(IndexNumber::PLAY_REMAIN_SEC, param["sec"]);

        State::set(IndexNumber::RESULT_RECORD_EX_BEFORE, param["dbexscore"]);
        State::set(IndexNumber::RESULT_RECORD_EX_NOW, param["newexscore"]);
        State::set(IndexNumber::RESULT_RECORD_EX_DIFF, param["newexscorediff"]);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_BEFORE, param["dbmaxcombo"]);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_NOW, param["newmaxcombo"]);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_DIFF, param["newmaxcombodiff"]);
        State::set(IndexNumber::RESULT_RECORD_BP_BEFORE, param["dbbp"]);
        State::set(IndexNumber::RESULT_RECORD_BP_NOW, param["1pbp"]);
        State::set(IndexNumber::RESULT_RECORD_BP_DIFF, param["newbpdiff"]);
        State::set(IndexNumber::RESULT_RECORD_MYBEST_RATE, param["dbrate"]);
        State::set(IndexNumber::RESULT_RECORD_MYBEST_RATE_DECIMAL2, param["dbrated2"]);

        State::set(IndexNumber::RESULT_MYBEST_EX, param["dbexscore"]);
        State::set(IndexNumber::RESULT_MYBEST_DIFF, param["dbexscorediff"]);
        State::set(IndexNumber::RESULT_MYBEST_RATE, param["dbrate"]);
        State::set(IndexNumber::RESULT_MYBEST_RATE_DECIMAL2, param["dbrated2"]);

        State::set(IndexOption::RESULT_MYBEST_RANK, param_new.db_rank);
        State::set(IndexOption::RESULT_UPDATED_RANK, param_new.p1_rank);

        State::set(IndexSwitch::RESULT_UPDATED_SCORE, param["updatedscore"]);
        State::set(IndexSwitch::RESULT_UPDATED_MAXCOMBO, param["updatedmaxcombo"]);
        State::set(IndexSwitch::RESULT_UPDATED_BP, param["updatedbp"]);

        State::set(IndexOption::SELECT_ENTRY_LAMP, State::get(IndexOption::RESULT_CLEAR_TYPE_1P));
    }

    LOG_INFO << "[Result] " << (State::get(IndexSwitch::RESULT_CLEAR) ? "CLEARED" : "FAILED");

    _input.register_p("SCENE_PRESS", std::bind_front(&SceneResult::inputGamePress, this));
    _input.register_h("SCENE_HOLD", std::bind_front(&SceneResult::inputGameHold, this));

    lunaticvibes::Time t;
    State::set(IndexTimer::RESULT_GRAPH_START, t.norm());

    if (!gInCustomize)
    {
        SoundMgr::stopSysSamples();

        if (State::get(IndexSwitch::RESULT_CLEAR))
            SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::SOUND_CLEAR);
        else
            SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::SOUND_FAIL);
    }
}

////////////////////////////////////////////////////////////////////////////////

SceneResult::~SceneResult()
{
    _input.loopEnd();
}

void SceneResult::update_fixed(const lunaticvibes::Time& t)
{
    if (gNextScene != SceneType::RESULT)
        return;

    if (gAppIsExiting)
    {
        gNextScene = SceneType::EXIT_TRANS;
    }

    switch (state)
    {
    case eResultState::DRAW: updateDraw(t); break;
    case eResultState::STOP: updateStop(t); break;
    case eResultState::RECORD: updateRecord(t); break;
    case eResultState::FADEOUT: updateFadeout(t); break;
    case eResultState::WAIT_ARENA: updateWaitArena(t); break;
    }

    if (gArenaData.isOnline() && gArenaData.isExpired())
    {
        gArenaData.reset();
    }
}

static void saveReplay(const lunaticvibes::Time t, std::string replayFileName, ScoreBMS::Lamp lamp,
                       bool also_save_for_course)
{
    // save replay
    std::unique_lock l{gPlayContext._mutex};
    std::unique_lock rl{gPlayContext.replayNew->mutex};
    try
    {
        auto replayPath = gPlayContext.replayNew->replay->getReplayPath() / replayFileName;
        LOG_DEBUG << "[Result] Saving replay to " << replayPath;
        std::ranges::stable_sort(gPlayContext.replayNew->replay->commands, {}, &ReplayChart::Commands::ms);
        gPlayContext.replayNew->replay->saveFile(replayPath);
        if (also_save_for_course)
            gPlayContext.courseStageReplayPathNew.push_back(replayPath);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // `cannot create directories: Not a directory` lol
        LOG_ERROR << "[Result] Failed to save replay: " << e.what();
        replayFileName.clear();
    }

    // save score
    LVF_ASSERT(gPlayContext.ruleset[PLAYER_SLOT_PLAYER] != nullptr);
    LVF_ASSERT(gPlayContext.chartObj[PLAYER_SLOT_PLAYER] != nullptr);
    std::shared_ptr<ScoreBase> pScore = nullptr;
    switch (gChartContext.chart->type())
    {
    case eChartFormat::BMS:
    case eChartFormat::BMSON: {
        auto score = std::make_shared<ScoreBMS>();

        auto& chart = gPlayContext.chartObj[PLAYER_SLOT_PLAYER];
        auto& ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER];
        const auto data = ruleset->getData();
        score->notes = chart->getNoteTotalCount();
        score->rate = data.total_acc;
        score->first_max_combo = data.firstMaxCombo;
        score->final_combo = data.combo;
        score->maxcombo = data.maxCombo;
        score->addtime = t.norm() / 1000;
        score->play_time = data.play_time;
        score->playcount = 1;
        score->clearcount = ruleset->isCleared() ? 1 : 0;
        score->replayFileName = std::move(replayFileName);

        auto rBMS = std::dynamic_pointer_cast<RulesetBMS>(ruleset);
        score->score = int(std::floor(rBMS->getScore()));
        score->exscore = rBMS->getExScore();
        score->fast = rBMS->getJudgeCountEx(RulesetBMS::JUDGE_EARLY);
        score->slow = rBMS->getJudgeCountEx(RulesetBMS::JUDGE_LATE);
        score->lamp = lamp;

        if (gPlayContext.mods[PLAYER_SLOT_PLAYER].assist_mask == 0)
        {
            score->pgreat = rBMS->getJudgeCount(RulesetBMS::JudgeType::PERFECT);
            score->great = rBMS->getJudgeCount(RulesetBMS::JudgeType::GREAT);
            score->good = rBMS->getJudgeCount(RulesetBMS::JudgeType::GOOD);
            score->bad = rBMS->getJudgeCount(RulesetBMS::JudgeType::BAD);
            score->kpoor = rBMS->getJudgeCountEx(RulesetBMS::JUDGE_KPOOR);
            score->miss = rBMS->getJudgeCountEx(RulesetBMS::JUDGE_MISS);
            score->bp = rBMS->getJudgeCountEx(RulesetBMS::JUDGE_BP);
            score->combobreak = rBMS->getJudgeCountEx(RulesetBMS::JUDGE_CB);
        }

        g_pScoreDB->insertChartScoreBMS(gChartContext.hash, *score);
        pScore = std::move(score);

        break;
    }
    case eChartFormat::UNKNOWN: lunaticvibes::verify_failed("saveReplay"); break;
    }

    // update entry list score
    for (auto& frame : gSelectContext.backtrace)
        for (auto& [entry, scoreOld] : frame.displayEntries)
            if (entry->md5 == gChartContext.hash)
                scoreOld = pScore;
}

void SceneResult::updateDraw(const lunaticvibes::Time& t)
{
    auto rt = t - State::get(IndexTimer::SCENE_START);

    if (!_savedScore and !gChartContext.hash.empty() && saveScore)
    {
        _savedScore = true;
        std::string replayFileName = std::format("{:04}{:02}{:02}-{:02}{:02}{:02}.rep",
                                                 State::get(IndexNumber::DATE_YEAR), State::get(IndexNumber::DATE_MON),
                                                 State::get(IndexNumber::DATE_DAY), State::get(IndexNumber::DATE_HOUR),
                                                 State::get(IndexNumber::DATE_MIN), State::get(IndexNumber::DATE_SEC));
        saveReplay(State::get(IndexTimer::SCENE_START), std::move(replayFileName),
                   std::min(lamp[PLAYER_SLOT_PLAYER], saveLampMax),
                   !(_retryRequested && gPlayContext.canRetry) && gPlayContext.isCourse);
    }

    if (rt.norm() >= pSkin->info.timeResultRank)
    {
        State::set(IndexTimer::RESULT_RANK_START, t.norm());
        // TODO play hit sound
        state = eResultState::STOP;
        LOG_DEBUG << "[Result] State changed to STOP";
    }
}

void SceneResult::updateStop(const lunaticvibes::Time& t) {}

void SceneResult::updateRecord(const lunaticvibes::Time& t)
{
    // TODO sync score in online mode?
    if (true)
    {
        _scoreSyncFinished = true;
    }
}

[[nodiscard]] SceneType lunaticvibes::advanceCourseStage(const SceneType exitScene, const SceneType playScene)
{
    gPlayContext.courseStage++;
    if (gPlayContext.courseStage >= gPlayContext.courseCharts.size())
    {
        return exitScene;
    }

    if (gPlayContext.isReplay)
    {
        gPlayContext.replay = std::make_shared<ReplayChart>();
        gPlayContext.replay->loadFile(gPlayContext.courseStageReplayPath[gPlayContext.courseStage]);
    }

    if (gPlayContext.courseStage + 1 == gPlayContext.courseCharts.size())
        State::set(IndexOption::PLAY_COURSE_STAGE, Option::STAGE_FINAL);
    else
        State::set(IndexOption::PLAY_COURSE_STAGE, Option::STAGE_1 + gPlayContext.courseStage);

    // set metadata
    auto pChart = *g_pSongDB->findChartByHash(gPlayContext.courseCharts[gPlayContext.courseStage]).begin();
    gChartContext.chart = pChart;

    auto& nextChart = *gChartContext.chart;
    // gChartContext.path = chart._filePath;
    gChartContext.path = nextChart.absolutePath;

    // only reload resources if selected chart is different
    if (gChartContext.hash != nextChart.fileHash)
    {
        gChartContext.sampleLoadedHash.reset();
        gChartContext.bgaLoadedHash.reset();
    }
    gChartContext.hash = nextChart.fileHash;

    // gChartContext.chart = std::make_shared<ChartFormatBase>(chart);
    gChartContext.title = nextChart.title;
    gChartContext.title2 = nextChart.title2;
    gChartContext.artist = nextChart.artist;
    gChartContext.artist2 = nextChart.artist2;
    gChartContext.genre = nextChart.genre;
    gChartContext.version = nextChart.version;
    gChartContext.level = nextChart.levelEstimated;
    gChartContext.minBPM = nextChart.minBPM;
    gChartContext.maxBPM = nextChart.maxBPM;
    gChartContext.startBPM = nextChart.startBPM;

    auto pScore = g_pScoreDB->getChartScoreBMS(gChartContext.hash);
    if (pScore && !pScore->replayFileName.empty())
    {
        Path replayFilePath = ReplayChart::getReplayPath(gChartContext.hash) / pScore->replayFileName;
        if (fs::is_regular_file(replayFilePath))
        {
            gPlayContext.replayMybest = std::make_shared<ReplayChart>();
            if (gPlayContext.replayMybest->loadFile(replayFilePath))
            {
                gPlayContext.mods[PLAYER_SLOT_MYBEST].randomLeft = gPlayContext.replayMybest->randomTypeLeft;
                gPlayContext.mods[PLAYER_SLOT_MYBEST].randomRight = gPlayContext.replayMybest->randomTypeRight;
                gPlayContext.mods[PLAYER_SLOT_MYBEST].gauge = gPlayContext.replayMybest->gaugeType;
                gPlayContext.mods[PLAYER_SLOT_MYBEST].assist_mask = gPlayContext.replayMybest->assistMask;
                gPlayContext.mods[PLAYER_SLOT_MYBEST].hispeedFix = gPlayContext.replayMybest->hispeedFix;
                gPlayContext.mods[PLAYER_SLOT_MYBEST].laneEffect =
                    (PlayModifierLaneEffectType)gPlayContext.replayMybest->laneEffectType;
                gPlayContext.mods[PLAYER_SLOT_MYBEST].DPFlip = gPlayContext.replayMybest->DPFlip;
            }
            else
            {
                gPlayContext.replayMybest.reset();
            }
        }
    }
    clearContextPlayForRetry();
    return playScene;
}

void SceneResult::updateFadeout(const lunaticvibes::Time& t)
{
    auto ft = t - State::get(IndexTimer::FADEOUT_BEGIN);

    if (ft >= pSkin->info.timeOutro)
    {
        SoundMgr::stopNoteSamples();

        // check retry
        if (_retryRequested && gPlayContext.canRetry)
        {
            SoundMgr::stopSysSamples();

            // update mybest
            if (!gPlayContext.isBattle)
            {
                auto pScore = g_pScoreDB->getChartScoreBMS(gChartContext.hash);
                if (pScore && !pScore->replayFileName.empty())
                {
                    Path replayFilePath = ReplayChart::getReplayPath(gChartContext.hash) / pScore->replayFileName;
                    if (!replayFilePath.empty() && fs::is_regular_file(replayFilePath))
                    {
                        gPlayContext.replayMybest = std::make_shared<ReplayChart>();
                        if (gPlayContext.replayMybest->loadFile(replayFilePath))
                        {
                            gPlayContext.mods[PLAYER_SLOT_MYBEST] = gPlayContext.replayMybest->getMods();
                        }
                        else
                        {
                            gPlayContext.replayMybest.reset();
                        }
                    }
                }
            }

            clearContextPlayForRetry();
            gNextScene = SceneType::PLAY;
        }
        else if (gPlayContext.isCourse)
        {
            if (!saveScore)
                gPlayContext.courseStageReplayPathNew.emplace_back();

            if (gPlayContext.ruleset[PLAYER_SLOT_PLAYER])
            {
                gPlayContext.initialHealth[PLAYER_SLOT_PLAYER] =
                    gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->getData().health;
                gPlayContext.courseStageRulesetCopy[PLAYER_SLOT_PLAYER].push_back(
                    gPlayContext.ruleset[PLAYER_SLOT_PLAYER]);
                gPlayContext.courseRunningCombo[PLAYER_SLOT_PLAYER] =
                    gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->getData().combo;
                gPlayContext.courseMaxCombo[PLAYER_SLOT_PLAYER] =
                    gPlayContext.ruleset[PLAYER_SLOT_PLAYER]->getData().maxCombo;
            }
            if (gPlayContext.ruleset[PLAYER_SLOT_TARGET])
            {
                gPlayContext.initialHealth[PLAYER_SLOT_TARGET] =
                    gPlayContext.ruleset[PLAYER_SLOT_TARGET]->getData().health;
                gPlayContext.courseStageRulesetCopy[PLAYER_SLOT_TARGET].push_back(
                    gPlayContext.ruleset[PLAYER_SLOT_TARGET]);
                gPlayContext.courseRunningCombo[PLAYER_SLOT_TARGET] =
                    gPlayContext.ruleset[PLAYER_SLOT_TARGET]->getData().combo;
                gPlayContext.courseMaxCombo[PLAYER_SLOT_TARGET] =
                    gPlayContext.ruleset[PLAYER_SLOT_TARGET]->getData().maxCombo;
            }

            gNextScene = lunaticvibes::advanceCourseStage(SceneType::COURSE_RESULT, SceneType::PLAY);
        }
        else
        {
            clearContextPlay();
            gPlayContext.isAuto = false;
            gPlayContext.isReplay = false;
            gNextScene = gQuitOnFinish ? SceneType::EXIT_TRANS : SceneType::SELECT;
        }
    }
}

void SceneResult::updateWaitArena(const lunaticvibes::Time& t)
{
    LVF_DEBUG_ASSERT(gArenaData.isOnline());

    if (!gArenaData.isOnline() || !gSelectContext.isArenaReady)
    {
        State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
        state = eResultState::FADEOUT;
        SoundMgr::setSysVolume(0.0, 2000);
        SoundMgr::setNoteVolume(0.0, 2000);
        LOG_DEBUG << "[Result] State changed to FADEOUT";
    }
}

////////////////////////////////////////////////////////////////////////////////

// CALLBACK
void SceneResult::inputGamePress(InputMask& m, const lunaticvibes::Time& t)
{
    if (t - State::get(IndexTimer::SCENE_START) < pSkin->info.timeIntro)
        return;

    if ((_inputAvailable & m & (INPUT_MASK_DECIDE | INPUT_MASK_CANCEL)).any() || m[Input::ESC])
    {
        switch (state)
        {
        case eResultState::DRAW:
            State::set(IndexTimer::RESULT_RANK_START, t.norm());
            // TODO play hit sound
            state = eResultState::STOP;
            LOG_DEBUG << "[Result] State changed to STOP";
            break;

        case eResultState::STOP:
            if (saveScore)
            {
                State::set(IndexTimer::RESULT_HIGHSCORE_START, t.norm());
                // TODO stop result sound
                // TODO play record sound
                state = eResultState::RECORD;
                LOG_DEBUG << "[Result] State changed to RECORD";
            }
            else if (gArenaData.isOnline())
            {
                if (gArenaData.isClient())
                    g_pArenaClient->setResultFinished();
                else
                    g_pArenaHost->setResultFinished();

                State::set(IndexTimer::ARENA_RESULT_WAIT, t.norm());
                state = eResultState::WAIT_ARENA;
                LOG_DEBUG << "[Result] State changed to WAIT_ARENA";
            }
            else
            {
                State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
                state = eResultState::FADEOUT;
                SoundMgr::setSysVolume(0.0, 2000);
                SoundMgr::setNoteVolume(0.0, 2000);
                LOG_DEBUG << "[Result] State changed to FADEOUT";
            }
            break;

        case eResultState::RECORD:
            if (_scoreSyncFinished)
            {
                if (gArenaData.isOnline())
                {
                    if (gArenaData.isClient())
                        g_pArenaClient->setResultFinished();
                    else
                        g_pArenaHost->setResultFinished();

                    State::set(IndexTimer::ARENA_RESULT_WAIT, t.norm());
                    state = eResultState::WAIT_ARENA;
                    LOG_DEBUG << "[Result] State changed to WAIT_ARENA";
                }
                else
                {
                    State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
                    state = eResultState::FADEOUT;
                    SoundMgr::setSysVolume(0.0, 2000);
                    SoundMgr::setNoteVolume(0.0, 2000);
                    LOG_DEBUG << "[Result] State changed to FADEOUT";
                }
            }
            break;

        case eResultState::FADEOUT:
        case eResultState::WAIT_ARENA: break;
        }
    }
}

// CALLBACK
void SceneResult::inputGameHold(InputMask& m, const lunaticvibes::Time& t)
{
    if (t - State::get(IndexTimer::SCENE_START) < pSkin->info.timeIntro)
        return;

    if (state == eResultState::FADEOUT)
    {
        _retryRequested =
            (_inputAvailable & m & INPUT_MASK_DECIDE).any() && (_inputAvailable & m & INPUT_MASK_CANCEL).any();
    }
}
