#pragma once

#include <common/beat.h>
#include <common/chartformat/chartformat.h>
#include <common/hash.h>
#include <common/play_modifiers.h>
#include <common/types.h>
#include <game/chart/chart.h>
#include <game/graphics/texture_bms_bga.h>
#include <game/replay/replay_chart.h>
#include <game/ruleset/ruleset.h>
#include <game/runtime/index/option.h>

#include <array>
#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <utility>

using std::size_t;

enum PLAYER_SLOT
{
    PLAYER_SLOT_PLAYER = 0,
    PLAYER_SLOT_TARGET = 1,
    PLAYER_SLOT_MYBEST = 2,
    MAX_PLAYERS = 3,
};
struct PlayContextParams
{
    static constexpr unsigned GRAPH_POINT_NUMBER{500};

    std::shared_mutex _mutex;

    SkinType mode = SkinType::PLAY7;
    bool canRetry = false;

    unsigned judgeLevel = 0;

    std::shared_ptr<TextureBmsBga> bgaTexture = std::make_shared<TextureBmsBga>();

    std::array<std::shared_ptr<ChartObjectBase>, MAX_PLAYERS> chartObj{nullptr, nullptr, nullptr};
    std::array<double, MAX_PLAYERS> initialHealth{1.0, 1.0, 1.0};
    std::array<GaugeDisplayType, MAX_PLAYERS> gaugeType{}; // resolve on ruleset construction
    std::array<PlayModifiers, MAX_PLAYERS> mods{};         // eMod:

    RulesetType rulesetType = RulesetType::BMS;
    std::array<std::shared_ptr<RulesetBase>, MAX_PLAYERS> ruleset;

    std::shared_ptr<ReplayChart> replay;
    std::shared_ptr<ReplayChart> replayMybest;

    struct MutexReplayChart
    {
        // Usage: name locks 'rl'.
        std::mutex mutex;
        std::shared_ptr<ReplayChart> replay;
    };
    std::shared_ptr<MutexReplayChart> replayNew;

    bool isCourse = false;
    size_t courseStage = 0;
    HashMD5 courseHash;
    std::vector<HashMD5> courseCharts;
    std::vector<std::shared_ptr<RulesetBase>> courseStageRulesetCopy[2];
    std::vector<Path> courseStageReplayPath;
    std::vector<Path> courseStageReplayPathNew;
    unsigned courseRunningCombo[2] = {0, 0};
    unsigned courseMaxCombo[2] = {0, 0};

    double bargraph_mybest_final;
    double bargraph_mybest_now_fallback;

    // loading indicators
    bool chartObjLoaded = false;
    bool rulesetLoaded = false;
    unsigned wavLoaded = 0;
    unsigned wavTotal = 0;
    unsigned bmpLoaded = 0;
    unsigned bmpTotal = 0;

    lunaticvibes::Time remainTime;

    uint64_t randomSeed;

    bool isAuto = false;
    bool isReplay = false;
    bool haveReplay = false;
    bool isBattle = false; // Note: DB is NOT Battle

    struct PlayerState
    {
        double hispeed = 2.0;
        lunaticvibes::Time hispeedGradientStart;
        double hispeedGradientFrom = 2.0;
        double hispeedGradientNow = 2.0;

    } playerState[2];

    bool shift1PNotes5KFor7KSkin = false;
    bool shift2PNotes5KFor7KSkin = false;
    constexpr int shiftFiveKeyForSevenKeyIndex(const bool isFiveKey)
    {
        if (!isFiveKey)
            return -1;
        if (shift1PNotes5KFor7KSkin)
            return shift2PNotes5KFor7KSkin ? 3 : 2;
        return shift2PNotes5KFor7KSkin ? 1 : 0;
    }
};

// byGauge - ignore full combo+ and limit lamp type by gauge.
std::pair<bool, Option::e_lamp_type> getSaveScoreType(bool byGauge = true);
void clearContextPlayForRetry();
void clearContextPlay();
void prepareChartForPlay(std::shared_ptr<ChartFormatBase> chart_, unsigned battleType);

void pushGraphPoints();

namespace lunaticvibes
{
double getSysLoadProgress();
double getWavLoadProgress();
double getBgaLoadProgress();
} // namespace lunaticvibes
