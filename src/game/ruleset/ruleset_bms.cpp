#include "ruleset_bms.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <utility>

#include <common/assert.h>
#include <common/sysutil.h>
#include <common/types.h>
#include <config/config_mgr.h>
#include <game/chart/chart_bms.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>
#include <game/sound/sound_mgr.h>
#include <game/sound/sound_sample.h>

using namespace chart;

void setJudgeInternalTimer1P(RulesetBMS::JudgeType judge, long long t)
{
    State::set(IndexTimer::_JUDGE_1P_0, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_1P_1, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_1P_2, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_1P_3, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_1P_4, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_1P_5, TIMER_NEVER);
    switch (judge)
    {
    case RulesetBMS::JudgeType::KPOOR: State::set(IndexTimer::_JUDGE_1P_0, t); break;
    case RulesetBMS::JudgeType::MISS: State::set(IndexTimer::_JUDGE_1P_1, t); break;
    case RulesetBMS::JudgeType::BAD: State::set(IndexTimer::_JUDGE_1P_2, t); break;
    case RulesetBMS::JudgeType::GOOD: State::set(IndexTimer::_JUDGE_1P_3, t); break;
    case RulesetBMS::JudgeType::GREAT: State::set(IndexTimer::_JUDGE_1P_4, t); break;
    case RulesetBMS::JudgeType::PERFECT: State::set(IndexTimer::_JUDGE_1P_5, t); break;
    default: break;
    }
}

void setJudgeInternalTimer2P(RulesetBMS::JudgeType judge, long long t)
{
    State::set(IndexTimer::_JUDGE_2P_0, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_2P_1, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_2P_2, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_2P_3, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_2P_4, TIMER_NEVER);
    State::set(IndexTimer::_JUDGE_2P_5, TIMER_NEVER);
    switch (judge)
    {
    case RulesetBMS::JudgeType::KPOOR: State::set(IndexTimer::_JUDGE_2P_0, t); break;
    case RulesetBMS::JudgeType::MISS: State::set(IndexTimer::_JUDGE_2P_1, t); break;
    case RulesetBMS::JudgeType::BAD: State::set(IndexTimer::_JUDGE_2P_2, t); break;
    case RulesetBMS::JudgeType::GOOD: State::set(IndexTimer::_JUDGE_2P_3, t); break;
    case RulesetBMS::JudgeType::GREAT: State::set(IndexTimer::_JUDGE_2P_4, t); break;
    case RulesetBMS::JudgeType::PERFECT: State::set(IndexTimer::_JUDGE_2P_5, t); break;
    default: break;
    }
}

static RulesetBMS::GaugeType get_gauge(PlayModifierGaugeType gauge)
{
    using P = PlayModifierGaugeType;
    using G = RulesetBMS::GaugeType;
    switch (gauge)
    {
    case P::NORMAL: return G::GROOVE;
    case P::HARD: return G::HARD;
    case P::DEATH: return G::DEATH;
    case P::EASY: return G::EASY;
    // TODO: check these.
    case P::PATTACK:
    case P::GATTACK: return G::GROOVE;
    // case P::PATTACK: return G::P_ATK;
    // case P::GATTACK: return G::G_ATK;
    case P::EXHARD: return G::EXHARD;
    case P::ASSISTEASY: return G::ASSIST;
    case P::GRADE_NORMAL: return G::GRADE;
    case P::GRADE_DEATH:
    case P::GRADE_HARD: return G::EXGRADE;
    }
    lunaticvibes::assert_failed("invalid PlayModifierGaugeType");
}

static double calculateHardNegativeHpDiffMultiplier(unsigned total, unsigned note_count)
{
    double by_total = 1.0;
    if (total >= 240)
        ;
    else if (total >= 230)
        by_total = 10.0 / 9;
    else if (total >= 210)
        by_total = 1.25;
    else if (total >= 200)
        by_total = 1.5;
    else if (total >= 180)
        by_total = 5.0 / 3;
    else if (total >= 160)
        by_total = 2.0;
    else if (total >= 150)
        by_total = 2.5;
    else if (total >= 130)
        by_total = 10.0 / 3;
    else if (total >= 120)
        by_total = 5.0;
    else
        by_total = 10.0;

    double by_notes = 1.0;
    if (note_count >= 1000)
        ;
    else if (note_count >= 500)
        by_notes = (note_count - 500) * 0.002;
    else if (note_count >= 250)
        by_notes = 1.0 + (note_count - 250) * 0.004;
    else if (note_count >= 125)
        by_notes = 2.0 + (note_count - 125) * 0.008;
    else if (note_count >= 62)
        by_notes = 3.0 + (note_count - 62) * (1.0 / 62);
    else if (note_count >= 31)
        by_notes = 4.0 + (note_count - 31) * (1.0 / 31);
    else if (note_count >= 16)
        by_notes = 5.0 + (note_count - 16) * 0.0625;
    else if (note_count >= 8)
        by_notes = 6.0 + (note_count - 8) * 0.125;
    else if (note_count >= 4)
        by_notes = 7.0 + (note_count - 4) * 0.25;
    else if (note_count >= 2)
        by_notes = 8.0 + (note_count - 2) * 0.50;
    else if (note_count == 1)
        by_notes = 9.0;
    else
        by_notes = 10.0;

    return std::max(by_total, by_notes);
}

static unsigned getDefaultTotal(RulesetBMS::GaugeType gauge)
{
    switch (gauge)
    {
        using enum RulesetBMS::GaugeType;
    case HARD:
    case EXHARD:
    case DEATH:
    case GRADE:
    case EXGRADE: return 300;
    case P_ATK: // ?
    case G_ATK: // ?
    case GROOVE:
    case EASY:
    case ASSIST: return 160;
    }
    lunaticvibes::assert_failed("getDefaultTotal");
}

static lunaticvibes::Lr2GaugeIncrements getGauge(RulesetBMS::GaugeType type, int total, unsigned note_count)
{
    // Older reference: https://github.com/aeventyr/LR2GAS_pub
    // Health gain later updated from LR2 disassembly project.
    // HARD and GATTACK have additional multipliers applied elsewhere.
    //
    // ASSISTEASY and EXHARD match lr2oraja.
    // https://github.com/wcko87/lr2oraja/blob/4e9df9a5394561a1c24fe48d52c1c6539de559f3/src/bms/player/beatoraja/play/GaugeProperty.java

    // NOTE: LR2 handles #TOTAL 0 as if total was not set.
    if (total <= 0)
        total = getDefaultTotal(type);

    switch (type)
    {
        using enum RulesetBMS::GaugeType;
    case GROOVE:
        return {.health_gain =
                    {
                        0.01 * total / note_count,     // PG
                        0.01 * total / note_count,     // GR
                        0.01 * total / note_count / 2, // GD
                        -0.04,                         // BD
                        -0.02,                         // KPR
                        -0.06,                         // MISS
                    },
                .start_health = 0.2,
                .min_health = 0.02,
                .clear_health = 0.8,
                .type = type,
                .fail_no_health = false};
    case EASY:
        return {.health_gain =
                    {
                        0.01 * total / note_count * 1.2,
                        0.01 * total / note_count * 1.2,
                        0.01 * total / note_count / 2 * 1.2,
                        -0.032,
                        -0.016,
                        -0.04800000000000001,
                    },
                .start_health = 0.2,
                .min_health = 0.02,
                .clear_health = 0.8,
                .type = type,
                .fail_no_health = false};
    case ASSIST:
        return {.health_gain =
                    {
                        0.01 * total / note_count * 1.2,
                        0.01 * total / note_count * 1.2,
                        0.01 * total / note_count / 2 * 1.2,
                        -0.032,
                        -0.016,
                        -0.048,
                    },
                .start_health = 0.2,
                .min_health = 0.02,
                .clear_health = 0.6,
                .type = type,
                .fail_no_health = false};
    case HARD:
        return {.health_gain =
                    {
                        0.001,
                        0.001,
                        0.001 / 2,
                        -0.06,
                        -0.02,
                        -0.1,
                    },
                .start_health = 1.0,
                .min_health = 0,
                .clear_health = 0,
                .hp_lose_multiplier = calculateHardNegativeHpDiffMultiplier(total, note_count),
                .type = type,
                .fail_no_health = true,
                .reduce_below_30_hp_damage = true};
    case EXHARD:
        return {.health_gain =
                    {
                        0.001,
                        0.001,
                        0.001 / 2,
                        -0.12,
                        -0.02,
                        -0.2,
                    },
                .start_health = 1.0,
                .min_health = 0,
                .clear_health = 0,
                .hp_lose_multiplier = calculateHardNegativeHpDiffMultiplier(total, note_count),
                .type = type,
                .fail_no_health = true};
    case DEATH:
        return {.health_gain =
                    {
                        0.0,
                        0.0,
                        0.0,
                        -1.0,
                        0.0,
                        -1.0,
                    },
                .start_health = 1.0,
                .min_health = 0,
                .clear_health = 0,
                .type = type,
                .fail_no_health = true};
    case P_ATK:
        return {.health_gain =
                    {
                        0.001,
                        -0.01,
                        -1.0,
                        -1.0,
                        -1.0,
                        -1.0,
                    },
                .start_health = 1.0,
                .min_health = 0,
                .clear_health = 0,
                .type = type,
                .fail_no_health = true};
    case G_ATK:
        return {.health_gain =
                    {
                        -0.1,
                        -0.01,
                        0.001,
                        -0.06,
                        -0.02,
                        -0.1,
                    },
                .start_health = 1.0,
                .min_health = 0,
                .clear_health = 0,
                .type = type,
                .fail_no_health = true};
    case GRADE:
        return {.health_gain =
                    {
                        0.001,
                        0.001,
                        0.0004,
                        -0.02,
                        -0.02,
                        -0.03,
                    },
                .start_health = 1.0,
                .min_health = 0,
                .clear_health = 0,
                .type = type,
                .fail_no_health = true,
                .reduce_below_30_hp_damage = true};
    case EXGRADE:
        return {.health_gain =
                    {
                        0.001,
                        0.001,
                        0.0004,
                        -0.06,
                        -0.2,
                        -0.1,
                    },
                .start_health = 1.0,
                .min_health = 0,
                .clear_health = 0,
                .type = type,
                .fail_no_health = true};
    }
    lunaticvibes::assert_failed("getGauge");
}

static int getTotal(const std::shared_ptr<ChartFormatBase>& format)
{
    switch (format->type())
    {
    case eChartFormat::BMS: return std::reinterpret_pointer_cast<ChartFormatBMSMeta>(format)->total;
    case eChartFormat::UNKNOWN:
    case eChartFormat::BMSON: return -1;
    }
    lunaticvibes::assert_failed("getTotal");
}

namespace lunaticvibes
{

void GaugeHolder::feed(RulesetBMS::JudgeArea judge)
{
    // LOG_VERBOSE << "[RulesetBMS] GaugeHolder::feed " << static_cast<int>(judge);
    if (_did_fail)
        return;
    process(_gauge.health_gain[static_cast<unsigned>(RulesetBMS::JudgeAreaTypeMap.at(judge))]);
}

void GaugeHolder::feed_mine(long long mine_value)
{
    if (_did_fail)
        return;
    process(-0.01 * mine_value / 2);
}

std::ostream& operator<<(std::ostream& os, const BmsGaugeType& type)
{
    switch (type)
    {
    case BmsGaugeType::GROOVE: return os << "GROOVE";
    case BmsGaugeType::EASY: return os << "EASY";
    case BmsGaugeType::ASSIST: return os << "ASSIST";
    case BmsGaugeType::HARD: return os << "HARD";
    case BmsGaugeType::EXHARD: return os << "EXHARD";
    case BmsGaugeType::DEATH: return os << "DEATH";
    case BmsGaugeType::P_ATK: return os << "P_ATK";
    case BmsGaugeType::G_ATK: return os << "G_ATK";
    case BmsGaugeType::GRADE: return os << "GRADE";
    case BmsGaugeType::EXGRADE: return os << "EXGRADE";
    }
    lunaticvibes::assert_failed("operator<<(BmsGaugeType)");
}

void GaugeHolder::update_for_show(RulesetBMS& ruleset)
{
    // LOG_VERBOSE << "GaugeHolder update_for_show side=" << (int)ruleset._side << " gauge=" << _gauge.type;
    // >>>> copy-paste this to initGaugeParams :(
    ruleset._basic.health = _health.to;
    ruleset._clearHealth = _gauge.clear_health;
    ruleset._failWhenNoHealth = _gauge.fail_no_health;
    ruleset._minHealth = _gauge.min_health;
    if (_did_fail && !ruleset._isFailed)
    {
        LOG_DEBUG << "[RulesetBMS] Gauge failed: " << _gauge.type;
        ruleset.fail();
    }
}

void GaugeHolder::process(double diff)
{
    // LOG_VERBOSE << "[GaugeHolder] <- process, current=" << _health.to << " diff=" << diff;
    LVF_DEBUG_ASSERT(!_did_fail);

    // TOTAL補正, totalnotes補正
    // ref: https://web.archive.org/web/20150226213104/http://2nd.geocities.jp/yoshi_65c816/bms/LR2.html
    // TODO: instead match https://github.com/aeventyr/LR2GAS_pub/blob/main/src/gas.cpp and also check LR2
    // disassembly.
    if (diff < 0)
        diff *= _gauge.hp_lose_multiplier;

    if (_gauge.reduce_below_30_hp_damage && _health.to < 0.32 && diff < 0.0)
        diff *= 0.6;

    // TODO: verify the animation. Try with hard gauge something where one miss makes you lose like 90% HP.
    _health.from = _health.to;
    const double new_hp_tmp = _health.to + diff;
    const auto new_hp = std::max(_gauge.min_health, std::min(1.0, new_hp_tmp));
    if (_health.from != new_hp)
    {
        auto get_lr2_hp_animation_time = [](double from, double to) {
            return std::abs(static_cast<int>(from - to)) * 10;
        };
        _health.from = _health.animate(lunaticvibes::Time::now());
        _health.to = new_hp;
        _health.start = lunaticvibes::Time::now();
        _health.end = _health.start + get_lr2_hp_animation_time(_health.from, _health.to);
    }

    if (_gauge.fail_no_health && _health.to <= _gauge.min_health)
    {
        LOG_DEBUG << "[GaugeHolder] Gauge failed: " << _gauge.type;
        _did_fail = true;
    }

    // LOG_VERBOSE << "[GaugeHolder] -> process, new=" << _health.to;
}

void GaugeHolderProxy::feed(BmsJudgeArea judge)
{
    for (auto& gauge : _gauges)
        gauge.feed(judge);
}

void GaugeHolderProxy::feed_mine(long long mine_value)
{
    for (auto& gauge : _gauges)
        gauge.feed_mine(mine_value);
}

void GaugeHolderProxy::update_for_show(RulesetBMS& ruleset)
{
    // LOG_VERBOSE << "GaugeHolderProxy update_for_show";
    current_gauge().update_for_show(ruleset);
}

const Lr2GaugeIncrements& GaugeHolderProxy::get_gauge() const
{
    return current_gauge().get_gauge();
}

const NumberAnimation& GaugeHolderProxy::get_health() const
{
    return current_gauge().get_health();
}

} // namespace lunaticvibes

static const char* randomTypeStringForShow(PlayModifierRandomType type)
{
    switch (type)
    {
    case PlayModifierRandomType::NONE: return "NORMAL";
    case PlayModifierRandomType::MIRROR: return "MIRROR";
    case PlayModifierRandomType::RANDOM: return "RANDOM";
    case PlayModifierRandomType::SRAN: return "S-RANDOM";
    case PlayModifierRandomType::HRAN: return "H-RANDOM";
    case PlayModifierRandomType::ALLSCR: return "ALL-SCR";
    case PlayModifierRandomType::RRAN: return "R-RANDOM";
    case PlayModifierRandomType::DB_SYNCHRONIZE: return "SYNCHRONIZE";
    case PlayModifierRandomType::DB_SYMMETRY: return "SYMMETRY";
    }
    lunaticvibes::assert_failed("playModifierStringForShow");
}

static std::string makeModsStringForShow(RulesetBMS::PlaySide side, PlayModifiers mods)
{
    auto push_part = [](std::stringstream& os, auto part) {
        if (!os.str().empty()) // woah there, good thing we are calling this .. thrice per play
            os << ", ";
        os << part;
    };
    std::stringstream os;
    switch (side)
    {
    case RulesetBMS::PlaySide::SINGLE:
    case RulesetBMS::PlaySide::BATTLE_1P:
    case RulesetBMS::PlaySide::AUTO:
        if (mods.randomLeft != PlayModifierRandomType::NONE)
            os << randomTypeStringForShow(mods.randomLeft);
        if (mods.assist_mask & PLAY_MOD_ASSIST_AUTOSCR)
            push_part(os, "AUTO-SCR");
        break;

    case RulesetBMS::PlaySide::RIVAL:
    case RulesetBMS::PlaySide::BATTLE_2P:
    case RulesetBMS::PlaySide::AUTO_2P:
        if (mods.randomRight != PlayModifierRandomType::NONE)
            os << randomTypeStringForShow(mods.randomRight);
        if (mods.assist_mask & PLAY_MOD_ASSIST_AUTOSCR)
            push_part(os, "AUTO-SCR");
        break;

    case RulesetBMS::PlaySide::DOUBLE:
    case RulesetBMS::PlaySide::AUTO_DOUBLE:
        if (mods.randomLeft != PlayModifierRandomType::NONE && mods.randomRight != PlayModifierRandomType::NONE)
        {
            os << (mods.randomLeft == PlayModifierRandomType::NONE ? "-" : randomTypeStringForShow(mods.randomLeft))
               << "/"
               << (mods.randomRight == PlayModifierRandomType::NONE ? "-" : randomTypeStringForShow(mods.randomRight));
        }
        if (mods.DPFlip)
            push_part(os, "FLIP");
        if (mods.assist_mask & PLAY_MOD_ASSIST_AUTOSCR)
            push_part(os, "AUTO-SCR");
        break;

    case RulesetBMS::PlaySide::MYBEST:
    case RulesetBMS::PlaySide::NETWORK: break;
    }

    std::string out = os.str();
    if (out.empty())
        out = "NONE";
    return out;
}

RulesetBMS::RulesetBMS(std::shared_ptr<ChartFormatBase> format, std::shared_ptr<ChartObjectBase> chart,
                       PlayModifiers mods, GameModeKeys keys, JudgeDifficulty difficulty, double health,
                       RulesetBMS::PlaySide side, const int fiveKeyMapIndex,
                       std::shared_ptr<PlayContextParams::MutexReplayChart> replayNew)
    : RulesetBase(std::move(format), std::move(chart)),
      _gaugeProc(std::array{
          lunaticvibes::GaugeHolder{getGauge(GaugeType::GROOVE, /*total*/ 0, /*note_count=*/0), _basic.health}}),
      _judgeDifficulty(difficulty), _replayNew(std::move(replayNew))
{
    if (_replayNew)
    {
        LVF_DEBUG_ASSERT(_replayNew->replay != nullptr);
    }

    static const NoteLaneTimerMap bombTimer5k[] = {
        {{
            {chart::Sc1, IndexTimer::S1_BOMB},
            {chart::N11, IndexTimer::K11_BOMB},
            {chart::N12, IndexTimer::K12_BOMB},
            {chart::N13, IndexTimer::K13_BOMB},
            {chart::N14, IndexTimer::K14_BOMB},
            {chart::N15, IndexTimer::K15_BOMB},
            {chart::N21, IndexTimer::K21_BOMB},
            {chart::N22, IndexTimer::K22_BOMB},
            {chart::N23, IndexTimer::K23_BOMB},
            {chart::N24, IndexTimer::K24_BOMB},
            {chart::N25, IndexTimer::K25_BOMB},
            {chart::Sc2, IndexTimer::S2_BOMB},
        }},
        {{
            {chart::Sc1, IndexTimer::S1_BOMB},
            {chart::N11, IndexTimer::K11_BOMB},
            {chart::N12, IndexTimer::K12_BOMB},
            {chart::N13, IndexTimer::K13_BOMB},
            {chart::N14, IndexTimer::K14_BOMB},
            {chart::N15, IndexTimer::K15_BOMB},
            {chart::N21, IndexTimer::K23_BOMB},
            {chart::N22, IndexTimer::K24_BOMB},
            {chart::N23, IndexTimer::K25_BOMB},
            {chart::N24, IndexTimer::K26_BOMB},
            {chart::N25, IndexTimer::K27_BOMB},
            {chart::Sc2, IndexTimer::S2_BOMB},
        }},
        {{
            {chart::Sc1, IndexTimer::S1_BOMB},
            {chart::N11, IndexTimer::K13_BOMB},
            {chart::N12, IndexTimer::K14_BOMB},
            {chart::N13, IndexTimer::K15_BOMB},
            {chart::N14, IndexTimer::K16_BOMB},
            {chart::N15, IndexTimer::K17_BOMB},
            {chart::N21, IndexTimer::K21_BOMB},
            {chart::N22, IndexTimer::K22_BOMB},
            {chart::N23, IndexTimer::K23_BOMB},
            {chart::N24, IndexTimer::K24_BOMB},
            {chart::N25, IndexTimer::K25_BOMB},
            {chart::Sc2, IndexTimer::S2_BOMB},
        }},
        {{
            {chart::Sc1, IndexTimer::S1_BOMB},
            {chart::N11, IndexTimer::K13_BOMB},
            {chart::N12, IndexTimer::K14_BOMB},
            {chart::N13, IndexTimer::K15_BOMB},
            {chart::N14, IndexTimer::K16_BOMB},
            {chart::N15, IndexTimer::K17_BOMB},
            {chart::N21, IndexTimer::K23_BOMB},
            {chart::N22, IndexTimer::K24_BOMB},
            {chart::N23, IndexTimer::K25_BOMB},
            {chart::N24, IndexTimer::K26_BOMB},
            {chart::N25, IndexTimer::K27_BOMB},
            {chart::Sc2, IndexTimer::S2_BOMB},
        }},
    };

    static const NoteLaneTimerMap bombTimer5kLN[] = {
        {{
            {chart::Sc1, IndexTimer::S1_LN_BOMB},
            {chart::N11, IndexTimer::K11_LN_BOMB},
            {chart::N12, IndexTimer::K12_LN_BOMB},
            {chart::N13, IndexTimer::K13_LN_BOMB},
            {chart::N14, IndexTimer::K14_LN_BOMB},
            {chart::N15, IndexTimer::K15_LN_BOMB},
            {chart::N21, IndexTimer::K21_LN_BOMB},
            {chart::N22, IndexTimer::K22_LN_BOMB},
            {chart::N23, IndexTimer::K23_LN_BOMB},
            {chart::N24, IndexTimer::K24_LN_BOMB},
            {chart::N25, IndexTimer::K25_LN_BOMB},
            {chart::Sc2, IndexTimer::S2_LN_BOMB},
        }},
        {{
            {chart::Sc1, IndexTimer::S1_LN_BOMB},
            {chart::N11, IndexTimer::K11_LN_BOMB},
            {chart::N12, IndexTimer::K12_LN_BOMB},
            {chart::N13, IndexTimer::K13_LN_BOMB},
            {chart::N14, IndexTimer::K14_LN_BOMB},
            {chart::N15, IndexTimer::K15_LN_BOMB},
            {chart::N21, IndexTimer::K23_LN_BOMB},
            {chart::N22, IndexTimer::K24_LN_BOMB},
            {chart::N23, IndexTimer::K25_LN_BOMB},
            {chart::N24, IndexTimer::K26_LN_BOMB},
            {chart::N25, IndexTimer::K27_LN_BOMB},
            {chart::Sc2, IndexTimer::S2_LN_BOMB},
        }},
        {{
            {chart::Sc1, IndexTimer::S1_LN_BOMB},
            {chart::N11, IndexTimer::K13_LN_BOMB},
            {chart::N12, IndexTimer::K14_LN_BOMB},
            {chart::N13, IndexTimer::K15_LN_BOMB},
            {chart::N14, IndexTimer::K16_LN_BOMB},
            {chart::N15, IndexTimer::K17_LN_BOMB},
            {chart::N21, IndexTimer::K21_LN_BOMB},
            {chart::N22, IndexTimer::K22_LN_BOMB},
            {chart::N23, IndexTimer::K23_LN_BOMB},
            {chart::N24, IndexTimer::K24_LN_BOMB},
            {chart::N25, IndexTimer::K25_LN_BOMB},
            {chart::Sc2, IndexTimer::S2_LN_BOMB},
        }},
        {{
            {chart::Sc1, IndexTimer::S1_LN_BOMB},
            {chart::N11, IndexTimer::K13_LN_BOMB},
            {chart::N12, IndexTimer::K14_LN_BOMB},
            {chart::N13, IndexTimer::K15_LN_BOMB},
            {chart::N14, IndexTimer::K16_LN_BOMB},
            {chart::N15, IndexTimer::K17_LN_BOMB},
            {chart::N21, IndexTimer::K23_LN_BOMB},
            {chart::N22, IndexTimer::K24_LN_BOMB},
            {chart::N23, IndexTimer::K25_LN_BOMB},
            {chart::N24, IndexTimer::K26_LN_BOMB},
            {chart::N25, IndexTimer::K27_LN_BOMB},
            {chart::Sc2, IndexTimer::S2_LN_BOMB},
        }},
    };

    static const NoteLaneTimerMap bombTimer7k = {{
        {chart::Sc1, IndexTimer::S1_BOMB},
        {chart::N11, IndexTimer::K11_BOMB},
        {chart::N12, IndexTimer::K12_BOMB},
        {chart::N13, IndexTimer::K13_BOMB},
        {chart::N14, IndexTimer::K14_BOMB},
        {chart::N15, IndexTimer::K15_BOMB},
        {chart::N16, IndexTimer::K16_BOMB},
        {chart::N17, IndexTimer::K17_BOMB},
        {chart::N21, IndexTimer::K21_BOMB},
        {chart::N22, IndexTimer::K22_BOMB},
        {chart::N23, IndexTimer::K23_BOMB},
        {chart::N24, IndexTimer::K24_BOMB},
        {chart::N25, IndexTimer::K25_BOMB},
        {chart::N26, IndexTimer::K26_BOMB},
        {chart::N27, IndexTimer::K27_BOMB},
        {chart::Sc2, IndexTimer::S2_BOMB},
    }};

    static const NoteLaneTimerMap bombTimer7kLN = {{
        {chart::Sc1, IndexTimer::S1_LN_BOMB},
        {chart::N11, IndexTimer::K11_LN_BOMB},
        {chart::N12, IndexTimer::K12_LN_BOMB},
        {chart::N13, IndexTimer::K13_LN_BOMB},
        {chart::N14, IndexTimer::K14_LN_BOMB},
        {chart::N15, IndexTimer::K15_LN_BOMB},
        {chart::N16, IndexTimer::K16_LN_BOMB},
        {chart::N17, IndexTimer::K17_LN_BOMB},
        {chart::N21, IndexTimer::K21_LN_BOMB},
        {chart::N22, IndexTimer::K22_LN_BOMB},
        {chart::N23, IndexTimer::K23_LN_BOMB},
        {chart::N24, IndexTimer::K24_LN_BOMB},
        {chart::N25, IndexTimer::K25_LN_BOMB},
        {chart::N26, IndexTimer::K26_LN_BOMB},
        {chart::N27, IndexTimer::K27_LN_BOMB},
        {chart::Sc2, IndexTimer::S2_LN_BOMB},
    }};

    static const NoteLaneTimerMap bombTimer9k = {{
        {chart::N11, IndexTimer::K11_BOMB},
        {chart::N12, IndexTimer::K12_BOMB},
        {chart::N13, IndexTimer::K13_BOMB},
        {chart::N14, IndexTimer::K14_BOMB},
        {chart::N15, IndexTimer::K15_BOMB},
        {chart::N16, IndexTimer::K16_BOMB},
        {chart::N17, IndexTimer::K17_BOMB},
        {chart::N18, IndexTimer::K18_BOMB},
        {chart::N19, IndexTimer::K19_BOMB},
    }};

    static const NoteLaneTimerMap bombTimer9kLN = {{
        {chart::N11, IndexTimer::K11_LN_BOMB},
        {chart::N12, IndexTimer::K12_LN_BOMB},
        {chart::N13, IndexTimer::K13_LN_BOMB},
        {chart::N14, IndexTimer::K14_LN_BOMB},
        {chart::N15, IndexTimer::K15_LN_BOMB},
        {chart::N16, IndexTimer::K16_LN_BOMB},
        {chart::N17, IndexTimer::K17_LN_BOMB},
        {chart::N18, IndexTimer::K18_LN_BOMB},
        {chart::N19, IndexTimer::K19_LN_BOMB},
    }};

    switch (keys)
    {
    case 5:
    case 10: {
        LVF_DEBUG_ASSERT(fiveKeyMapIndex >= 0);
        LVF_DEBUG_ASSERT(fiveKeyMapIndex < static_cast<int>(std::size(bombTimer5k)));
        LVF_DEBUG_ASSERT(fiveKeyMapIndex < static_cast<int>(std::size(bombTimer5kLN)));
        _bombTimerMap = &bombTimer5k[fiveKeyMapIndex];
        _bombLNTimerMap = &bombTimer5kLN[fiveKeyMapIndex];
        break;
    }
    case 7:
    case 14:
        _bombTimerMap = &bombTimer7k;
        _bombLNTimerMap = &bombTimer7kLN;
        break;
    case 9:
        _bombTimerMap = &bombTimer9k;
        _bombLNTimerMap = &bombTimer9kLN;
        break;
    default: break;
    }

    switch (keys)
    {
    case 7:
    case 14: maxMoneyScore = 200000.0; break;
    case 5:
    case 10:
    case 9: maxMoneyScore = 100000.0; break;
    default: break;
    }

    if (_chart)
    {
        noteCount = _chart->getNoteRegularCount() + _chart->getNoteLnCount();
    }

    using namespace std::string_literals;

    _basic.health = health;
    initGaugeParams(mods.gauge);

    _side = side;
    _judgeScratch = !(mods.assist_mask & PLAY_MOD_ASSIST_AUTOSCR);
    switch (side)
    {
    case RulesetBMS::PlaySide::SINGLE:
        _k1P = true;
        _k2P = false;
        break;
    case RulesetBMS::PlaySide::DOUBLE:
        _k1P = true;
        _k2P = true;
        break;
    case RulesetBMS::PlaySide::BATTLE_1P:
        _k1P = true;
        _k2P = false;
        break;
    case RulesetBMS::PlaySide::BATTLE_2P:
        _k1P = false;
        _k2P = true;
        break;
    default:
        _k1P = true;
        _k2P = true;
        break;
    }

    modifierText = makeModsStringForShow(side, mods);
    if (modifierText.empty())
        modifierText = "NONE";
    modifierTextShort = modifierText;

    saveLampMax = getSaveScoreType(false).second;

    _lnJudge.fill(JudgeArea::NOTHING);

    if (_chart)
    {
        for (size_t k = Input::S1L; k <= Input::K2SPDDN; ++k)
        {
            NoteLaneIndex idx;
            idx = _chart->getLaneFromKey(NoteLaneCategory::Note, (Input::Pad)k);
            if (idx != NoteLaneIndex::_)
                _noteListIterators[{NoteLaneCategory::Note, idx}] = _chart->firstNote(NoteLaneCategory::Note, idx);
            idx = _chart->getLaneFromKey(NoteLaneCategory::LN, (Input::Pad)k);
            if (idx != NoteLaneIndex::_)
                _noteListIterators[{NoteLaneCategory::LN, idx}] = _chart->firstNote(NoteLaneCategory::LN, idx);
            idx = _chart->getLaneFromKey(NoteLaneCategory::Mine, (Input::Pad)k);
            if (idx != NoteLaneIndex::_)
                _noteListIterators[{NoteLaneCategory::Mine, idx}] = _chart->firstNote(NoteLaneCategory::Mine, idx);
            idx = _chart->getLaneFromKey(NoteLaneCategory::Invs, (Input::Pad)k);
            if (idx != NoteLaneIndex::_)
                _noteListIterators[{NoteLaneCategory::Invs, idx}] = _chart->firstNote(NoteLaneCategory::Invs, idx);
        }
    }
}

static bool isCourseGrade(RulesetBMS::GaugeType gauge)
{
    switch (gauge)
    {
    case RulesetBMS::GaugeType::GROOVE:
    case RulesetBMS::GaugeType::EASY:
    case RulesetBMS::GaugeType::ASSIST:
    case RulesetBMS::GaugeType::HARD:
    case RulesetBMS::GaugeType::EXHARD:
    case RulesetBMS::GaugeType::DEATH:
    case RulesetBMS::GaugeType::P_ATK:
    case RulesetBMS::GaugeType::G_ATK: return false;
    case RulesetBMS::GaugeType::GRADE:
    case RulesetBMS::GaugeType::EXGRADE: return true;
    }
    lunaticvibes::assert_failed("isCourseGrade");
}

extern bool g_enable_gas_for_test;

void RulesetBMS::initGaugeParams(PlayModifierGaugeType gauge)
{
    const int total = _format != nullptr ? getTotal(_format) : 0;
    auto bms_gauge = get_gauge(gauge);
    LOG_VERBOSE << "[RulesetBMS] initGaugeParams " << bms_gauge;

    // FIXME: adjust result graph per gauge.
    if (g_enable_gas_for_test)
    {
        auto build = [this, total](GaugeType type) -> lunaticvibes::GaugeHolder {
            return {getGauge(type, total, getNoteCount())};
        };
        std::vector<lunaticvibes::GaugeHolder> gauges;
        if (isCourseGrade(bms_gauge))
        {
            for (auto g : std::array{GaugeType::EXGRADE, GaugeType::GRADE})
                gauges.push_back(build(g));
        }
        else if (ConfigMgr::get('P', cfg::P_ENABLE_NEW_GAUGE, false))
        {
            for (auto g : std::array{GaugeType::DEATH, GaugeType::EXHARD, GaugeType::HARD, GaugeType::GROOVE,
                                     GaugeType::EASY, GaugeType::ASSIST})
                gauges.push_back(build(g));
        }
        else
        {
            for (auto g : std::array{GaugeType::DEATH, GaugeType::HARD, GaugeType::GROOVE, GaugeType::EASY})
                gauges.push_back(build(g));
        }
        _gaugeProc = lunaticvibes::GaugeHolderProxy{gauges};
    }
    else
    {
        _gaugeProc = lunaticvibes::GaugeHolderProxy{
            std::array{lunaticvibes::GaugeHolder{getGauge(bms_gauge, total, getNoteCount()), _basic.health}}};
    }
    // TODO: can't use 'this' in constructor :(
    // _gaugeProc.update_for_show(*this);
    // >>>> copy-paste:
    _basic.health = _gaugeProc.get_health().to;
    _clearHealth = _gaugeProc.get_gauge().clear_health;
    _failWhenNoHealth = _gaugeProc.get_gauge().fail_no_health;
    _minHealth = _gaugeProc.get_gauge().min_health;
}

RulesetBMS::JudgeRes RulesetBMS::_calcJudgeByTimes(const Note& note, const lunaticvibes::Time& time) const
{
    // spot judge area
    JudgeArea a = JudgeArea::NOTHING;
    int error = time.norm() - note.time.norm();
    if (error > -judgeTime[(size_t)_judgeDifficulty].KPOOR)
    {
        if (error < -judgeTime[(size_t)_judgeDifficulty].BAD)
            a = JudgeArea::EARLY_KPOOR;
        else if (error < -judgeTime[(size_t)_judgeDifficulty].GOOD)
            a = JudgeArea::EARLY_BAD;
        else if (error < -judgeTime[(size_t)_judgeDifficulty].GREAT)
            a = JudgeArea::EARLY_GOOD;
        else if (error < -judgeTime[(size_t)_judgeDifficulty].PERFECT)
            a = JudgeArea::EARLY_GREAT;
        else if (error < 0)
            a = JudgeArea::EARLY_PERFECT;
        else if (error == 0)
            a = JudgeArea::EXACT_PERFECT;
        else if (error < judgeTime[(size_t)_judgeDifficulty].PERFECT)
            a = JudgeArea::LATE_PERFECT;
        else if (error < judgeTime[(size_t)_judgeDifficulty].GREAT)
            a = JudgeArea::LATE_GREAT;
        else if (error < judgeTime[(size_t)_judgeDifficulty].GOOD)
            a = JudgeArea::LATE_GOOD;
        else if (error < judgeTime[(size_t)_judgeDifficulty].BAD)
            a = JudgeArea::LATE_BAD;
    }

    // log
    /*
    switch (a)
    {
    case JudgeArea::EARLY_KPOOR:   LOG_DEBUG << "EARLY  KPOOR   " << error; break;
    case JudgeArea::EARLY_BAD:     LOG_DEBUG << "EARLY  BAD     " << error; break;
    case JudgeArea::EARLY_GOOD:    LOG_DEBUG << "EARLY  GOOD    " << error; break;
    case JudgeArea::EARLY_GREAT:   LOG_DEBUG << "EARLY  GREAT   " << error; break;
    case JudgeArea::EARLY_PERFECT: LOG_DEBUG << "EARLY  PERFECT " << error; break;
    case JudgeArea::LATE_PERFECT:  LOG_DEBUG << "LATE   PERFECT " << error; break;
    case JudgeArea::LATE_GREAT:    LOG_DEBUG << "LATE   GREAT   " << error; break;
    case JudgeArea::LATE_GOOD:     LOG_DEBUG << "LATE   GOOD    " << error; break;
    case JudgeArea::LATE_BAD:      LOG_DEBUG << "LATE   BAD     " << error; break;
    case JudgeArea::NOTHING:
    case JudgeArea::EXACT_PERFECT:
    case JudgeArea::MISS:
    case JudgeArea::LATE_KPOOR:
    case JudgeArea::MINE_KPOOR: break;
    }
    */

    return {a, error};
}

void RulesetBMS::_judgePress(NoteLaneCategory cat, NoteLaneIndex idx, HitableNote& note, const JudgeRes& judge,
                             const lunaticvibes::Time& t, int slot)
{
    if (cat == NoteLaneCategory::LN && (note.flags & Note::LN_TAIL) &&
        (idx == NoteLaneIndex::Sc1 || idx == NoteLaneIndex::Sc2) && _lnJudge[idx] != JudgeArea::NOTHING)
    {
        // Handle scratch direction change as miss
        _judgeRelease(cat, idx, note, judge, t, slot);
        if (showJudge && _bombLNTimerMap != nullptr && _bombLNTimerMap->find(idx) != _bombLNTimerMap->end())
            State::set(_bombLNTimerMap->at(idx), TIMER_NEVER);
    }

    switch (cat)
    {
    case NoteLaneCategory::Note:
        switch (judge.area)
        {
        case JudgeArea::EARLY_PERFECT:
        case JudgeArea::EXACT_PERFECT:
        case JudgeArea::LATE_PERFECT:
        case JudgeArea::EARLY_GREAT:
        case JudgeArea::LATE_GREAT:
        case JudgeArea::EARLY_GOOD:
        case JudgeArea::LATE_GOOD:
            note.hit = true;
            note.expired = true;
            notesExpired++;
            updateJudge(t, idx, judge.area, slot);
            break;

        case JudgeArea::EARLY_BAD:
        case JudgeArea::LATE_BAD:
            note.expired = true;
            notesExpired++;
            updateJudge(t, idx, judge.area, slot);
            break;

        case JudgeArea::EARLY_KPOOR: updateJudge(t, idx, judge.area, slot); break;

        case JudgeArea::NOTHING:
        case JudgeArea::MISS:
        case JudgeArea::LATE_KPOOR:
        case JudgeArea::MINE_KPOOR: break;
        }
        break;

    case NoteLaneCategory::Invs: break;

    case NoteLaneCategory::LN:
        if (!(note.flags & Note::LN_TAIL))
        {
            switch (judge.area)
            {
            case JudgeArea::EARLY_PERFECT:
            case JudgeArea::EXACT_PERFECT:
            case JudgeArea::LATE_PERFECT:
            case JudgeArea::EARLY_GREAT:
            case JudgeArea::LATE_GREAT:
            case JudgeArea::EARLY_GOOD:
            case JudgeArea::LATE_GOOD:
                _lnJudge[idx] = judge.area;
                note.hit = true;
                note.expired = true;
                if (showJudge && _bombLNTimerMap != nullptr && _bombLNTimerMap->find(idx) != _bombLNTimerMap->end())
                    State::set(_bombLNTimerMap->at(idx), t.norm());
                break;

            case JudgeArea::EARLY_BAD:
            case JudgeArea::LATE_BAD:
                note.expired = true;
                notesExpired++;
                updateJudge(t, idx, judge.area, slot);
                break;

            case JudgeArea::EARLY_KPOOR: updateJudge(t, idx, judge.area, slot); break;

            case JudgeArea::NOTHING:
            case JudgeArea::MISS:
            case JudgeArea::LATE_KPOOR:
            case JudgeArea::MINE_KPOOR: break;
            }
            break;
        }
        break;

    case NoteLaneCategory::_:
    case NoteLaneCategory::Mine:
    case NoteLaneCategory::EXTRA:
    case NoteLaneCategory::NOTECATEGORY_COUNT: break;
    }

    if (note.expired || judge.area == JudgeArea::EARLY_KPOOR || judge.area == JudgeArea::MINE_KPOOR)
        _lastNoteJudge[slot] = judge;
}

// Judges LNs in the case of overhold.
void RulesetBMS::_judgeHold(NoteLaneCategory cat, NoteLaneIndex idx, HitableNote& note, const JudgeRes& judge,
                            const lunaticvibes::Time& t, int slot)
{
    switch (cat)
    {
    case NoteLaneCategory::Mine: {
        if (judge.area == JudgeArea::EXACT_PERFECT || (judge.area == JudgeArea::EARLY_PERFECT && judge.time < -2) ||
            (judge.area == JudgeArea::LATE_PERFECT && judge.time < 2))
        {
            note.hit = true;
            note.expired = true;
            processHpHitMine(note.dvalue);

            // kpoor + 1
            for (auto& i : JudgeAreaIndexAccMap.at(JudgeArea::MINE_KPOOR))
            {
                ++_basic.judge[i];
            }
            if (showJudge)
            {
                if (slot == PLAYER_SLOT_PLAYER)
                {
                    State::set(IndexTimer::PLAY_JUDGE_1P, t.norm());
                    setJudgeInternalTimer1P(JudgeType::KPOOR, t.norm());
                    SoundMgr::playSysSample(SoundChannelType::KEY_LEFT, eSoundSample::SOUND_LANDMINE);
                }
                else if (slot == PLAYER_SLOT_TARGET)
                {
                    State::set(IndexTimer::PLAY_JUDGE_2P, t.norm());
                    setJudgeInternalTimer2P(JudgeType::KPOOR, t.norm());
                    SoundMgr::playSysSample(SoundChannelType::KEY_RIGHT, eSoundSample::SOUND_LANDMINE);
                }
            }

            _lastNoteJudge = {{JudgeArea::MINE_KPOOR, t.norm()}};
        }
        break;
    }
    case NoteLaneCategory::LN:
        if ((note.flags & Note::LN_TAIL) && _lnJudge[idx] != RulesetBMS::JudgeArea::NOTHING &&
            _lnJudge[idx] != RulesetBMS::JudgeArea::EARLY_BAD && _lnJudge[idx] != RulesetBMS::JudgeArea::LATE_BAD)
        {
            if (judge.area == JudgeArea::EXACT_PERFECT || (judge.area == JudgeArea::EARLY_PERFECT && judge.time < -2) ||
                (judge.area == JudgeArea::LATE_PERFECT && judge.time < 2))
            {
                note.hit = true;
                note.expired = true;
                notesExpired++;
                updateJudge(t, idx, _lnJudge[idx], slot);

                if (showJudge && _bombLNTimerMap != nullptr && _bombLNTimerMap->find(idx) != _bombLNTimerMap->end())
                    State::set(_bombLNTimerMap->at(idx), TIMER_NEVER);

                _lastNoteJudge[slot].area = _lnJudge[idx];
                _lastNoteJudge[slot].time = 0;

                _lnJudge[idx] = RulesetBMS::JudgeArea::NOTHING;
            }
        }
        break;

    default: break;
    }
}
void RulesetBMS::_judgeRelease(NoteLaneCategory cat, NoteLaneIndex idx, HitableNote& note, const JudgeRes& judge,
                               const lunaticvibes::Time& t, int slot)
{
    switch (cat)
    {
    case NoteLaneCategory::LN:
        if ((note.flags & Note::LN_TAIL) && _lnJudge[idx] != RulesetBMS::JudgeArea::NOTHING)
        {
            bool hit = true;
            JudgeArea lnJudge = judge.area;
            switch (judge.area)
            {
            case JudgeArea::NOTHING:
            case JudgeArea::EARLY_KPOOR:
            case JudgeArea::EARLY_BAD:
                lnJudge = JudgeArea::EARLY_BAD;
                hit = false;
                break;

            default:
                switch (_lnJudge[idx])
                {
                case JudgeArea::EARLY_PERFECT:
                case JudgeArea::LATE_PERFECT:
                    if (judge.area == JudgeArea::EARLY_PERFECT)
                        lnJudge = JudgeArea::EARLY_PERFECT;
                    break;

                case JudgeArea::EARLY_GREAT:
                case JudgeArea::LATE_GREAT:
                    if (judge.area == JudgeArea::EARLY_PERFECT || judge.area == JudgeArea::EARLY_GREAT)
                        lnJudge = JudgeArea::EARLY_GREAT;
                    break;

                case JudgeArea::EARLY_GOOD:
                case JudgeArea::LATE_GOOD:
                    if (judge.area == JudgeArea::EARLY_PERFECT || judge.area == JudgeArea::EARLY_GREAT ||
                        judge.area == JudgeArea::EARLY_GOOD)
                        lnJudge = JudgeArea::EARLY_GOOD;
                    break;

                default: lnJudge = JudgeArea::EARLY_BAD; break;
                }
                break;
            }

            note.hit = hit;
            note.expired = true;
            notesExpired++;
            updateJudge(t, idx, lnJudge, slot);
            _lnJudge[idx] = RulesetBMS::JudgeArea::NOTHING;
            _lastNoteJudge[slot] = judge;

            if (showJudge && _bombLNTimerMap != nullptr && _bombLNTimerMap->find(idx) != _bombLNTimerMap->end())
                State::set(_bombLNTimerMap->at(idx), TIMER_NEVER);

            break;
        }
        break;

    default: break;
    }
}

void RulesetBMS::processHp(JudgeArea judge)
{
    _gaugeProc.feed(judge);
    _gaugeProc.update_for_show(*this);
}

void RulesetBMS::processHpHitMine(long long mine_value)
{
    _gaugeProc.feed_mine(mine_value);
    _gaugeProc.update_for_show(*this);
}

void RulesetBMS::updateJudge(const lunaticvibes::Time& t, const NoteLaneIndex ch, const RulesetBMS::JudgeArea judge,
                             const int slot)
{
    if (isFailed())
        return;

    for (auto& i : JudgeAreaIndexAccMap.at(judge))
    {
        ++_basic.judge[i];
    }

    // Matches LR2.
    static constexpr lunaticvibes::Time MONEY_SCORE_ANIMATION_TIME{500};

    switch (judge)
    {
    case JudgeArea::EARLY_PERFECT:
    case JudgeArea::EXACT_PERFECT:
    case JudgeArea::LATE_PERFECT:
        // moneyScore += 150000.0 / getNoteCount() +
        //     std::min(int(_basic.combo) - 1, 10) * 50000.0 / (10 * getNoteCount() - 55);
        _moneyScoreAnim.from = moneyScore;
        moneyScore += 1.0 * maxMoneyScore / getNoteCount();
        _moneyScoreAnim.to = moneyScore;
        _moneyScoreAnim.start = lunaticvibes::Time{};
        _moneyScoreAnim.end = _moneyScoreAnim.start + MONEY_SCORE_ANIMATION_TIME;
        exScore += 2;
        ++_basic.combo;
        break;

    case JudgeArea::EARLY_GREAT:
    case JudgeArea::LATE_GREAT:
        // moneyScore += 100000.0 / getNoteCount() +
        //     std::min(int(_basic.combo) - 1, 10) * 50000.0 / (10 * getNoteCount() - 55);
        _moneyScoreAnim.from = moneyScore;
        moneyScore += 0.5 * maxMoneyScore / getNoteCount();
        _moneyScoreAnim.to = moneyScore;
        _moneyScoreAnim.start = lunaticvibes::Time{};
        _moneyScoreAnim.end = _moneyScoreAnim.start + MONEY_SCORE_ANIMATION_TIME;
        exScore += 1;
        ++_basic.combo;
        break;

    case JudgeArea::EARLY_GOOD:
    case JudgeArea::LATE_GOOD:
        // moneyScore += 20000.0 / getNoteCount() +
        //     std::min(int(_basic.combo) - 1, 10) * 50000.0 / (10 * getNoteCount() - 55);
        _moneyScoreAnim.from = moneyScore;
        moneyScore += 0.25 * maxMoneyScore / getNoteCount();
        _moneyScoreAnim.to = moneyScore;
        _moneyScoreAnim.start = lunaticvibes::Time{};
        _moneyScoreAnim.end = _moneyScoreAnim.start + MONEY_SCORE_ANIMATION_TIME;
        ++_basic.combo;
        break;

    case JudgeArea::EARLY_BAD:
    case JudgeArea::LATE_BAD:
    case JudgeArea::MISS:
        _basic.combo = 0;
        _basic.comboDisplay = 0;
        break;

    case JudgeArea::NOTHING:
    case JudgeArea::EARLY_KPOOR:
    case JudgeArea::LATE_KPOOR:
    case JudgeArea::MINE_KPOOR: break;
    }

    processHp(judge);
    if (_basic.combo > _basic.maxCombo)
        _basic.maxCombo = _basic.combo;
    if (_basic.combo + _basic.comboDisplay > _basic.maxComboDisplay)
        _basic.maxComboDisplay = _basic.combo + _basic.comboDisplay;
    if (_basic.judge[JUDGE_CB] == 0)
        _basic.firstMaxCombo = _basic.combo;

    unsigned max = getNoteCount() * 2;
    _basic.total_acc = 100.0 * exScore / max;
    _basic.acc = notesExpired ? (100.0 * exScore / notesExpired / 2) : 0;

    const JudgeType judgeType = JudgeAreaTypeMap.at(judge);
    if (showJudge)
    {
        const bool should_show_bomb = judgeType == JudgeType::PERFECT || judgeType == JudgeType::GREAT;
        if (should_show_bomb && _bombTimerMap != nullptr)
            if (auto it = _bombTimerMap->find(ch); it != _bombTimerMap->end())
                State::set(it->second, t.norm());

        if (slot == PLAYER_SLOT_PLAYER)
        {
            State::set(IndexTimer::PLAY_JUDGE_1P, t.norm());
            setJudgeInternalTimer1P(judgeType, t.norm());
            State::set(IndexNumber::_DISP_NOWCOMBO_1P, _basic.combo + _basic.comboDisplay);
            State::set(IndexOption::PLAY_LAST_JUDGE_1P, JudgeTypeOptMap.at(judgeType));
        }
        else if (slot == PLAYER_SLOT_TARGET)
        {
            State::set(IndexTimer::PLAY_JUDGE_2P, t.norm());
            setJudgeInternalTimer2P(judgeType, t.norm());
            State::set(IndexNumber::_DISP_NOWCOMBO_2P, _basic.combo + _basic.comboDisplay);
            State::set(IndexOption::PLAY_LAST_JUDGE_2P, JudgeTypeOptMap.at(judgeType));
        }
    }
}

static bool isMainUserSide(RulesetBMS::PlaySide side)
{
    switch (side)
    {
    case RulesetBMS::PlaySide::SINGLE:
    case RulesetBMS::PlaySide::DOUBLE:
    case RulesetBMS::PlaySide::BATTLE_1P: return true;
    case RulesetBMS::PlaySide::BATTLE_2P:
    case RulesetBMS::PlaySide::AUTO:
    case RulesetBMS::PlaySide::AUTO_2P:
    case RulesetBMS::PlaySide::AUTO_DOUBLE:
    case RulesetBMS::PlaySide::RIVAL:
    case RulesetBMS::PlaySide::MYBEST:
    case RulesetBMS::PlaySide::NETWORK: return false;
    }
    lunaticvibes::assert_failed("isMainUserSide");
}

static bool isBadOrBetter(const RulesetBMS::JudgeArea area)
{
    switch (area)
    {
    case RulesetBMS::JudgeArea::EARLY_BAD:
    case RulesetBMS::JudgeArea::EARLY_GOOD:
    case RulesetBMS::JudgeArea::EARLY_GREAT:
    case RulesetBMS::JudgeArea::EARLY_PERFECT:
    case RulesetBMS::JudgeArea::EXACT_PERFECT:
    case RulesetBMS::JudgeArea::LATE_PERFECT:
    case RulesetBMS::JudgeArea::LATE_GREAT:
    case RulesetBMS::JudgeArea::LATE_GOOD:
    case RulesetBMS::JudgeArea::LATE_BAD: return true;
    case RulesetBMS::JudgeArea::NOTHING:
    case RulesetBMS::JudgeArea::EARLY_KPOOR:
    case RulesetBMS::JudgeArea::MISS:
    case RulesetBMS::JudgeArea::LATE_KPOOR:
    case RulesetBMS::JudgeArea::MINE_KPOOR: return false;
    }
    lunaticvibes::assert_failed("isBadOrBetter");
}

void RulesetBMS::updateAutoadjust(const JudgeRes& j, const lunaticvibes::Time& rt)
{
    if (!isMainUserSide(_side))
        return;
    if (State::get(IndexOption::PLAY_AUTOADJUST) == 0)
        return;
    const bool isOffsetPositive = j.time >= 0;
    if (isBadOrBetter(j.area))
    {
        _notesSinceLastAutoadjust++;
        if (_notesSinceLastAutoadjust > 9)
        {
            if (j.time.norm() != 0)
            {
                // NOTE: compared to adjust buttons this doesn't need to clamp. Some people do in fact abuse this.
                const int newAdjust = State::get(IndexNumber::TIMING_ADJUST_VISUAL) + (isOffsetPositive ? 1 : -1);
                State::set(IndexNumber::TIMING_ADJUST_VISUAL, newAdjust);
            }
            _notesSinceLastAutoadjust = 0;
        }
    }
}

static std::pair<NoteLaneIndex, HitableNote*> getClosestNote(ChartObjectBase& chart, const Input::Pad k,
                                                             const NoteLaneCategory cat)
{
    NoteLaneIndex idx = chart.getLaneFromKey(cat, k);
    if (idx != _ && !chart.isLastNote(cat, idx))
    {
        auto itNote = chart.incomingNote(cat, idx);
        while (!chart.isLastNote(cat, idx, itNote) && itNote->expired)
            ++itNote;
        if (!chart.isLastNote(cat, idx, itNote))
            return {idx, &*itNote};
    }
    return {};
}

void RulesetBMS::judgeNotePress(const Input::Pad k, const lunaticvibes::Time& t, const lunaticvibes::Time& rt,
                                const int slot)
{
    auto [idx, note] = getClosestNote(*_chart, k, NoteLaneCategory::Note);
    auto category = NoteLaneCategory::Note;
    {
        auto [idx2, note2] = getClosestNote(*_chart, k, NoteLaneCategory::LN);
        if (note2 && (note == nullptr || note2->time < note->time))
        {
            idx = idx2;
            note = note2;
            category = NoteLaneCategory::LN;
        }
    }

    if (note && !note->expired)
    {
        const JudgeRes j = _calcJudgeByTimes(*note, rt);
        updateAutoadjust(j, rt);
        _judgePress(category, idx, *note, j, t, slot);
    }
}
void RulesetBMS::judgeNoteHold(Input::Pad k, const lunaticvibes::Time& t, const lunaticvibes::Time& rt, int slot)
{
    NoteLaneIndex idx;

    idx = _chart->getLaneFromKey(NoteLaneCategory::Mine, k);
    if (idx != _ && !_chart->isLastNote(NoteLaneCategory::Mine, idx))
    {
        auto& note = *_chart->incomingNote(NoteLaneCategory::Mine, idx);
        auto j = _calcJudgeByTimes(note, rt);
        _judgeHold(NoteLaneCategory::Mine, idx, note, j, t, slot);
    }

    idx = _chart->getLaneFromKey(NoteLaneCategory::LN, k);
    if (idx != _ && !_chart->isLastNote(NoteLaneCategory::LN, idx))
    {
        auto& note = *_chart->incomingNote(NoteLaneCategory::LN, idx);
        auto j = _calcJudgeByTimes(note, rt);
        _judgeHold(NoteLaneCategory::LN, idx, note, j, t, slot);
    }
}
void RulesetBMS::judgeNoteRelease(Input::Pad k, const lunaticvibes::Time& t, const lunaticvibes::Time& rt, int slot)
{
    NoteLaneIndex idx = _chart->getLaneFromKey(NoteLaneCategory::LN, k);
    if (idx != _)
    {
        auto itNote = _chart->incomingNote(NoteLaneCategory::LN, idx);
        while (!_chart->isLastNote(NoteLaneCategory::LN, idx, itNote))
        {
            if (!itNote->expired)
            {
                auto j = _calcJudgeByTimes(*itNote, rt);
                _judgeRelease(NoteLaneCategory::LN, idx, *itNote, j, t, slot);
                break;
            }
            ++itNote;
        }
    }
}

void RulesetBMS::updatePress(InputMask& pg, const lunaticvibes::Time& t, const lunaticvibes::InputMaskTimes& tt)
{
    if (t.norm() - _startTime.norm() < 0)
        return;
    if (gPlayContext.isAuto)
        return;
    auto updatePressRange = [&](Input::Pad begin, Input::Pad end, int slot) {
        for (size_t k = begin; k <= static_cast<size_t>(end); ++k)
        {
            if (!pg[k])
                continue;
            judgeNotePress((Input::Pad)k, t, tt[k] - _startTime, slot);
        }
    };
    if (_k1P)
        updatePressRange(Input::K11, Input::K19, PLAYER_SLOT_PLAYER);
    if (_k2P)
        updatePressRange(Input::K21, Input::K29, PLAYER_SLOT_TARGET);
    if (_judgeScratch)
    {
        if (_k1P)
        {
            if (pg[Input::S1L])
                playerScratchDirection[PLAYER_SLOT_PLAYER] = AxisDir::AXIS_UP;
            if (pg[Input::S1R])
                playerScratchDirection[PLAYER_SLOT_PLAYER] = AxisDir::AXIS_DOWN;
            updatePressRange(Input::S1L, Input::S1R, PLAYER_SLOT_PLAYER);
        }
        if (_k2P)
        {
            if (pg[Input::S2L])
                playerScratchDirection[PLAYER_SLOT_TARGET] = AxisDir::AXIS_UP;
            if (pg[Input::S2R])
                playerScratchDirection[PLAYER_SLOT_TARGET] = AxisDir::AXIS_DOWN;
            updatePressRange(Input::S2L, Input::S2R, PLAYER_SLOT_TARGET);
        }
    }
}
void RulesetBMS::updateHold(InputMask& hg, const lunaticvibes::Time& t)
{
    lunaticvibes::Time rt = t - _startTime.norm();
    if (rt < 0)
        return;
    if (gPlayContext.isAuto)
        return;

    auto updateHoldRange = [&](Input::Pad begin, Input::Pad end, int slot) {
        for (size_t k = begin; k <= static_cast<size_t>(end); ++k)
        {
            if (!hg[k])
                continue;
            judgeNoteHold((Input::Pad)k, t, rt, slot);
        }
    };
    if (_k1P)
        updateHoldRange(Input::K11, Input::K19, PLAYER_SLOT_PLAYER);
    if (_k2P)
        updateHoldRange(Input::K21, Input::K29, PLAYER_SLOT_TARGET);
    if (_judgeScratch)
    {
        if (_k1P)
            updateHoldRange(Input::S1L, Input::S1R, PLAYER_SLOT_PLAYER);
        if (_k2P)
            updateHoldRange(Input::S2L, Input::S2R, PLAYER_SLOT_TARGET);
    }
}
void RulesetBMS::updateRelease(InputMask& rg, const lunaticvibes::Time& t)
{
    lunaticvibes::Time rt = t - _startTime.norm();
    if (rt < 0)
        return;
    if (gPlayContext.isAuto)
        return;

    auto updateReleaseRange = [&](Input::Pad begin, Input::Pad end, int slot) {
        for (size_t k = begin; k <= static_cast<size_t>(end); ++k)
        {
            if (!rg[k])
                continue;
            judgeNoteRelease((Input::Pad)k, t, rt, slot);
        }
    };
    if (_k1P)
        updateReleaseRange(Input::K11, Input::K19, PLAYER_SLOT_PLAYER);
    if (_k2P)
        updateReleaseRange(Input::K21, Input::K29, PLAYER_SLOT_TARGET);
    if (_judgeScratch)
    {
        if (_k1P)
        {
            if (playerScratchDirection[PLAYER_SLOT_PLAYER] == AxisDir::AXIS_UP && rg[Input::S1L])
                updateReleaseRange(Input::S1L, Input::S1L, PLAYER_SLOT_PLAYER);
            if (playerScratchDirection[PLAYER_SLOT_PLAYER] == AxisDir::AXIS_DOWN && rg[Input::S1R])
                updateReleaseRange(Input::S1R, Input::S1R, PLAYER_SLOT_PLAYER);
        }
        if (_k2P)
        {
            if (playerScratchDirection[PLAYER_SLOT_TARGET] == AxisDir::AXIS_UP && rg[Input::S2L])
                updateReleaseRange(Input::S2L, Input::S2L, PLAYER_SLOT_TARGET);
            if (playerScratchDirection[PLAYER_SLOT_TARGET] == AxisDir::AXIS_DOWN && rg[Input::S2R])
                updateReleaseRange(Input::S2R, Input::S2R, PLAYER_SLOT_TARGET);
        }
    }
}
void RulesetBMS::updateAxis(double s1, double s2, const lunaticvibes::Time& t)
{
    lunaticvibes::Time rt = t - _startTime.norm();
    if (rt.norm() < 0)
        return;

    using namespace Input;

    if (!gPlayContext.isAuto && (!gPlayContext.isReplay || !_hasStartTime))
    {
        playerScratchAccumulator[PLAYER_SLOT_PLAYER] += s1;
        playerScratchAccumulator[PLAYER_SLOT_TARGET] += s2;
    }
}

void RulesetBMS::update(const lunaticvibes::Time& t)
{
    if (!_hasStartTime)
        setStartTime(t);

    auto rt = t - _startTime.norm();
    _basic.play_time = rt;

    for (auto& [c, n] : _noteListIterators)
    {
        auto [cat, idx] = c;
        while (!_chart->isLastNote(cat, idx, n) && rt >= n->time)
        {
            switch (cat)
            {
            case NoteLaneCategory::Note: notesReached++; break;

            case NoteLaneCategory::LN:
                if (n->flags & Note::LN_TAIL)
                    notesReached++;
                break;

            case NoteLaneCategory::_:
            case NoteLaneCategory::Mine:
            case NoteLaneCategory::Invs:
            case NoteLaneCategory::EXTRA:
            case NoteLaneCategory::NOTECATEGORY_COUNT: break;
            }

            n++;
        }
    }

    auto updateRange = [&](Input::Pad begin, Input::Pad end, int slot) {
        for (size_t k = begin; k <= static_cast<size_t>(end); ++k)
        {
            auto is_scratch_input = [](size_t k) {
                switch (k)
                {
                case Input::S1L:
                case Input::S1R:
                case Input::S2L:
                case Input::S2R: return true;
                default: return false;
                }
            };
            const bool scratch = is_scratch_input(k);

            NoteLaneIndex idx;

            idx = _chart->getLaneFromKey(NoteLaneCategory::Note, (Input::Pad)k);
            if (idx != NoteLaneIndex::_)
            {
                auto itNote = _chart->incomingNote(NoteLaneCategory::Note, idx);
                while (!_chart->isLastNote(NoteLaneCategory::Note, idx, itNote) && !itNote->expired)
                {
                    const auto latePoorWindow =
                        (!scratch || _judgeScratch) ? judgeTime[(size_t)_judgeDifficulty].BAD : 0;
                    if (rt.norm() - itNote->time.norm() >= latePoorWindow)
                    {
                        itNote->expired = true;

                        if (!scratch || _judgeScratch)
                        {
                            updateJudge(t, idx, JudgeArea::MISS, slot);
                            _lastNoteJudge[slot].area = JudgeArea::MISS;
                            _lastNoteJudge[slot].time = latePoorWindow;
                        }

                        notesExpired++;
                        // LOG_DEBUG << "LATE   POOR    "; break;
                    }
                    itNote++;
                }
            }

            idx = _chart->getLaneFromKey(NoteLaneCategory::LN, (Input::Pad)k);
            if (idx != NoteLaneIndex::_)
            {
                auto itNote = _chart->incomingNote(NoteLaneCategory::LN, idx);
                while (!_chart->isLastNote(NoteLaneCategory::LN, idx, itNote) && !itNote->expired)
                {
                    if (!(itNote->flags & Note::LN_TAIL))
                    {
                        if (rt >= itNote->time)
                        {
                            auto hitTime = itNote->time.norm() + judgeTime[(size_t)_judgeDifficulty].BAD;
                            auto itTail = itNote;
                            itTail++;
                            if (!_chart->isLastNote(NoteLaneCategory::LN, idx, itTail) &&
                                (itTail->flags & Note::LN_TAIL) && hitTime > itTail->time.norm())
                            {
                                hitTime = itTail->time.norm();
                            }
                            if (rt.norm() >= hitTime)
                            {
                                itNote->expired = true;

                                if (!scratch || _judgeScratch)
                                {
                                    updateJudge(t, idx, JudgeArea::MISS, slot);
                                    _lastNoteJudge[slot].area = JudgeArea::MISS;
                                    _lastNoteJudge[slot].time = hitTime;
                                }

                                // LOG_DEBUG << "LATE   POOR    "; break;
                            }
                        }
                    }
                    else
                    {
                        auto itHead = itNote;
                        itHead--;
                        if (rt >= itNote->time)
                        {
                            if (!scratch || _judgeScratch)
                            {
                                if (!itHead->hit)
                                {
                                    itNote->expired = true;
                                    notesExpired++;
                                }
                            }
                        }
                    }
                    itNote++;
                }
            }

            idx = _chart->getLaneFromKey(NoteLaneCategory::Invs, (Input::Pad)k);
            if (idx != NoteLaneIndex::_)
            {
                const auto& hitTime = -judgeTime[(size_t)_judgeDifficulty].BAD;
                auto itNote = _chart->incomingNote(NoteLaneCategory::Invs, idx);
                while (!_chart->isLastNote(NoteLaneCategory::Invs, idx, itNote) && !itNote->expired &&
                       rt.norm() - itNote->time.norm() >= hitTime)
                {
                    itNote->expired = true;
                    itNote++;
                }
            }

            idx = _chart->getLaneFromKey(NoteLaneCategory::Mine, (Input::Pad)k);
            if (idx != NoteLaneIndex::_)
            {
                auto itNote = _chart->incomingNote(NoteLaneCategory::Mine, idx);
                while (!_chart->isLastNote(NoteLaneCategory::Mine, idx, itNote) && !itNote->expired &&
                       rt.norm() >= itNote->time.norm())
                {
                    itNote->expired = true;
                    itNote++;
                }
            }
        }
    };
    if (_k1P)
        updateRange(Input::S1L, Input::K19, PLAYER_SLOT_PLAYER);
    if (_k2P)
        updateRange(Input::S2L, Input::K29, PLAYER_SLOT_TARGET);

    if (_judgeScratch)
    {
        auto updateScratch = [&](const lunaticvibes::Time& t, Input::Pad up, Input::Pad dn, double& val, int slot) {
            double scratchThreshold = 0.001;
            double scratchRewind = 0.0001;
            if (val > scratchThreshold)
            {
                // scratch down
                val -= scratchThreshold;

                switch (playerScratchDirection[slot])
                {
                case AxisDir::AXIS_DOWN: judgeNoteHold(dn, t, rt, slot); break;
                case AxisDir::AXIS_UP:
                case AxisDir::AXIS_NONE:
                    judgeNoteRelease(up, t, rt, slot);
                    judgeNotePress(dn, t, rt, slot);
                    break;
                }

                playerScratchLastUpdate[slot] = t;
                playerScratchDirection[slot] = AxisDir::AXIS_DOWN;
            }
            else if (val < -scratchThreshold)
            {
                // scratch up
                val += scratchThreshold;

                switch (playerScratchDirection[slot])
                {
                case AxisDir::AXIS_UP: judgeNoteHold(up, t, rt, slot); break;
                case AxisDir::AXIS_DOWN:
                case AxisDir::AXIS_NONE:
                    judgeNoteRelease(dn, t, rt, slot);
                    judgeNotePress(up, t, rt, slot);
                    break;
                }

                playerScratchLastUpdate[slot] = t;
                playerScratchDirection[slot] = AxisDir::AXIS_UP;
            }

            if (val > scratchRewind)
                val -= scratchRewind;
            else if (val < -scratchRewind)
                val += scratchRewind;
            else
                val = 0.;

            if ((t - playerScratchLastUpdate[slot]).norm() > 133)
            {
                // release
                switch (playerScratchDirection[slot])
                {
                case AxisDir::AXIS_UP: judgeNoteRelease(up, t, rt, slot); break;
                case AxisDir::AXIS_DOWN: judgeNoteRelease(dn, t, rt, slot); break;
                }

                playerScratchDirection[slot] = AxisDir::AXIS_NONE;
                playerScratchLastUpdate[slot] = TIMER_NEVER;
            }
        };
        updateScratch(t, Input::S1L, Input::S1R, playerScratchAccumulator[PLAYER_SLOT_PLAYER], PLAYER_SLOT_PLAYER);
        updateScratch(t, Input::S2L, Input::S2R, playerScratchAccumulator[PLAYER_SLOT_TARGET], PLAYER_SLOT_TARGET);
    }

    _gaugeProc.update_for_show(*this); // Would not need to call this once health is taken from the gauge instead of
                                       // _basic.
    _isCleared = isCleared();
    // LOG_VERBOSE << "ruleset_bms update isFailed()=" << isFailed() << " _isFailed=" << _isFailed;
    if (isFinished())
        _isFailed |= _basic.health < getClearHealth();

    updateGlobals();
}

static Option::e_lamp_type gaugeToOption(RulesetBMS::GaugeType _gauge)
{
    using GT = RulesetBMS::GaugeType;
    switch (_gauge)
    {
    case GT::HARD: return Option::LAMP_HARD;
    case GT::EXHARD: return Option::LAMP_EXHARD;
    case GT::DEATH: return Option::LAMP_FULLCOMBO;
    case GT::P_ATK: // return Option::LAMP_FULLCOMBO; // TODO: check this.
    case GT::G_ATK: // return Option::LAMP_FULLCOMBO; // TODO: check this.
    case GT::GROOVE: return Option::LAMP_NORMAL;
    case GT::EASY: return Option::LAMP_EASY;
    case GT::ASSIST: return Option::LAMP_ASSIST;
    case GT::GRADE:
    case GT::EXGRADE: return Option::LAMP_NOPLAY;
    }
    lunaticvibes::assert_failed("gaugeToOption");
}

Option::e_lamp_type RulesetBMS::calculateLamp() const
{
    Option::e_lamp_type lamp = Option::LAMP_NOPLAY;
    if (isNoScore() && _basic.judge[JUDGE_BP] == 0)
    {
        lamp = Option::LAMP_NOPLAY;
    }
    else if (_basic.judge[JUDGE_CB] == 0)
    {
        if (_basic.acc >= 100.0)
            lamp = Option::LAMP_MAX;
        else if (_basic.judge[JUDGE_GOOD] == 0)
            lamp = Option::LAMP_PERFECT;
        else if (isCleared())
            lamp = Option::LAMP_FULLCOMBO;
    }
    else if (isFailed())
    {
        lamp = Option::LAMP_FAILED;
    }
    else
    {
        lamp = gaugeToOption(getGaugeType());
    }
    return std::min(lamp, saveLampMax);
}

double RulesetBMS::getScore() const
{
    return moneyScore;
}

double RulesetBMS::getMaxMoneyScore() const
{
    return maxMoneyScore;
}

unsigned RulesetBMS::getExScore() const
{
    return exScore;
}

unsigned RulesetBMS::getJudgeCount(JudgeType idx) const
{
    switch (idx)
    {
    case JudgeType::PERFECT: return _basic.judge[JUDGE_PERFECT];
    case JudgeType::GREAT: return _basic.judge[JUDGE_GREAT];
    case JudgeType::GOOD: return _basic.judge[JUDGE_GOOD];
    case JudgeType::BAD: return _basic.judge[JUDGE_BAD];
    case JudgeType::KPOOR: return _basic.judge[JUDGE_KPOOR];
    case JudgeType::MISS: return _basic.judge[JUDGE_MISS];
    }
    return 0;
}

unsigned RulesetBMS::getJudgeCountEx(JudgeIndex idx) const
{
    return _basic.judge[idx];
}

std::string RulesetBMS::getModifierText() const
{
    return modifierText;
}
std::string RulesetBMS::getModifierTextShort() const
{
    return modifierTextShort;
}

unsigned RulesetBMS::getNoteCount() const
{
    return noteCount;
}

unsigned RulesetBMS::getMaxCombo() const
{
    if (_judgeScratch)
    {
        return getNoteCount();
    }
    else
    {
        unsigned count = getNoteCount();
        auto pChart = std::dynamic_pointer_cast<ChartObjectBMS>(_chart);
        if (pChart != nullptr)
        {
            count -= pChart->getScratchCount();
        }
        return count;
    }
}

void RulesetBMS::fail()
{
    LOG_VERBOSE << "[RulesetBMS] fail()";
    _isFailed = true;

    _basic.health = _minHealth;
    _basic.combo = 0;

    int notesRemain = getNoteCount() - notesExpired;
    _basic.judge[JUDGE_BP] += notesRemain;
    _basic.judge[JUDGE_CB] += notesRemain;
    notesExpired = notesReached = getNoteCount();

    //_basic.acc = _basic.total_acc;
}

// 1:fast 2:slow
static unsigned getFastSlow(RulesetBMS::JudgeArea area)
{
    switch (area)
    {
        using enum RulesetBMS::JudgeArea;
    case EARLY_GREAT:
    case EARLY_GOOD:
    case EARLY_BAD:
    case EARLY_KPOOR: return 1;
    case LATE_GREAT:
    case LATE_GOOD:
    case LATE_BAD:
    case MISS:
    case LATE_KPOOR: return 2;
    case NOTHING:
    case EARLY_PERFECT:
    case EXACT_PERFECT:
    case LATE_PERFECT:
    case MINE_KPOOR: return 0;
    }
    lunaticvibes::assert_failed("getFastSlow");
}

static int diffToNextRank(double total_acc, int exScore, int maxScore)
{
    if (total_acc >= 100.0 * 8.0 / 9)
        return exScore - maxScore; // MAX-
    if (total_acc >= 100.0 * 7.0 / 9)
        return static_cast<int>(exScore - maxScore * 8.0 / 9); // AAA-
    if (total_acc >= 100.0 * 6.0 / 9)
        return static_cast<int>(exScore - maxScore * 7.0 / 9); // AA-
    if (total_acc >= 100.0 * 5.0 / 9)
        return static_cast<int>(exScore - maxScore * 6.0 / 9); // A-
    if (total_acc >= 100.0 * 4.0 / 9)
        return static_cast<int>(exScore - maxScore * 5.0 / 9); // B-
    if (total_acc >= 100.0 * 3.0 / 9)
        return static_cast<int>(exScore - maxScore * 4.0 / 9); // C-
    if (total_acc >= 100.0 * 2.0 / 9)
        return static_cast<int>(exScore - maxScore * 3.0 / 9); // D-
    return static_cast<int>(exScore - maxScore * 2.0 / 9);     // E-
}

void RulesetBMS::updateGlobals()
{
    if (_side == PlaySide::SINGLE || _side == PlaySide::DOUBLE || _side == PlaySide::BATTLE_1P ||
        _side == PlaySide::AUTO || _side == PlaySide::AUTO_DOUBLE) // includes DP
    {
        State::set(IndexNumber::PLAY_1P_EXSCORE, exScore);
        State::set(IndexNumber::PLAY_1P_NOWCOMBO, _basic.combo + _basic.comboDisplay);
        State::set(IndexNumber::PLAY_1P_MAXCOMBO, _basic.maxComboDisplay);
        State::set(IndexNumber::PLAY_1P_RATE, int(std::floor(_basic.acc)));
        State::set(IndexNumber::PLAY_1P_RATEDECIMAL, int(std::floor((_basic.acc - int(_basic.acc)) * 100)));
        State::set(IndexNumber::PLAY_1P_TOTALNOTES, getNoteCount());
        State::set(IndexNumber::PLAY_1P_TOTAL_RATE, int(std::floor(_basic.total_acc)));
        State::set(IndexNumber::PLAY_1P_TOTAL_RATE_DECIMAL2,
                   int(std::floor((_basic.total_acc - int(_basic.total_acc)) * 100)));
        State::set(IndexNumber::PLAY_1P_PERFECT, _basic.judge[JUDGE_PERFECT]);
        State::set(IndexNumber::PLAY_1P_GREAT, _basic.judge[JUDGE_GREAT]);
        State::set(IndexNumber::PLAY_1P_GOOD, _basic.judge[JUDGE_GOOD]);
        State::set(IndexNumber::PLAY_1P_BAD, _basic.judge[JUDGE_BAD]);
        State::set(IndexNumber::PLAY_1P_POOR, _basic.judge[JUDGE_POOR]);
        // TODO: remove as it can be calculated. This is needed right now for internal state.
        State::set(IndexNumber::PLAY_1P_GROOVEGAUGE, int(_basic.health * 100));

        State::set(IndexNumber::PLAY_1P_MISS, _basic.judge[JUDGE_MISS]);
        State::set(IndexNumber::PLAY_1P_FAST_COUNT, _basic.judge[JUDGE_EARLY]);
        State::set(IndexNumber::PLAY_1P_SLOW_COUNT, _basic.judge[JUDGE_LATE]);
        State::set(IndexNumber::PLAY_1P_COMBOBREAK, _basic.judge[JUDGE_CB]);
        State::set(IndexNumber::PLAY_1P_BPOOR, _basic.judge[JUDGE_KPOOR]);
        State::set(IndexNumber::PLAY_1P_BP, _basic.judge[JUDGE_BP]);
        State::set(IndexNumber::LR2IR_REPLACE_PLAY_1P_FAST_COUNT, _basic.judge[JUDGE_EARLY]);
        State::set(IndexNumber::LR2IR_REPLACE_PLAY_1P_SLOW_COUNT, _basic.judge[JUDGE_LATE]);
        State::set(IndexNumber::LR2IR_REPLACE_PLAY_1P_COMBOBREAK, _basic.judge[JUDGE_CB]);

        if (showJudge)
        {
            const int fs_of_player = getFastSlow(_lastNoteJudge[PLAYER_SLOT_PLAYER].area);
            State::set(IndexNumber::LR2IR_REPLACE_PLAY_1P_FAST_SLOW, fs_of_player);
            State::set(IndexOption::PLAY_LAST_JUDGE_FASTSLOW_1P, fs_of_player);
            State::set(IndexNumber::LR2IR_REPLACE_PLAY_1P_JUDGE_TIME_ERROR_MS,
                       _lastNoteJudge[PLAYER_SLOT_PLAYER].time.norm());
            State::set(IndexNumber::PLAY_1P_JUDGE_TIME_ERROR_MS, _lastNoteJudge[PLAYER_SLOT_PLAYER].time.norm());

            if (_side == PlaySide::DOUBLE || _side == PlaySide::AUTO_DOUBLE)
            {
                const int fs_of_target = getFastSlow(_lastNoteJudge[PLAYER_SLOT_TARGET].area);
                State::set(IndexNumber::LR2IR_REPLACE_PLAY_2P_FAST_SLOW, fs_of_target);
                State::set(IndexOption::PLAY_LAST_JUDGE_FASTSLOW_2P, fs_of_target);
                State::set(IndexNumber::LR2IR_REPLACE_PLAY_2P_JUDGE_TIME_ERROR_MS,
                           _lastNoteJudge[PLAYER_SLOT_TARGET].time.norm());
                State::set(IndexNumber::PLAY_2P_JUDGE_TIME_ERROR_MS, _lastNoteJudge[PLAYER_SLOT_TARGET].time.norm());
            }
        }

        State::set(IndexOption::PLAY_RANK_ESTIMATED_1P, Option::getRankType(_basic.acc));
        State::set(IndexOption::PLAY_RANK_BORDER_1P, Option::getRankType(_basic.total_acc));
        State::set(IndexOption::RESULT_RANK_1P, Option::getRankType(_basic.total_acc));
        State::set(IndexOption::PLAY_HEALTH_1P, Option::getHealthType(_basic.health));

        const int diff_to_next_rank = diffToNextRank(_basic.total_acc, exScore, getMaxScore());
        State::set(IndexNumber::PLAY_1P_NEXT_RANK_EX_DIFF, diff_to_next_rank);

        State::set(IndexNumber::RESULT_NEXT_RANK_EX_DIFF, diff_to_next_rank);

        State::set(IndexNumber::LR2IR_REPLACE_PLAY_RUNNING_NOTES, notesExpired);
        State::set(IndexNumber::LR2IR_REPLACE_PLAY_REMAIN_NOTES, getNoteCount() - notesExpired);

        State::set(IndexOption::RESULT_CLEAR_TYPE_1P, calculateLamp());
    }
    else if (_side == PlaySide::BATTLE_2P || _side == PlaySide::AUTO_2P || _side == PlaySide::RIVAL) // excludes DP
    {
        if (_side == PlaySide::RIVAL)
        {
            // target exscore is affected by target type. Handle in ScenePlay
        }
        else
        {
            State::set(IndexNumber::PLAY_2P_EXSCORE, exScore);
        }
        State::set(IndexNumber::PLAY_2P_NOWCOMBO, _basic.combo + _basic.comboDisplay);
        State::set(IndexNumber::PLAY_2P_MAXCOMBO, _basic.maxComboDisplay);
        State::set(IndexNumber::PLAY_2P_RATE, int(std::floor(_basic.acc)));
        State::set(IndexNumber::PLAY_2P_RATEDECIMAL, int(std::floor((_basic.acc - int(_basic.acc)) * 100)));
        State::set(IndexNumber::PLAY_2P_TOTALNOTES, getNoteCount());
        State::set(IndexNumber::PLAY_2P_TOTAL_RATE, int(std::floor(_basic.total_acc)));
        State::set(IndexNumber::PLAY_2P_TOTAL_RATE_DECIMAL2,
                   int(std::floor((_basic.total_acc - int(_basic.total_acc)) * 100)));
        State::set(IndexNumber::PLAY_2P_PERFECT, _basic.judge[JUDGE_PERFECT]);
        State::set(IndexNumber::PLAY_2P_GREAT, _basic.judge[JUDGE_GREAT]);
        State::set(IndexNumber::PLAY_2P_GOOD, _basic.judge[JUDGE_GOOD]);
        State::set(IndexNumber::PLAY_2P_BAD, _basic.judge[JUDGE_BAD]);
        State::set(IndexNumber::PLAY_2P_POOR, _basic.judge[JUDGE_POOR]);
        // TODO: remove as it can be calculated. This is needed right now for internal state.
        State::set(IndexNumber::PLAY_2P_GROOVEGAUGE, int(_basic.health * 100));

        State::set(IndexNumber::PLAY_2P_MISS, _basic.judge[JUDGE_MISS]);
        State::set(IndexNumber::PLAY_2P_FAST_COUNT, _basic.judge[JUDGE_EARLY]);
        State::set(IndexNumber::PLAY_2P_SLOW_COUNT, _basic.judge[JUDGE_LATE]);
        State::set(IndexNumber::PLAY_2P_COMBOBREAK, _basic.judge[JUDGE_CB]);
        State::set(IndexNumber::PLAY_2P_BPOOR, _basic.judge[JUDGE_KPOOR]);
        State::set(IndexNumber::PLAY_2P_BP, _basic.judge[JUDGE_BP]);

        if (showJudge)
        {
            const unsigned fastslow = getFastSlow(_lastNoteJudge[PLAYER_SLOT_TARGET].area);
            State::set(IndexNumber::LR2IR_REPLACE_PLAY_2P_FAST_SLOW, fastslow);
            State::set(IndexOption::PLAY_LAST_JUDGE_FASTSLOW_2P, fastslow);
            State::set(IndexNumber::LR2IR_REPLACE_PLAY_2P_JUDGE_TIME_ERROR_MS,
                       _lastNoteJudge[PLAYER_SLOT_TARGET].time.norm());
            State::set(IndexNumber::PLAY_2P_JUDGE_TIME_ERROR_MS, _lastNoteJudge[PLAYER_SLOT_TARGET].time.norm());
        }

        State::set(IndexOption::PLAY_RANK_ESTIMATED_2P, Option::getRankType(_basic.acc));
        State::set(IndexOption::PLAY_RANK_BORDER_2P, Option::getRankType(_basic.total_acc));
        State::set(IndexOption::RESULT_RANK_2P, Option::getRankType(_basic.total_acc));
        State::set(IndexOption::PLAY_HEALTH_2P, Option::getHealthType(_basic.health));

        const int diff_to_next_rank = diffToNextRank(_basic.total_acc, exScore, getMaxScore());
        State::set(IndexNumber::PLAY_2P_NEXT_RANK_EX_DIFF, diff_to_next_rank);

        State::set(IndexOption::RESULT_CLEAR_TYPE_2P, calculateLamp());
    }
}
