#include "skin_lr2_bargraph.h"

#include <common/keymap.h>
#include <game/arena/arena_data.h>
#include <game/ruleset/ruleset_bms.h>
#include <game/ruleset/ruleset_bms_auto.h>
#include <game/scene/scene_context.h>
#include <game/scene/scene_play.h>

static double s_test1{};
void lunaticvibes::set_bargraph_test1(double val)
{
    s_test1 = val;
}

double lunaticvibes::get_bargraph(IndexBargraph ind)
{
    if (static_cast<int>(ind) >= static_cast<int>(IndexBargraph::ARENA_PLAYDATA_BASE) &&
        static_cast<int>(ind) <= static_cast<int>(IndexBargraph::ARENA_PLAYDATA_ALL_MAX))
    {
        const auto player = static_cast<unsigned>(ind) / 10 - 10;
        const auto ruleset = gArenaData.getPlayerRuleset(player);
        if (!ruleset)
            return 0.;
        const auto bargraph = static_cast<unsigned>(ind) % 10;
        const auto index_by_base =
            static_cast<IndexBargraph>((static_cast<unsigned>(IndexBargraph::ARENA_PLAYDATA_BASE) + bargraph));
        if (index_by_base == IndexBargraph::ARENA_PLAYDATA_EXSCORE)
            return ruleset->getData().total_acc / 100.;
        if (index_by_base == IndexBargraph::ARENA_PLAYDATA_EXSCORE_PREDICT)
            return ruleset->getData().acc / 100.;
        return 0.;
    }

    switch (ind)
    {
    case IndexBargraph::ZERO: return 0.;
    case IndexBargraph::MUSIC_PROGRESS: break;
    case IndexBargraph::MUSIC_LOAD_PROGRESS:
        return (lunaticvibes::getWavLoadProgress() + lunaticvibes::getBgaLoadProgress()) / 2.;
    case IndexBargraph::LEVEL_BAR: break;
    case IndexBargraph::LEVEL_BAR_BEGINNER: return gSelectContext.bargraph_level_bar_beginner;
    case IndexBargraph::LEVEL_BAR_NORMAL: return gSelectContext.bargraph_level_bar_normal;
    case IndexBargraph::LEVEL_BAR_HYPER: return gSelectContext.bargraph_level_bar_hyper;
    case IndexBargraph::LEVEL_BAR_ANOTHER: return gSelectContext.bargraph_level_bar_another;
    case IndexBargraph::LEVEL_BAR_INSANE: return gSelectContext.bargraph_level_bar_insane;
    case IndexBargraph::PLAY_EXSCORE:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset && !gArenaData.isOnline())
            return ruleset->getData().total_acc / 100.0;
        break;
    case IndexBargraph::PLAY_EXSCORE_PREDICT:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset && !gArenaData.isOnline())
            return ruleset->getData().acc / 100.0;
        break;
    case IndexBargraph::PLAY_MYBEST_NOW:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_MYBEST]; ruleset && !gArenaData.isOnline())
            return ruleset->getData().total_acc / 100.0;
        return gPlayContext.bargraph_mybest_now_fallback;
    case IndexBargraph::PLAY_MYBEST_FINAL: return gPlayContext.bargraph_mybest_final;
    case IndexBargraph::PLAY_RIVAL_EXSCORE:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset && !gArenaData.isOnline())
            return ruleset->getData().total_acc / 100.0;
        break;
    case IndexBargraph::PLAY_RIVAL_EXSCORE_FINAL:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMSAuto>(gPlayContext.ruleset[PLAYER_SLOT_TARGET]); ruleset)
            return ruleset->getTargetRate();
        break;
    case IndexBargraph::PLAY_EXSCORE_BACKUP:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset && !gArenaData.isOnline())
            return ruleset->getData().total_acc / 100.0;
        break;
    case IndexBargraph::PLAY_RIVAL_EXSCORE_BACKUP:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return ruleset->getData().total_acc / 100.0;
        break;
    case IndexBargraph::RESULT_PG:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_PERFECT]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_GR:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_GREAT]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_GD:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_GOOD]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_BD:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_BAD]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_PR:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_POOR]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_MAXCOMBO:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<double>(ruleset->getData().maxCombo) / ruleset->getMaxCombo();
        break;
    case IndexBargraph::RESULT_SCORE:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); ruleset)
            return ruleset->getScore() / ruleset->getMaxMoneyScore();
        break;
    case IndexBargraph::RESULT_EXSCORE:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); ruleset)
            return static_cast<double>(ruleset->getExScore()) / ruleset->getMaxScore();
        break;
    case IndexBargraph::RESULT_RIVAL_PG:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_PERFECT]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_RIVAL_GR:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_GREAT]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_RIVAL_GD:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_GOOD]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_RIVAL_BD:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_BAD]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_RIVAL_PR:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_POOR]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::RESULT_RIVAL_MAXCOMBO:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return static_cast<double>(ruleset->getData().maxCombo) / ruleset->getMaxCombo();
        break;
    case IndexBargraph::RESULT_RIVAL_SCORE:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_TARGET]); ruleset)
            return ruleset->getScore() / ruleset->getMaxMoneyScore();
        break;
    case IndexBargraph::RESULT_RIVAL_EXSCORE:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_TARGET]); ruleset)
            return static_cast<double>(ruleset->getExScore()) / ruleset->getMaxScore();
        break;
    case IndexBargraph::SELECT_MYBEST_PG: return gSelectContext.bargraph_select_mybest_pg;
    case IndexBargraph::SELECT_MYBEST_GR: return gSelectContext.bargraph_select_mybest_gr;
    case IndexBargraph::SELECT_MYBEST_GD: return gSelectContext.bargraph_select_mybest_gd;
    case IndexBargraph::SELECT_MYBEST_BD: return gSelectContext.bargraph_select_mybest_bd;
    case IndexBargraph::SELECT_MYBEST_PR: return gSelectContext.bargraph_select_mybest_pr;
    case IndexBargraph::SELECT_MYBEST_MAXCOMBO: return gSelectContext.bargraph_select_mybest_maxcombo;
    case IndexBargraph::SELECT_MYBEST_SCORE: return gSelectContext.bargraph_select_mybest_score;
    case IndexBargraph::SELECT_MYBEST_EXSCORE: return gSelectContext.bargraph_select_mybest_exscore;
    case IndexBargraph::PLAY_1P_SLOW_COUNT:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_LATE]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::PLAY_1P_FAST_COUNT:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_EARLY]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::MINBPM_LANECOVER:
    case IndexBargraph::MAXBPM_LANECOVER:
    case IndexBargraph::PLAY_2P_SLOW_COUNT:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_LATE]) / ruleset->getNoteCount();
        break;
    case IndexBargraph::PLAY_2P_FAST_COUNT:
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_TARGET]; ruleset)
            return static_cast<double>(ruleset->getData().judge[RulesetBMS::JUDGE_EARLY]) / ruleset->getNoteCount();
        break;
    // `case IndexBargraph::FORCE_\(.*\):/case IndexBargraph::FORCE_\1: return
    // gKeyconfigContext.bargraphForce[Input::Pad::\1];`
    case IndexBargraph::FORCE_K11: return gKeyconfigContext.bargraphForce[Input::Pad::K11];
    case IndexBargraph::FORCE_K12: return gKeyconfigContext.bargraphForce[Input::Pad::K12];
    case IndexBargraph::FORCE_K13: return gKeyconfigContext.bargraphForce[Input::Pad::K13];
    case IndexBargraph::FORCE_K14: return gKeyconfigContext.bargraphForce[Input::Pad::K14];
    case IndexBargraph::FORCE_K15: return gKeyconfigContext.bargraphForce[Input::Pad::K15];
    case IndexBargraph::FORCE_K16: return gKeyconfigContext.bargraphForce[Input::Pad::K16];
    case IndexBargraph::FORCE_K17: return gKeyconfigContext.bargraphForce[Input::Pad::K17];
    case IndexBargraph::FORCE_K18: return gKeyconfigContext.bargraphForce[Input::Pad::K18];
    case IndexBargraph::FORCE_K19: return gKeyconfigContext.bargraphForce[Input::Pad::K19];
    case IndexBargraph::FORCE_S1L: return gKeyconfigContext.bargraphForce[Input::Pad::S1L];
    case IndexBargraph::FORCE_S1R: return gKeyconfigContext.bargraphForce[Input::Pad::S1R];
    case IndexBargraph::FORCE_K1Start: return gKeyconfigContext.bargraphForce[Input::Pad::K1START];
    case IndexBargraph::FORCE_K1Select: return gKeyconfigContext.bargraphForce[Input::Pad::K1SELECT];
    case IndexBargraph::FORCE_K21: return gKeyconfigContext.bargraphForce[Input::Pad::K21];
    case IndexBargraph::FORCE_K22: return gKeyconfigContext.bargraphForce[Input::Pad::K22];
    case IndexBargraph::FORCE_K23: return gKeyconfigContext.bargraphForce[Input::Pad::K23];
    case IndexBargraph::FORCE_K24: return gKeyconfigContext.bargraphForce[Input::Pad::K24];
    case IndexBargraph::FORCE_K25: return gKeyconfigContext.bargraphForce[Input::Pad::K25];
    case IndexBargraph::FORCE_K26: return gKeyconfigContext.bargraphForce[Input::Pad::K26];
    case IndexBargraph::FORCE_K27: return gKeyconfigContext.bargraphForce[Input::Pad::K27];
    case IndexBargraph::FORCE_K28: return gKeyconfigContext.bargraphForce[Input::Pad::K28];
    case IndexBargraph::FORCE_K29: return gKeyconfigContext.bargraphForce[Input::Pad::K29];
    case IndexBargraph::FORCE_S2L: return gKeyconfigContext.bargraphForce[Input::Pad::S2L];
    case IndexBargraph::FORCE_S2R: return gKeyconfigContext.bargraphForce[Input::Pad::S2R];
    case IndexBargraph::FORCE_K2Start: return gKeyconfigContext.bargraphForce[Input::Pad::K2START];
    case IndexBargraph::FORCE_K2Select: return gKeyconfigContext.bargraphForce[Input::Pad::K2SELECT];
    case IndexBargraph::ARENA_PLAYDATA_EXSCORE:
    case IndexBargraph::ARENA_PLAYDATA_EXSCORE_PREDICT:
    case IndexBargraph::ARENA_PLAYDATA_MAX:
    case IndexBargraph::ARENA_PLAYDATA_ALL_MAX: break; // Arena things are handled separately above.
    case IndexBargraph::MUSIC_LOAD_PROGRESS_SYS: return lunaticvibes::getSysLoadProgress();
    case IndexBargraph::MUSIC_LOAD_PROGRESS_WAV: return lunaticvibes::getWavLoadProgress();
    case IndexBargraph::MUSIC_LOAD_PROGRESS_BGA: return lunaticvibes::getBgaLoadProgress();
    case IndexBargraph::_TEST1: return s_test1;
    }
    return 0.;
};
