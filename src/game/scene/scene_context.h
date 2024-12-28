#pragma once

#include <common/chartformat/chartformat.h>
#include <common/difficultytable/table_bms.h>
#include <common/entry/entry.h>
#include <common/types.h>
#include <game/chart/chart.h>
#include <game/graphics/texture_extra.h>
#include <game/replay/replay_chart.h>
#include <game/ruleset/ruleset.h>
#include <game/scene/scene.h>

#include <array>
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <variant>

class ScoreDB;
class SongDB;

inline SceneType gNextScene = SceneType::SELECT;
inline bool gInCustomize = false;
inline bool gCustomizeSceneChanged = false;
inline bool gExitingCustomize = false;

////////////////////////////////////////////////////////////////////////////////

struct ChartContextParams
{
    Path path{};
    HashMD5 hash{};
    std::shared_ptr<ChartFormatBase> chart;
    std::shared_ptr<ChartFormatBase> chartMybest; // mybest obj is loaded with a different random seed

    // bool isChartSamplesLoaded;
    bool isSampleLoaded = false;
    HashMD5 sampleLoadedHash;
    bool isBgaLoaded = false;
    HashMD5 bgaLoadedHash;
    bool started = false;

    // DP flags
    bool isDoubleBattle = false;

    // For displaying purpose, typically fetch from song db directly
    StringContent title{};
    StringContent title2{};
    StringContent artist{};
    StringContent artist2{};
    StringContent genre{};
    StringContent version{};
    double level = 0.0;

    BPM minBPM = 150;
    BPM maxBPM = 150;
    BPM startBPM = 150;

    TextureDynamic texStagefile;
    TextureDynamic texBackbmp;
    TextureDynamic texBanner;
};

////////////////////////////////////////////////////////////////////////////////

constexpr unsigned MAX_PLAYERS = 3;
constexpr unsigned PLAYER_SLOT_PLAYER = 0;
constexpr unsigned PLAYER_SLOT_TARGET = 1;
constexpr unsigned PLAYER_SLOT_MYBEST = 2;
struct PlayContextParams
{
    std::shared_mutex _mutex;

    SkinType mode = SkinType::PLAY7;
    bool canRetry = false;

    unsigned judgeLevel = 0;

    std::shared_ptr<TextureBmsBga> bgaTexture = std::make_shared<TextureBmsBga>();

    std::array<std::shared_ptr<ChartObjectBase>, MAX_PLAYERS> chartObj{nullptr, nullptr, nullptr};
    std::array<double, MAX_PLAYERS> initialHealth{1.0, 1.0, 1.0};
    std::array<std::vector<int>, MAX_PLAYERS> graphGauge;
    std::array<std::vector<double>, MAX_PLAYERS> graphAcc;
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

void pushGraphPoints();

namespace lunaticvibes
{
double getSysLoadProgress();
double getWavLoadProgress();
double getBgaLoadProgress();
} // namespace lunaticvibes

////////////////////////////////////////////////////////////////////////////////

using Entry = std::pair<std::shared_ptr<EntryBase>, std::shared_ptr<ScoreBase>>;
using EntryList = std::vector<Entry>;

struct SongListProperties
{
    HashMD5 parent;
    HashMD5 folder;
    std::string name; // folder path, search query+result, etc.
    EntryList dbBrowseEntries;
    EntryList displayEntries;
    size_t index;
    bool ignoreFilters = false;
};

enum class SongListSortType
{
    DEFAULT, // LEVEL
    TITLE,
    LEVEL,
    CLEAR,
    RATE,

    TYPE_COUNT,
};

struct HelpFileOpenRequest
{
    // 0-10.
    unsigned idx;
};
struct ChartReadmeOpenRequest
{
};
using ReadmeOpenRequest = std::variant<std::monostate, HelpFileOpenRequest, ChartReadmeOpenRequest>;

struct SelectContextParams
{
    std::shared_mutex _mutex;
    std::list<SongListProperties> backtrace;
    EntryList entries;
    size_t selectedEntryIndex = 0;   // current selected entry index
    size_t highlightBarIndex = 0;    // highlighted bar index
    bool draggingListSlider = false; // is dragging slider

    size_t cursorClick = 0;    // click bar
    int cursorClickScroll = 0; // -1: scroll up / 1: scroll down / 2: decide
    bool cursorEnterPending = false;

    SongListSortType sortType = SongListSortType::DEFAULT;
    unsigned filterDifficulty = 0; // all / B / N / H / A / I (type 0 is not included)
    unsigned filterKeys = 0;       // all / 5, 7, 9, 10, 14, etc
    bool optionChangePending = false;

    std::vector<DifficultyTableBMS> tables;

    double pitchSpeed = 1.0;

    unsigned scrollTimeLength = 300; //
    int scrollDirection = 0;         // -1: up / 1: down

    int lastLaneEffectType1P = 0;

    ReadmeOpenRequest readmeOpenRequest;
    unsigned readme_line = 0;

    double bargraph_level_bar_another;
    double bargraph_level_bar_beginner;
    double bargraph_level_bar_hyper;
    double bargraph_level_bar_insane;
    double bargraph_level_bar_normal;
    double bargraph_select_mybest_bd;
    double bargraph_select_mybest_exscore;
    double bargraph_select_mybest_gd;
    double bargraph_select_mybest_gr;
    double bargraph_select_mybest_maxcombo;
    double bargraph_select_mybest_pg;
    double bargraph_select_mybest_pr;
    double bargraph_select_mybest_score;

    HashMD5 remoteRequestedChart;      // only valid when remote is requesting a new chart; reset after list change
    std::string remoteRequestedPlayer; // only valid when remote is requesting a new chart; reset after list change

    bool isArenaReady = false;
    bool isInArenaRequest = false;
    bool isArenaCancellingRequest = false;

    bool isGoingToSkinSelect = false;
    bool isGoingToKeyConfig = false;
    bool isGoingToAutoPlay = false;
    bool isGoingToReplay = false;
    bool isGoingToReboot = false;
};

void loadSongList();
void updateEntryScore(size_t idx);
void sortSongList();
void setBarInfo();
void setEntryInfo();
void setEntryInfo(size_t idx);
void setPlayModeInfo();
void switchVersion(unsigned difficulty);

void setDynamicTextures();

////////////////////////////////////////////////////////////////////////////////

struct KeyConfigContextParams
{
    GameModeKeys keys;
    std::pair<Input::Pad, int> selecting = {Input::Pad::K11, 0};
    bool modeChanged = false;
    bool skinHasAbsAxis = false;
    std::array<double, Input::Pad::KEY_COUNT> bargraphForce;
};

////////////////////////////////////////////////////////////////////////////////

struct CustomizeContextParams
{
    SkinType mode;
    bool modeUpdate = false;

    int skinDir = 0;

    bool optionUpdate = false;
    size_t optionIdx;
    int optionDir = 0;

    bool optionDragging = false;
};

////////////////////////////////////////////////////////////////////////////////

struct UpdateContextParams
{
    // shared
    lunaticvibes::Time updateTime;

    // SkinBase / Sprite
    double metre;
    unsigned bar;
};

////////////////////////////////////////////////////////////////////////////////

struct OverlayContextParams
{
    std::shared_mutex _mutex;
    std::list<std::pair<lunaticvibes::Time, StringContent>> notifications;
};

void createNotification(StringContent text);

////////////////////////////////////////////////////////////////////////////////

extern std::atomic<bool> gQuitOnFinish;
extern std::atomic<bool> gAppIsExiting;
extern ChartContextParams gChartContext;
extern PlayContextParams gPlayContext;
extern SelectContextParams gSelectContext;
extern KeyConfigContextParams gKeyconfigContext;
extern CustomizeContextParams gCustomizeContext;
extern UpdateContextParams gUpdateContext;
extern OverlayContextParams gOverlayContext;
extern std::shared_ptr<SongDB> g_pSongDB;
extern std::shared_ptr<ScoreDB> g_pScoreDB;

////////////////////////////////////////////////////////////////////////////////
