#pragma once

#include <common/difficultytable/table_bms.h>
#include <common/entry/entry.h>
#include <common/hash.h>
#include <game/runtime/index/option.h>

#include <cstddef>
#include <list>
#include <memory>
#include <shared_mutex>
#include <string>
#include <utility>
#include <variant>

using std::size_t;

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
    struct SelectedEntry
    {
        Option::e_rank_type rank = Option::e_rank_type::RANK_NONE;
    };

    std::shared_mutex _mutex;
    std::list<SongListProperties> backtrace;
    EntryList entries;
    SelectedEntry selectedEntryInfo;
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
