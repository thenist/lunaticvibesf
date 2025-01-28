#pragma once

#include "ruleset.h"

#include <common/chartformat/chartformat.h>
#include <common/chartformat/chartformat_bms.h>
#include <game/scene/scene_context.h>
#include <game/skin/skin_lr2_number_animation.h>

#include <array>
#include <memory>
#include <ranges>

using lunaticvibes::parser_bms::JudgeDifficulty;

class RulesetBMS;

namespace lunaticvibes
{

PlayModifierGaugeType convertGaugeType(int nType);
constexpr inline PlayModifierRandomType convertRandomType(int nType)
{
    switch (nType)
    {
    case 1: return PlayModifierRandomType::MIRROR;
    case 2: return PlayModifierRandomType::RANDOM;
    case 3: return PlayModifierRandomType::SRAN;
    case 4: return PlayModifierRandomType::HRAN;
    case 5: return PlayModifierRandomType::ALLSCR;
    case 6: return PlayModifierRandomType::RRAN;
    case 7: return PlayModifierRandomType::DB_SYNCHRONIZE;
    case 8: return PlayModifierRandomType::DB_SYMMETRY;
    case 0:
    default: return PlayModifierRandomType::NONE;
    };
};
constexpr inline PlayModifierHispeedFixType convertHSType(int nType)
{
    switch (nType)
    {
    case 1: return PlayModifierHispeedFixType::MAXBPM;
    case 2: return PlayModifierHispeedFixType::MINBPM;
    case 3: return PlayModifierHispeedFixType::AVERAGE;
    case 4: return PlayModifierHispeedFixType::CONSTANT;
    case 5: return PlayModifierHispeedFixType::INITIAL;
    case 6: return PlayModifierHispeedFixType::MAIN;
    case 0:
    default: return PlayModifierHispeedFixType::NONE;
    };
};
unsigned getEffectiveChartTotal(ChartFormatBase& format, PlayModifierGaugeType gauge);

// Judge area definitions.
// e.g. EARLY_PERFECT: Perfect early half part
enum class BmsJudgeArea
{
    NOTHING = 0,
    EARLY_KPOOR,
    EARLY_BAD,
    EARLY_GOOD,
    EARLY_GREAT,
    EARLY_PERFECT,
    EXACT_PERFECT,
    LATE_PERFECT,
    LATE_GREAT,
    LATE_GOOD,
    LATE_BAD,
    MISS,
    LATE_KPOOR,
    MINE_KPOOR,
};

enum class BmsGaugeType
{
    GROOVE,
    EASY,
    ASSIST,
    HARD,
    EXHARD,
    DEATH,
    P_ATK,
    G_ATK,
    GRADE,
    EXGRADE,
};

struct Lr2GaugeIncrements
{
    static constexpr size_t JUDGE_TYPE_COUNT = 6; // Count of enum class JudgeType.

    std::array<double, JUDGE_TYPE_COUNT> health_gain{}; // How much health to add for note hit.
    double start_health{};
    double min_health{};
    double clear_health{}; // Minimal health for clear at the end of the chart.
    double hp_lose_multiplier = 1.;
    BmsGaugeType type{};
    bool fail_no_health{};
    bool reduce_below_30_hp_damage{};
};

class GaugeHolder
{
public:
    constexpr GaugeHolder(Lr2GaugeIncrements gauge) : GaugeHolder(gauge, gauge.start_health) {}
    // health - use following health instead of gauge default.
    constexpr GaugeHolder(Lr2GaugeIncrements gauge, double health)
        : _gauge(gauge), _health({.to = health}), _did_fail(false)
    {
    }

    void feed(BmsJudgeArea judge);
    void feed_mine(long long mine_value);
    void update_for_show(RulesetBMS& ruleset);
    [[nodiscard]] bool did_fail() const { return _did_fail; };
    [[nodiscard]] const Lr2GaugeIncrements& get_gauge() const { return _gauge; };
    [[nodiscard]] const NumberAnimation& get_health() const { return _health; };

private:
    void process(double diff);

    Lr2GaugeIncrements _gauge;
    NumberAnimation _health;
    bool _did_fail;
};

// Holds GaugeHolder objects. If there are several, show methods use the highest one.
class GaugeHolderProxy
{
public:
    // gauges - earlier objects get priority.
    constexpr GaugeHolderProxy(std::ranges::sized_range auto gauges)
    {
        // static_assert(!gauges.empty());
        for (auto gauge : gauges)
            _gauges.push_back(gauge);
    }

    void feed(BmsJudgeArea judge);
    void feed_mine(long long mine_value);
    void update_for_show(RulesetBMS& ruleset);
    [[nodiscard]] const Lr2GaugeIncrements& get_gauge() const;
    [[nodiscard]] const NumberAnimation& get_health() const;

private:
    // [[nodiscard]] decltype(auto) current_gauge(this auto&& self) // TODO(GCC14): use this.
    [[nodiscard]] GaugeHolder& current_gauge()
    {
        for (auto& gauge : _gauges)
            if (gauge.get_health().to >= gauge.get_gauge().clear_health && !gauge.did_fail())
                return gauge;
        return _gauges.back();
    }
    [[nodiscard]] const GaugeHolder& current_gauge() const
    {
        for (auto& gauge : _gauges)
            if (gauge.get_health().to >= gauge.get_gauge().clear_health && !gauge.did_fail())
                return gauge;
        return _gauges.back();
    }

    std::vector<GaugeHolder> _gauges;
};

} // namespace lunaticvibes

class RulesetBMS : virtual public RulesetBase
{
    friend lunaticvibes::GaugeHolder;

public:
    enum JudgeIndex
    {
        JUDGE_PERFECT,
        JUDGE_GREAT,
        JUDGE_GOOD,
        JUDGE_BAD,
        JUDGE_POOR,
        JUDGE_KPOOR,
        JUDGE_MISS,
        JUDGE_BP,
        JUDGE_CB,
        JUDGE_EARLY,
        JUDGE_LATE,

        JUDGE_EARLY_POOR,
        JUDGE_EARLY_BAD,
        JUDGE_EARLY_GOOD,
        JUDGE_EARLY_GREAT,
        JUDGE_EARLY_PERFECT,
        JUDGE_EXACT_PERFECT,
        JUDGE_LATE_PERFECT,
        JUDGE_LATE_GREAT,
        JUDGE_LATE_GOOD,
        JUDGE_LATE_BAD,
        JUDGE_LATE_POOR,
        JUDGE_MINE_POOR,
    };

    static constexpr auto LR2_DEFAULT_RANK{JudgeDifficulty::NORMAL};

    enum class JudgeType
    {
        PERFECT = 0, // Option::JUDGE_0
        GREAT,
        GOOD,
        BAD,
        KPOOR,
        MISS,
    };
    inline static const std::map<JudgeType, Option::e_judge_type> JudgeTypeOptMap = {
        {JudgeType::PERFECT, Option::JUDGE_0}, {JudgeType::GREAT, Option::JUDGE_1}, {JudgeType::GOOD, Option::JUDGE_2},
        {JudgeType::BAD, Option::JUDGE_3},     {JudgeType::KPOOR, Option::JUDGE_4}, {JudgeType::MISS, Option::JUDGE_5},
    };

    using GaugeType = lunaticvibes::BmsGaugeType;

    // Judge Time definitions.
    // Values are one-way judge times in ms, representing
    // PERFECT, GREAT, GOOD, BAD, ПеPOOR respectively.
    struct JudgeTime
    {
        int PERFECT;
        int GREAT;
        int GOOD;
        int BAD;
        int KPOOR;
    };
    // https://github.com/wcko87/lr2oraja/blob/readme/README.md
    // https://iidx.org/misc/iidx_lr2_beatoraja_diff#timing-window
    // This has been verified with LR2 disassembly.
    inline static const JudgeTime judgeTime[] = {
        {8, 24, 40, 200, 1000},   // VERY HARD
        {15, 30, 60, 200, 1000},  // HARD
        {18, 40, 100, 200, 1000}, // NORMAL
        {21, 60, 120, 200, 1000}, // EASY
        // TODO: VERY EASY? beatoraja uses EASY*1.25 windows, lr2oraja uses NORMAL.
        // {8, 24, 40, 200, 1000}, // GAMBOL LEVEL 1
        // {8, 12, 12, 200, 1000}, // GAMBOL LEVEL 2
    };

    using JudgeArea = ::lunaticvibes::BmsJudgeArea;
    inline static const std::map<JudgeArea, JudgeType> JudgeAreaTypeMap = {
        {JudgeArea::NOTHING, JudgeType::MISS},          {JudgeArea::EARLY_KPOOR, JudgeType::KPOOR},
        {JudgeArea::EARLY_BAD, JudgeType::BAD},         {JudgeArea::EARLY_GOOD, JudgeType::GOOD},
        {JudgeArea::EARLY_GREAT, JudgeType::GREAT},     {JudgeArea::EARLY_PERFECT, JudgeType::PERFECT},
        {JudgeArea::EXACT_PERFECT, JudgeType::PERFECT}, {JudgeArea::LATE_PERFECT, JudgeType::PERFECT},
        {JudgeArea::LATE_GREAT, JudgeType::GREAT},      {JudgeArea::LATE_GOOD, JudgeType::GOOD},
        {JudgeArea::LATE_BAD, JudgeType::BAD},          {JudgeArea::MISS, JudgeType::MISS},
        {JudgeArea::LATE_KPOOR, JudgeType::KPOOR},      {JudgeArea::MINE_KPOOR, JudgeType::KPOOR},
    };
    inline static const std::map<JudgeArea, std::vector<JudgeIndex>> JudgeAreaIndexAccMap = {
        {JudgeArea::NOTHING, {}},
        {JudgeArea::EARLY_KPOOR, {JUDGE_KPOOR, JUDGE_POOR, JUDGE_BP, JUDGE_EARLY}},
        {JudgeArea::EARLY_BAD, {JUDGE_BAD, JUDGE_EARLY_BAD, JUDGE_BP, JUDGE_CB, JUDGE_EARLY}},
        {JudgeArea::EARLY_GOOD, {JUDGE_GOOD, JUDGE_EARLY_GOOD, JUDGE_EARLY}},
        {JudgeArea::EARLY_GREAT, {JUDGE_GREAT, JUDGE_EARLY_GREAT, JUDGE_EARLY}},
        {JudgeArea::EARLY_PERFECT, {JUDGE_PERFECT, JUDGE_EARLY_PERFECT}},
        {JudgeArea::EXACT_PERFECT, {JUDGE_PERFECT, JUDGE_EXACT_PERFECT}},
        {JudgeArea::LATE_PERFECT, {JUDGE_PERFECT, JUDGE_LATE_PERFECT}},
        {JudgeArea::LATE_GREAT, {JUDGE_GREAT, JUDGE_LATE_GREAT, JUDGE_LATE}},
        {JudgeArea::LATE_GOOD, {JUDGE_GOOD, JUDGE_LATE_GOOD, JUDGE_LATE}},
        {JudgeArea::LATE_BAD, {JUDGE_BAD, JUDGE_LATE_BAD, JUDGE_BP, JUDGE_CB, JUDGE_LATE}},
        {JudgeArea::MISS, {JUDGE_MISS, JUDGE_POOR, JUDGE_BP, JUDGE_CB, JUDGE_LATE}},
        {JudgeArea::LATE_KPOOR, {JUDGE_KPOOR, JUDGE_POOR, JUDGE_BP, JUDGE_LATE}},
        {JudgeArea::MINE_KPOOR, {JUDGE_KPOOR, JUDGE_POOR, JUDGE_BP}},
    };

    /// /////////////////////////////////////////////////////////////////////

    using NoteLaneTimerMap = std::map<chart::NoteLaneIndex, IndexTimer>;

    enum class PlaySide
    {
        SINGLE,
        DOUBLE,
        BATTLE_1P,
        BATTLE_2P,
        AUTO,
        AUTO_2P,
        AUTO_DOUBLE,
        RIVAL,
        MYBEST,
        NETWORK,
    };

    struct JudgeRes
    {
        JudgeArea area{JudgeArea::NOTHING};
        lunaticvibes::Time time{0};
    };

protected:
    // members set on construction
    lunaticvibes::GaugeHolderProxy _gaugeProc;
    PlaySide _side = PlaySide::SINGLE;
    bool _k1P = false, _k2P = false;
    JudgeDifficulty _judgeDifficulty = LR2_DEFAULT_RANK;
    std::shared_ptr<PlayContextParams::MutexReplayChart> _replayNew;

    bool _judgeScratch = true;

    bool showJudge = true;
    const NoteLaneTimerMap* _bombTimerMap = nullptr;
    const NoteLaneTimerMap* _bombLNTimerMap = nullptr;

    unsigned noteCount = 0;

    std::string modifierText, modifierTextShort;
    Option::e_lamp_type saveLampMax;

private:
    unsigned int _notesSinceLastAutoadjust = 0;

protected:
    // members change in game
    std::array<JudgeArea, chart::NOTELANEINDEX_COUNT> _lnJudge{JudgeArea::NOTHING};
    std::array<JudgeRes, 2> _lastNoteJudge{};

    std::map<chart::NoteLane, ChartObjectBase::NoteIterator> _noteListIterators;

    std::array<AxisDir, 2> playerScratchDirection = {0, 0};
    std::array<lunaticvibes::Time, 2> playerScratchLastUpdate = {TIMER_NEVER, TIMER_NEVER};
    std::array<double, 2> playerScratchAccumulator = {0, 0};

protected:
    // members essential for score calculation
    double moneyScore = 0.0;
    double maxMoneyScore = 200000.0;
    unsigned exScore = 0;

    lunaticvibes::NumberAnimation _moneyScoreAnim;

public:
    // fiveKeyMapIndex - if not 5k, set to -1.
    RulesetBMS(std::shared_ptr<ChartFormatBase> format, std::shared_ptr<ChartObjectBase> chart, PlayModifiers mods,
               GameModeKeys keys, JudgeDifficulty difficulty, double health, PlaySide side, int fiveKeyMapIndex,
               std::shared_ptr<PlayContextParams::MutexReplayChart> replayNew);

    void initGaugeParams(PlayModifierGaugeType gauge);

private:
    [[nodiscard]] JudgeRes _calcJudgeByTimes(const Note& note, const lunaticvibes::Time& time) const;

    void updateAutoadjust(const JudgeRes& j, const lunaticvibes::Time& rt);

    void _judgePress(chart::NoteLaneCategory cat, chart::NoteLaneIndex idx, HitableNote& note, const JudgeRes& judge,
                     const lunaticvibes::Time& t, int slot);
    void _judgeHold(chart::NoteLaneCategory cat, chart::NoteLaneIndex idx, HitableNote& note, const JudgeRes& judge,
                    const lunaticvibes::Time& t, int slot);
    void _judgeRelease(chart::NoteLaneCategory cat, chart::NoteLaneIndex idx, HitableNote& note, const JudgeRes& judge,
                       const lunaticvibes::Time& t, int slot);
    void judgeNotePress(Input::Pad k, const lunaticvibes::Time& t, const lunaticvibes::Time& rt, int slot);
    void judgeNoteHold(Input::Pad k, const lunaticvibes::Time& t, const lunaticvibes::Time& rt, int slot);
    void judgeNoteRelease(Input::Pad k, const lunaticvibes::Time& t, const lunaticvibes::Time& rt, int slot);

    void processHp(JudgeArea judge);
    void processHpHitMine(long long mine_value);

public:
    // Register to InputWrapper
    void updatePress(InputMask& pg, const lunaticvibes::Time& t, const lunaticvibes::InputMaskTimes& tt) override;
    // Register to InputWrapper
    void updateHold(InputMask& hg, const lunaticvibes::Time& t) override;
    // Register to InputWrapper
    void updateRelease(InputMask& rg, const lunaticvibes::Time& t) override;
    // Register to InputWrapper
    void updateAxis(double s1, double s2, const lunaticvibes::Time& t) override;
    // Called by ScenePlay
    void update(const lunaticvibes::Time& t) override;

public:
    void updateJudge(const lunaticvibes::Time& t, chart::NoteLaneIndex ch, JudgeArea judge, int slot);

public:
    [[nodiscard]] GaugeType getGaugeType() const { return _gaugeProc.get_gauge().type; }

    [[nodiscard]] Option::e_lamp_type calculateLamp() const;

    [[nodiscard]] double getScore() const;
    [[nodiscard]] double getMaxMoneyScore() const;
    [[nodiscard]] unsigned getExScore() const;
    [[nodiscard]] unsigned getJudgeCount(JudgeType idx) const;
    [[nodiscard]] unsigned getJudgeCountEx(JudgeIndex idx) const;
    [[nodiscard]] std::string getModifierText() const;
    [[nodiscard]] std::string getModifierTextShort() const;

    [[nodiscard]] const lunaticvibes::NumberAnimation& getMoneyScoreAnimation() const { return _moneyScoreAnim; };
    [[nodiscard]] const lunaticvibes::NumberAnimation& getHealthAnimation() const { return _gaugeProc.get_health(); };

    [[nodiscard]] bool isNoScore() const override { return moneyScore == 0.0; }
    [[nodiscard]] bool isCleared() const override
    {
        return !isFailed() && isFinished() && _basic.health >= getClearHealth();
    }
    [[nodiscard]] bool isFailed() const override { return _isFailed; }

    [[nodiscard]] unsigned getCurrentMaxScore() const override { return notesReached * 2; }
    [[nodiscard]] unsigned getMaxScore() const override { return getNoteCount() * 2; }

    [[nodiscard]] unsigned getNoteCount() const override;
    [[nodiscard]] unsigned getMaxCombo() const override;

    void fail() override;

    void updateGlobals() override;
};
