#pragma once

#include "chartformat.h"

#include <array>
#include <list>
#include <map>
#include <optional>
#include <set>

#include <common/encoding.h>
#include <common/types.h>
#include <sstream>

namespace bms
{
const unsigned MAXSAMPLEIDX = 36 * 36;
const unsigned MAXBARIDX = 999;
enum class ErrorCode
{
    OK,
    FILE_ERROR,
    ALREADY_INITIALIZED,
    VALUE_ERROR,
    TYPE_MISMATCH,
    NOTE_LINE_ERROR,
};
enum class LaneCode
{
    BGM = 0,
    BPM,
    EXBPM,
    STOP,
    BGABASE,
    BGALAYER,
    BGAPOOR,
    NOTE1,
    NOTE2,
    NOTEINV1,
    NOTEINV2,
    NOTELN1,
    NOTELN2,
    NOTEMINE1,
    NOTEMINE2,
};
} // namespace bms

using namespace bms;

namespace lunaticvibes::parser_bms
{

enum class JudgeDifficulty : uint8_t
{
    VERYHARD = 0,
    HARD = 1,
    NORMAL = 2,
    EASY = 3,
};

// Parse #RANK.
[[nodiscard]] std::optional<JudgeDifficulty> parse_rank(int);

} // namespace lunaticvibes::parser_bms

class SceneSelect;
class SongDB;

class ChartFormatBMSMeta : public ChartFormatBase
{
public:
    // File properties.
    // Header.
    int player = 0; // 1: single, 2: couple, 3: double, 4: battle
    int raw_rank = -1;
    int total = -1;
    double bpm = 130.0;
    std::optional<lunaticvibes::parser_bms::JudgeDifficulty> rank;

    StringContent dedicatedPreview;

    // File assigned by the BMS file.
    // Ported to super class

public:
    // Properties detected when parsing.
    bool isPMS = false;
    bool haveNote = false;
    bool haveAny_2 = false;
    bool have67 = false;
    bool have67_2 = false;
    bool have89 = false;
    bool have89_2 = false;
    bool haveLN = false;
    bool haveMine = false;
    bool haveInvisible = false;
    bool haveMetricMod = false;
    bool haveStop = false;
    bool haveBPMChange = false;
    bool haveBGA = false;
    bool haveRandom = false;
    unsigned long notes_total = 0;
    unsigned long notes_key = 0;
    unsigned long notes_scratch = 0;
    unsigned long notes_key_ln = 0;
    unsigned long notes_scratch_ln = 0;
    unsigned long notes_mine = 0;
    unsigned lastBarIdx = 0;

public:
    ChartFormatBMSMeta() { _type = eChartFormat::BMS; }
    ~ChartFormatBMSMeta() override = default;
};

// the size of parsing result is kinda large..
class ChartFormatBMS : public ChartFormatBMSMeta
{
    friend class SceneSelect;
    friend class SongDB;

public:
    ChartFormatBMS();
    ChartFormatBMS(const Path& absolutePath, uint64_t randomSeed = 0);
    ChartFormatBMS(std::stringstream& bmsFile, eFileEncoding encoding, uint64_t randomSeed);
    ~ChartFormatBMS() override = default;

private:
    int initWithFile(const Path& absolutePath, uint64_t randomSeed = 0);
    int initWithText(std::stringstream& bmsFile, eFileEncoding encoding, uint64_t randomSeed);

protected:
    ErrorCode errorCode = ErrorCode::OK;
    int errorLine;

public:
    struct channel
    {
        struct NoteParseValue
        {
            unsigned segment;
            unsigned value;

            enum Flags
            {
                LN = 1 << 1,
            };
            unsigned flags;
        };
        std::list<NoteParseValue> notes{};
        unsigned resolution = 1;

        unsigned relax(unsigned target_resolution);
        // Merge with another channel.
        // \param c Channel to destructively merge with this channel.
        void imbue(channel& c);
        void sortNotes();
    };
    using LaneMap = std::map<unsigned int, channel>; // bar -> channel

protected:
    // Lanes.
    int seqToLane36(channel&, StringContentView str, unsigned flags = 0);
    int seqToLane16(channel&, StringContentView str);

    std::map<unsigned, LaneMap> chBGM{}; // lane -> [bar -> channel]
    LaneMap chStop{};
    LaneMap chBPMChange{};
    LaneMap chExBPMChange{};
    LaneMap chBGABase{};
    LaneMap chBGALayer{};
    LaneMap chBGAPoor{};

    std::map<unsigned, LaneMap> chNotesRegular{}; // lane -> [bar -> channel]
    std::map<unsigned, LaneMap> chNotesInvisible{};
    std::map<unsigned, LaneMap> chNotesLN{};
    std::map<unsigned, LaneMap> chMines{};

    std::pair<int, int> getLaneIndexBME(int x_, int _y);
    std::pair<int, int> getLaneIndexPMS(int x_, int _y);

public:
    std::set<unsigned> lnobjSet;
    bool haveLNchannels = false;

public:
    // Measures related.
    std::array<double, MAXSAMPLEIDX + 1> exBPM{};
    std::array<double, MAXSAMPLEIDX + 1> stop{};

    std::array<unsigned, MAXBARIDX + 1> bgmLayersCount{};

public:
    auto getLane(LaneCode, unsigned chIdx, unsigned measureIdx) const -> const channel&;
};
