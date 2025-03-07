#include "scene_course_result.h"

#include <common/assert.h>
#include <common/log.h>
#include <common/play_modifiers.h>
#include <common/types.h>
#include <db/db_score.h>
#include <game/ruleset/ruleset.h>
#include <game/ruleset/ruleset_bms.h>
#include <game/runtime/index/option.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>
#include <game/sound/sound_mgr.h>
#include <game/sound/sound_sample.h>

SceneCourseResult::SceneCourseResult(const std::shared_ptr<SkinMgr>& skinMgr)
    : SceneBase(skinMgr, SkinType::COURSE_RESULT, 1000), _inputAvailable(INPUT_MASK_FUNC)
{
    _type = SceneType::COURSE_RESULT;

    if (gPlayContext.chartObj[PLAYER_SLOT_PLAYER] != nullptr)
    {
        _inputAvailable |= INPUT_MASK_1P;
    }

    if (gPlayContext.chartObj[PLAYER_SLOT_TARGET] != nullptr)
    {
        _inputAvailable |= INPUT_MASK_2P;
    }

    state = eCourseResultState::DRAW;

    struct Params
    {
        Option::e_rank_type db_rank = Option::e_rank_type::RANK_NONE;
        Option::e_rank_type p1_rank = Option::e_rank_type::RANK_NONE;
        Option::e_rank_type p2_rank = Option::e_rank_type::RANK_NONE;

        int p1target = 0;
        int p1exscore = 0;
        int p1maxcombo = 0;
        int p1bp = 0;
        int p1pg = 0;
        int p1gr = 0;
        int p1gd = 0;
        int p1bd = 0;
        int p1pr = 0;
        int p1bpoor = 0;
        int p1cb = 0;
        int p1fast = 0;
        int p1slow = 0;
        int p1rate = 0;
        int p1rated2 = 0;

        int p2target = 0;
        int p2exscore = 0;
        int p2maxcombo = 0;
        int p2bp = 0;
        int p2pg = 0;
        int p2gr = 0;
        int p2gd = 0;
        int p2bd = 0;
        int p2pr = 0;
        int p2bpoor = 0;
        int p2cb = 0;
        int p2fast = 0;
        int p2slow = 0;
        int p2rate = 0;
        int p2rated2 = 0;

        int dbexscore = 0;
        int dbexscorediff = 0;
        int newexscore = 0;
        int newexscorediff = 0;
        int dbmaxcombo = 0;
        int newmaxcombo = 0;
        int newmaxcombodiff = 0;
        int dbbp = 0;
        int newbp = 0;
        int newbpdiff = 0;
        int dbrate = 0;
        int dbrated2 = 0;

        int updatedscore = 0;
        int updatedmaxcombo = 0;
        int updatedbp = 0;

        int min = 0;
        int sec = 0;
        int remainmin = 0;
        int remainsec = 0;

        int p1pgrate = 0;
        int p1grrate = 0;
        int p1gdrate = 0;
        int p1bdrate = 0;
        int p1prrate = 0;

        int p2pgrate = 0;
        int p2grrate = 0;
        int p2gdrate = 0;
        int p2bdrate = 0;
        int p2prrate = 0;

        int winlose = 0;
    } params;

    if (!gPlayContext.courseStageRulesetCopy[0].empty())
    {
        for (const auto& r : gPlayContext.courseStageRulesetCopy[0])
        {
            const auto d = r->getData();
            summary[ARG_TOTAL_NOTES] += r->getNoteCount();
            summary[ARG_MAX_SCORE] += r->getMaxScore();
            summary[ARG_MAXCOMBO] = std::max(summary[ARG_MAXCOMBO], d.maxComboDisplay);

            if (auto pr = std::dynamic_pointer_cast<RulesetBMS>(r); pr)
            {
                summary[ARG_SCORE] += static_cast<unsigned>(std::floor(pr->getScore()));
                summary[ARG_EXSCORE] += pr->getExScore();
                summary[ARG_FAST] += pr->getJudgeCountEx(RulesetBMS::JUDGE_EARLY);
                summary[ARG_SLOW] += pr->getJudgeCountEx(RulesetBMS::JUDGE_LATE);
                summary[ARG_BP] += pr->getJudgeCountEx(RulesetBMS::JUDGE_BP);
                summary[ARG_CB] += pr->getJudgeCountEx(RulesetBMS::JUDGE_CB);
                summary[ARG_JUDGE_0] += pr->getJudgeCount(RulesetBMS::JudgeType::PERFECT);
                summary[ARG_JUDGE_1] += pr->getJudgeCount(RulesetBMS::JudgeType::GREAT);
                summary[ARG_JUDGE_2] += pr->getJudgeCount(RulesetBMS::JudgeType::GOOD);
                summary[ARG_JUDGE_3] += pr->getJudgeCount(RulesetBMS::JudgeType::BAD);
                summary[ARG_JUDGE_4] += pr->getJudgeCount(RulesetBMS::JudgeType::KPOOR);
                summary[ARG_JUDGE_5] += pr->getJudgeCount(RulesetBMS::JudgeType::MISS);
            }
        }
        if (summary[ARG_TOTAL_NOTES] > 0)
        {
            for (const auto& r : gPlayContext.courseStageRulesetCopy[0])
            {
                acc += r->getData().total_acc * r->getNoteCount() / summary[ARG_TOTAL_NOTES];
            }
        }

        // set options
        params.p1_rank = Option::getRankType(acc);
        params.p1target = summary[ARG_EXSCORE];
        params.p1exscore = summary[ARG_EXSCORE];
        params.p1maxcombo = summary[ARG_MAXCOMBO];
        params.p1bp = summary[ARG_BP];
        params.p1pg = summary[ARG_JUDGE_0];
        params.p1gr = summary[ARG_JUDGE_1];
        params.p1gd = summary[ARG_JUDGE_2];
        params.p1bd = summary[ARG_JUDGE_3];
        params.p1pr = summary[ARG_JUDGE_4] + summary[ARG_JUDGE_5];
        params.p1cb = summary[ARG_CB];
        params.p1fast = summary[ARG_FAST];
        params.p1slow = summary[ARG_SLOW];
        params.p1rate = static_cast<int>(acc);
        params.p1rated2 = static_cast<int>(acc * 100.0) % 100;

        if (summary[ARG_TOTAL_NOTES] > 0)
        {
            params.p1pgrate = static_cast<int>(100 * params.p1pg / summary[ARG_TOTAL_NOTES]);
            params.p1grrate = static_cast<int>(100 * params.p1gr / summary[ARG_TOTAL_NOTES]);
            params.p1gdrate = static_cast<int>(100 * params.p1gd / summary[ARG_TOTAL_NOTES]);
            params.p1bdrate = static_cast<int>(100 * params.p1bd / summary[ARG_TOTAL_NOTES]);
            params.p1prrate = static_cast<int>(100 * params.p1pr / summary[ARG_TOTAL_NOTES]);
        }

        if (gPlayContext.isBattle && !gPlayContext.courseStageRulesetCopy[1].empty())
        {
            decltype(summary) summary2P;
            double acc2P = 0.;
            for (const auto& r : gPlayContext.courseStageRulesetCopy[1])
            {
                const auto d = r->getData();
                summary2P[ARG_TOTAL_NOTES] += r->getNoteCount();
                summary2P[ARG_MAX_SCORE] += r->getMaxScore();
                summary2P[ARG_MAXCOMBO] = std::max(summary2P[ARG_MAXCOMBO], d.maxComboDisplay);
                acc2P += r->getNoteCount() > 0 ? (d.total_acc / r->getNoteCount()) : 0.0;

                if (auto pr = std::dynamic_pointer_cast<RulesetBMS>(r); pr)
                {
                    summary2P[ARG_SCORE] += static_cast<unsigned>(std::floor(pr->getScore()));
                    summary2P[ARG_EXSCORE] += pr->getExScore();
                    summary2P[ARG_FAST] += pr->getJudgeCountEx(RulesetBMS::JUDGE_EARLY);
                    summary2P[ARG_SLOW] += pr->getJudgeCountEx(RulesetBMS::JUDGE_LATE);
                    summary2P[ARG_BP] += pr->getJudgeCountEx(RulesetBMS::JUDGE_BP);
                    summary2P[ARG_CB] += pr->getJudgeCountEx(RulesetBMS::JUDGE_CB);
                    summary2P[ARG_JUDGE_0] += pr->getJudgeCount(RulesetBMS::JudgeType::PERFECT);
                    summary2P[ARG_JUDGE_1] += pr->getJudgeCount(RulesetBMS::JudgeType::GREAT);
                    summary2P[ARG_JUDGE_2] += pr->getJudgeCount(RulesetBMS::JudgeType::GOOD);
                    summary2P[ARG_JUDGE_3] += pr->getJudgeCount(RulesetBMS::JudgeType::BAD);
                    summary2P[ARG_JUDGE_4] += pr->getJudgeCount(RulesetBMS::JudgeType::KPOOR);
                    summary2P[ARG_JUDGE_5] += pr->getJudgeCount(RulesetBMS::JudgeType::MISS);
                }
            }
            if (summary[ARG_TOTAL_NOTES] > 0)
            {
                for (const auto& r : gPlayContext.courseStageRulesetCopy[1])
                {
                    acc2P += r->getData().total_acc * r->getNoteCount() / summary2P[ARG_TOTAL_NOTES];
                }
            }

            params.p2_rank = Option::getRankType(acc2P);
            params.p2target = summary2P[ARG_EXSCORE];
            params.p2exscore = summary2P[ARG_EXSCORE];
            params.p2maxcombo = summary2P[ARG_MAXCOMBO];
            params.p2bp = summary2P[ARG_BP];
            params.p2pg = summary2P[ARG_JUDGE_0];
            params.p2gr = summary2P[ARG_JUDGE_1];
            params.p2gd = summary2P[ARG_JUDGE_2];
            params.p2bd = summary2P[ARG_JUDGE_3];
            params.p2pr = summary2P[ARG_JUDGE_4] + summary2P[ARG_JUDGE_5];
            params.p2cb = summary2P[ARG_CB];
            params.p2fast = summary2P[ARG_FAST];
            params.p2slow = summary2P[ARG_SLOW];
            params.p2rate = static_cast<int>(acc2P);
            params.p2rated2 = static_cast<int>(acc2P * 100.0) % 100;

            if (summary2P[ARG_TOTAL_NOTES] > 0)
            {
                params.p2pgrate = static_cast<int>(100 * params.p2pg / summary2P[ARG_TOTAL_NOTES]);
                params.p2grrate = static_cast<int>(100 * params.p2gr / summary2P[ARG_TOTAL_NOTES]);
                params.p2gdrate = static_cast<int>(100 * params.p2gd / summary2P[ARG_TOTAL_NOTES]);
                params.p2bdrate = static_cast<int>(100 * params.p2bd / summary2P[ARG_TOTAL_NOTES]);
                params.p2prrate = static_cast<int>(100 * params.p2pr / summary2P[ARG_TOTAL_NOTES]);
            }

            params.p1target = summary[ARG_EXSCORE] - summary2P[ARG_EXSCORE];
            params.winlose = (params.p1target > 0) ? 1 : (params.p1target < 0) ? 2 : 0;
        }

        // compare to db record
        auto pScore = g_pScoreDB->getCourseScoreBMS(gPlayContext.courseHash);
        if (pScore)
        {
            params.dbexscore = pScore->exscore;
            params.dbexscorediff = params.p1exscore - pScore->exscore;
            params.newexscore = params.p1exscore;
            params.newexscorediff = params.newexscore - pScore->exscore;
            params.dbmaxcombo = static_cast<int>(pScore->maxcombo);
            params.newmaxcombo = params.p1maxcombo;
            params.newmaxcombodiff = params.newmaxcombo - pScore->maxcombo;
            params.dbbp = pScore->bp;
            params.newbp = params.p1bp;
            params.newbpdiff = params.newbp - pScore->bp;
            params.dbrate = static_cast<int>(pScore->rate);
            params.dbrated2 = static_cast<int>(pScore->rate * 100.0) % 100;
            params.db_rank = Option::getRankType(pScore->rate);

            params.updatedscore = pScore->exscore < params.p1exscore;
            params.updatedmaxcombo = pScore->maxcombo < params.p1maxcombo;
            params.updatedbp = pScore->bp < params.p1bp;
        }
        else
        {
            params.dbexscorediff = params.p1exscore;
            params.newexscore = params.p1maxcombo;
            params.newexscorediff = params.newexscore;
            params.newmaxcombo = params.p1maxcombo;
            params.newmaxcombodiff = params.newmaxcombo;
            params.newbp = params.p1bp;
            params.newbpdiff = params.newbp;
            params.updatedscore = true;
            params.updatedmaxcombo = true;
            params.updatedbp = true;
        }
    }

    // save
    {
        State::set(IndexOption::RESULT_RANK_1P, params.p1_rank);
        State::set(IndexOption::RESULT_RANK_2P, params.p2_rank);
        State::set(IndexNumber::PLAY_1P_EXSCORE_DIFF, params.p1target);
        State::set(IndexNumber::PLAY_2P_EXSCORE_DIFF, params.p2target);
        State::set(IndexOption::RESULT_BATTLE_WIN_LOSE, params.winlose);

        State::set(IndexNumber::PLAY_1P_EXSCORE, params.p1exscore);
        State::set(IndexNumber::PLAY_1P_PERFECT, params.p1pg);
        State::set(IndexNumber::PLAY_1P_GREAT, params.p1gr);
        State::set(IndexNumber::PLAY_1P_GOOD, params.p1gd);
        State::set(IndexNumber::PLAY_1P_BAD, params.p1bd);
        State::set(IndexNumber::PLAY_1P_POOR, params.p1pr);
        State::set(IndexNumber::PLAY_1P_BPOOR, params.p1bpoor);
        State::set(IndexNumber::PLAY_1P_COMBOBREAK, params.p1cb);
        State::set(IndexNumber::PLAY_1P_BP, params.p1bp);
        State::set(IndexNumber::PLAY_1P_FAST_COUNT, params.p1fast);
        State::set(IndexNumber::PLAY_1P_SLOW_COUNT, params.p1slow);
        State::set(IndexNumber::PLAY_1P_RATE, params.p1rate);
        State::set(IndexNumber::PLAY_1P_RATEDECIMAL, params.p1rated2);

        State::set(IndexNumber::PLAY_2P_EXSCORE, params.p2exscore);
        State::set(IndexNumber::PLAY_2P_PERFECT, params.p2pg);
        State::set(IndexNumber::PLAY_2P_GREAT, params.p2gr);
        State::set(IndexNumber::PLAY_2P_GOOD, params.p2gd);
        State::set(IndexNumber::PLAY_2P_BAD, params.p2bd);
        State::set(IndexNumber::PLAY_2P_POOR, params.p2pr);
        State::set(IndexNumber::PLAY_2P_BPOOR, params.p2bpoor);
        State::set(IndexNumber::PLAY_2P_COMBOBREAK, params.p2cb);
        State::set(IndexNumber::PLAY_2P_BP, params.p2bp);
        State::set(IndexNumber::PLAY_2P_FAST_COUNT, params.p2fast);
        State::set(IndexNumber::PLAY_2P_SLOW_COUNT, params.p2slow);
        State::set(IndexNumber::PLAY_2P_RATE, params.p2rate);
        State::set(IndexNumber::PLAY_2P_RATEDECIMAL, params.p2rated2);

        State::set(IndexNumber::PLAY_MIN, params.min);
        State::set(IndexNumber::PLAY_SEC, params.sec);
        State::set(IndexNumber::PLAY_REMAIN_MIN, params.min);
        State::set(IndexNumber::PLAY_REMAIN_SEC, params.sec);

        State::set(IndexNumber::RESULT_RECORD_EX_BEFORE, params.dbexscore);
        State::set(IndexNumber::RESULT_RECORD_EX_NOW, params.newexscore);
        State::set(IndexNumber::RESULT_RECORD_EX_DIFF, params.newexscorediff);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_BEFORE, params.dbmaxcombo);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_NOW, params.newmaxcombo);
        State::set(IndexNumber::RESULT_RECORD_MAXCOMBO_DIFF, params.newmaxcombodiff);
        State::set(IndexNumber::RESULT_RECORD_BP_BEFORE, params.dbbp);
        State::set(IndexNumber::RESULT_RECORD_BP_NOW, params.p1bp);
        State::set(IndexNumber::RESULT_RECORD_BP_DIFF, params.newbpdiff);
        State::set(IndexNumber::RESULT_RECORD_MYBEST_RATE, params.dbrate);
        State::set(IndexNumber::RESULT_RECORD_MYBEST_RATE_DECIMAL2, params.dbrated2);

        State::set(IndexNumber::RESULT_MYBEST_EX, params.dbexscore);
        State::set(IndexNumber::RESULT_MYBEST_DIFF, params.dbexscorediff);
        State::set(IndexNumber::RESULT_MYBEST_RATE, params.dbrate);
        State::set(IndexNumber::RESULT_MYBEST_RATE_DECIMAL2, params.dbrated2);

        State::set(IndexOption::RESULT_MYBEST_RANK, params.p1_rank);
        State::set(IndexOption::RESULT_UPDATED_RANK, params.p2_rank);

        State::set(IndexSwitch::RESULT_UPDATED_SCORE, params.updatedscore);
        State::set(IndexSwitch::RESULT_UPDATED_MAXCOMBO, params.updatedmaxcombo);
        State::set(IndexSwitch::RESULT_UPDATED_BP, params.updatedbp);
    }

    _input.register_p("SCENE_PRESS", std::bind_front(&SceneCourseResult::inputGamePress, this));
    _input.register_h("SCENE_HOLD", std::bind_front(&SceneCourseResult::inputGameHold, this));
    _input.register_r("SCENE_RELEASE", std::bind_front(&SceneCourseResult::inputGameRelease, this));

    auto t = lunaticvibes::Time::now();
    State::set(IndexTimer::RESULT_GRAPH_START, t.norm());

    if (!gInCustomize)
    {
        SoundMgr::stopSysSamples();
        SoundMgr::setSysVolume(1.0);

        if (State::get(IndexSwitch::RESULT_CLEAR))
            SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::SOUND_COURSE_CLEAR);
        else
            SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::SOUND_COURSE_FAIL);
    }
}

////////////////////////////////////////////////////////////////////////////////

SceneCourseResult::~SceneCourseResult()
{
    _input.loopEnd();
}

void SceneCourseResult::update_fixed(const lunaticvibes::Time& t)
{
    if (gNextScene != SceneType::COURSE_RESULT)
        return;

    if (gAppIsExiting)
    {
        gNextScene = SceneType::EXIT_TRANS;
    }

    switch (state)
    {
    case eCourseResultState::DRAW: updateDraw(t); break;
    case eCourseResultState::STOP: updateStop(t); break;
    case eCourseResultState::RECORD: updateRecord(t); break;
    case eCourseResultState::FADEOUT: updateFadeout(t); break;
    }
}

void SceneCourseResult::updateDraw(const lunaticvibes::Time& t)
{
    auto rt = t - State::get(IndexTimer::SCENE_START);

    if (rt.norm() >= pSkin->info.timeResultRank)
    {
        State::set(IndexTimer::RESULT_RANK_START, t.norm());
        // TODO play hit sound
        state = eCourseResultState::STOP;
        LOG_DEBUG << "[Result] State changed to STOP";
    }
}

void SceneCourseResult::updateStop(const lunaticvibes::Time& t) {}

void SceneCourseResult::updateRecord(const lunaticvibes::Time& t)
{
    // TODO sync score in online mode?
    if (true)
    {
        _scoreSyncFinished = true;
    }
}

void SceneCourseResult::updateFadeout(const lunaticvibes::Time& t)
{
    auto ft = t - State::get(IndexTimer::FADEOUT_BEGIN);

    if (ft >= pSkin->info.timeOutro)
    {
        // save score
        if (!gInCustomize && !gPlayContext.isReplay && !gPlayContext.isBattle && !gChartContext.hash.empty())
        {
            LVF_DEBUG_ASSERT(gPlayContext.ruleset[PLAYER_SLOT_PLAYER] != nullptr);
            ScoreBMS score;
            score.notes = summary[ARG_TOTAL_NOTES];
            score.rate = acc;
            score.maxcombo = summary[ARG_MAXCOMBO];
            score.playcount = _pScoreOld ? _pScoreOld->playcount + 1 : 1;
            auto isclear = State::get(IndexSwitch::RESULT_CLEAR) ? 1 : 0;
            score.clearcount = _pScoreOld ? _pScoreOld->clearcount + isclear : isclear;

            std::stringstream dbReplayFile;
            for (size_t i = 0; i < gPlayContext.courseStageReplayPathNew.size(); ++i)
            {
                if (i != 0)
                    dbReplayFile << "|";
                dbReplayFile << gPlayContext.courseStageReplayPathNew[i];
            }
            score.replayFileName = dbReplayFile.str();

            score.score = summary[ARG_SCORE];
            score.exscore = summary[ARG_EXSCORE];
            score.fast = summary[ARG_FAST];
            score.slow = summary[ARG_SLOW];

            if (gPlayContext.mods[PLAYER_SLOT_PLAYER].assist_mask == 0)
            {
                score.pgreat = summary[ARG_JUDGE_0];
                score.great = summary[ARG_JUDGE_1];
                score.good = summary[ARG_JUDGE_2];
                score.bad = summary[ARG_JUDGE_3];
                score.kpoor = summary[ARG_JUDGE_4];
                score.miss = summary[ARG_JUDGE_5];
                score.bp = summary[ARG_BP];
                score.combobreak = summary[ARG_CB];
            }

            score.lamp = ScoreBMS::Lamp::NOPLAY;
            if (isclear)
            {
                if (summary[ARG_MAXCOMBO] == summary[ARG_TOTAL_NOTES])
                    score.lamp = ScoreBMS::Lamp::FULLCOMBO;
                else if (gPlayContext.mods[PLAYER_SLOT_PLAYER].gauge == PlayModifierGaugeType::GRADE_HARD)
                    score.lamp = ScoreBMS::Lamp::HARD;
                else if (gPlayContext.mods[PLAYER_SLOT_PLAYER].gauge == PlayModifierGaugeType::GRADE_NORMAL)
                    score.lamp = ScoreBMS::Lamp::NORMAL;
            }
            else
            {
                score.lamp = ScoreBMS::Lamp::FAILED;
            }

            //
            g_pScoreDB->updateCourseScoreBMS(gPlayContext.courseHash, score);
        }

        clearContextPlay();
        gPlayContext.courseStageRulesetCopy[0].clear();
        gPlayContext.courseStageRulesetCopy[1].clear();
        gPlayContext.courseStageReplayPathNew.clear();
        gPlayContext.isAuto = false;
        gPlayContext.isReplay = false;
        gNextScene = gQuitOnFinish ? SceneType::EXIT_TRANS : SceneType::SELECT;
    }
}

////////////////////////////////////////////////////////////////////////////////

// CALLBACK
void SceneCourseResult::inputGamePress(InputMask& m, const lunaticvibes::Time& t)
{
    if (t - State::get(IndexTimer::SCENE_START) < pSkin->info.timeIntro)
        return;

    if ((_inputAvailable & m & (INPUT_MASK_DECIDE | INPUT_MASK_CANCEL)).any() || m[Input::ESC])
    {
        switch (state)
        {
        case eCourseResultState::DRAW:
            State::set(IndexTimer::RESULT_RANK_START, t.norm());
            // TODO play hit sound
            state = eCourseResultState::STOP;
            LOG_DEBUG << "[Result] State changed to STOP";
            break;

        case eCourseResultState::STOP:
            if (!gPlayContext.isBattle && !gPlayContext.isReplay)
            {
                State::set(IndexTimer::RESULT_HIGHSCORE_START, t.norm());
                // TODO stop result sound
                // TODO play record sound
                state = eCourseResultState::RECORD;
                LOG_DEBUG << "[Result] State changed to RECORD";
            }
            else
            {
                State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
                state = eCourseResultState::FADEOUT;
                SoundMgr::setSysVolume(0.0, 2000);
                SoundMgr::setNoteVolume(0.0, 2000);
                LOG_DEBUG << "[Result] State changed to FADEOUT";
            }
            break;

        case eCourseResultState::RECORD:
            if (_scoreSyncFinished)
            {
                State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
                state = eCourseResultState::FADEOUT;
                SoundMgr::setSysVolume(0.0, 2000);
                SoundMgr::setNoteVolume(0.0, 2000);
                LOG_DEBUG << "[Result] State changed to FADEOUT";
            }
            break;

        case eCourseResultState::FADEOUT:
        default: break;
        }
    }
}

// CALLBACK
void SceneCourseResult::inputGameHold(InputMask& m, const lunaticvibes::Time& t)
{
    if (t - State::get(IndexTimer::SCENE_START) < pSkin->info.timeIntro)
        return;

    if (state == eCourseResultState::FADEOUT)
    {
        _retryRequested =
            (_inputAvailable & m & INPUT_MASK_DECIDE).any() && (_inputAvailable & m & INPUT_MASK_CANCEL).any();
    }
}

// CALLBACK
void SceneCourseResult::inputGameRelease(InputMask& m, const lunaticvibes::Time& t)
{
    if (t - State::get(IndexTimer::SCENE_START) < pSkin->info.timeIntro)
        return;
}
