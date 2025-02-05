#pragma once

#include <cstdint>

using uint8_t = std::uint8_t;

// Saved in replays.
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

// Saved in replays.
enum class PlayModifierGaugeType : uint8_t
{
    NORMAL = 0,
    HARD = 1,
    DEATH = 2,
    EASY = 3,
    PATTACK = 4, // placeholder, not included ingame
    GATTACK = 5, // placeholder, not included ingame
    EXHARD = 6,
    ASSISTEASY = 7,

    GRADE_NORMAL = 10,
    GRADE_HARD = 11,
    GRADE_DEATH = 12,
};

enum PLAY_MOD_ASSIST_MASK
{
    PLAY_MOD_ASSIST_AUTO67 = 1 << 0,  // 5keys, not implemented
    PLAY_MOD_ASSIST_AUTOSCR = 1 << 1, //
    PLAY_MOD_ASSIST_LEGACY = 1 << 2,  // LN head -> note, not implemented
    PLAY_MOD_ASSIST_NOMINES = 1 << 3, // from beatoraja, not implemented
};

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
