#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "common/beat.h"

namespace fs = std::filesystem;

using Path = std::filesystem::path;
using StringContent = std::string;                               // std::ifstream, std::getline
using StringContentView = std::string_view;                      // std::ifstream, std::getline
using StringPath = Path::string_type;                            // natively-encoded path string
using StringPathView = std::basic_string_view<Path::value_type>; // natively-encoded path string
using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using uint64_t = std::uint64_t;
using namespace std::string_literals;

const size_t INDEX_INVALID = ~0;

[[nodiscard]] inline StringPath operator""_p(const char8_t* s, size_t len)
{
    return Path(std::u8string_view(s, len)).make_preferred();
}

// Exhaustive.
// Iterated on using underlying value.
enum class SkinType : uint8_t
{
    EXIT = 0,
    TITLE = 1,
    MUSIC_SELECT,
    DECIDE,
    THEME_SELECT,
    SOUNDSET,
    KEY_CONFIG,

    PLAY5,
    PLAY5_2,
    PLAY7,
    PLAY7_2,
    PLAY9,
    PLAY9_2,
    PLAY10,
    PLAY14,

    RESULT,
    COURSE_RESULT,

    RETRY_TRANS,
    COURSE_TRANS,
    EXIT_TRANS,

    PRE_SELECT,

    TMPL,
    TEST,

    MODE_COUNT
};
std::ostream& operator<<(std::ostream& os, const SkinType& mode);

namespace lunaticvibes
{

class Time;

[[nodiscard]] inline SkinType skinTypeForKeys(unsigned keys)
{
    switch (keys)
    {
    case 5: return SkinType::PLAY5;
    case 7: return SkinType::PLAY7;
    case 9: return SkinType::PLAY9;
    case 10: return SkinType::PLAY10;
    case 14: return SkinType::PLAY14;
    default: abort();
    }
}

[[nodiscard]] inline unsigned skinTypeToKeys(SkinType mode)
{
    switch (mode)
    {
    case SkinType::PLAY5:
    case SkinType::PLAY5_2: return 5;
    case SkinType::PLAY7:
    case SkinType::PLAY7_2: return 7;
    case SkinType::PLAY9:
    case SkinType::PLAY9_2: return 9;
    case SkinType::PLAY10: return 10;
    case SkinType::PLAY14: return 14;
    default: abort();
    }
}

} // namespace lunaticvibes

enum class ePlayMode
{
    SINGLE_PLAYER, // means "Single Player Mode", so DP is also included
    LOCAL_BATTLE,  // separate chart objects are required
    GHOST_BATTLE,  //
};

using GameModeKeys = unsigned int; // 5 7 9 10 14

enum class PlayModifierRandomType : uint8_t
{
    NONE = 0,
    MIRROR,
    RANDOM,
    SRAN,
    HRAN,   // Scatter
    ALLSCR, // Converge
    RRAN,
    DB_SYNCHRONIZE,
    DB_SYMMETRY,
};

enum class PlayModifierGaugeType : uint8_t
{
    NORMAL = 0,
    HARD,
    DEATH,
    EASY,
    PATTACK, // placeholder, not included ingame
    GATTACK, // placeholder, not included ingame
    EXHARD,
    ASSISTEASY,

    GRADE_NORMAL = 10,
    GRADE_HARD,
    GRADE_DEATH,
};

inline const uint8_t PLAY_MOD_ASSIST_AUTO67 = 1 << 0;  // 5keys, not implemented
inline const uint8_t PLAY_MOD_ASSIST_AUTOSCR = 1 << 1; //
inline const uint8_t PLAY_MOD_ASSIST_LEGACY = 1 << 2;  // LN head -> note, not implemented
inline const uint8_t PLAY_MOD_ASSIST_NOMINES = 1 << 3; // from beatoraja, not implemented

enum class PlayModifierHispeedFixType : uint8_t
{
    NONE,
    MAXBPM,
    MINBPM,
    AVERAGE,
    CONSTANT,
    INITIAL,
    MAIN,
};

enum class PlayModifierLaneEffectType : uint8_t
{
    OFF,
    HIDDEN,
    SUDDEN,
    SUDHID,
    LIFT,
    LIFTSUD,
};

struct PlayModifiers
{
    PlayModifierRandomType randomLeft = PlayModifierRandomType::NONE;
    PlayModifierRandomType randomRight = PlayModifierRandomType::NONE;
    PlayModifierGaugeType gauge = PlayModifierGaugeType::NORMAL;
    uint8_t assist_mask = 0;
    PlayModifierHispeedFixType hispeedFix = PlayModifierHispeedFixType::NONE;
    PlayModifierLaneEffectType laneEffect = PlayModifierLaneEffectType::OFF;
    bool DPFlip = false;
};

enum class GaugeDisplayType
{
    GROOVE,      // 80+20
    SURVIVAL,    // red
    EX_SURVIVAL, // yellow
    ASSIST_EASY, // 60+40
};

class ScoreBase
{
public:
    int notes = 0;
    int score = 0;
    double rate = 0.0;
    int fast = 0;
    int slow = 0;
    // Max combo until first break.
    int64_t first_max_combo = 0;
    // Combo at the end of the play.
    int64_t final_combo = 0;
    int64_t maxcombo = 0;
    int64_t addtime = 0;
    int64_t playcount = 0;
    int64_t clearcount = 0;

    std::string replayFileName;

public:
    ScoreBase() = default;
    virtual ~ScoreBase() = default;
};

class ScoreBMS : public ScoreBase
{
public:
    ScoreBMS() = default;

public:
    lunaticvibes::Time play_time{0};

    int exscore = 0;

    enum class Lamp
    {
        NOPLAY,
        FAILED,
        ASSIST,
        EASY,
        NORMAL,
        HARD,
        EXHARD,
        FULLCOMBO,
        PERFECT,
        MAX
    } lamp = Lamp::NOPLAY;

    int pgreat = 0;
    int great = 0;
    int good = 0;
    int bad = 0;
    int kpoor = 0;
    int miss = 0;
    int bp = 0;
    int combobreak = 0;

    // extended info
    unsigned rival_win = 3; // win / lose / draw / noplay
    double rival_rate = 0;
    Lamp rival_lamp = Lamp::NOPLAY;
};

class AxisDir
{
public:
    static constexpr int AXIS_UP = -1;
    static constexpr int AXIS_NONE = 0;
    static constexpr int AXIS_DOWN = 1;

private:
    int dir;

public:
    AxisDir() : dir(AXIS_NONE) {}
    AxisDir(double val, double minVal = 0)
    {
        if (val > minVal)
            dir = AXIS_DOWN;
        else if (val < -minVal)
            dir = AXIS_UP;
        else
            dir = AXIS_NONE;
    }
    operator int() const { return dir; }
};

class Ratio
{
protected:
    double _data;

public:
    Ratio() : Ratio(0) {}
    Ratio(double d) { _data = std::clamp(d, 0.0, 1.0); }
    operator double() const { return _data; }
};
