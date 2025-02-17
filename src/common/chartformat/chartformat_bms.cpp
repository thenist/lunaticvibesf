#include "chartformat_bms.h"

#include <common/assert.h>
#include <common/encoding.h>
#include <common/log.h>
#include <common/types.h>
#include <common/u8.h>
#include <common/utils.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string_view>
#include <utility>

namespace r = std::ranges;

using lunaticvibes::parser_bms::JudgeDifficulty;

[[nodiscard]] static unsigned char ascii_tolower(unsigned char c)
{
    if (c > 127) // UTF-8 or something
        return c;
    return std::tolower(c);
};

[[nodiscard]] static bool istrcontains(std::string_view lhs, std::string_view rhs)
{
    return r::contains_subrange(lhs, rhs, {}, ascii_tolower);
}

// https://hitkey.nekokan.dyndns.info/cmds.htm#RANK
std::optional<JudgeDifficulty> lunaticvibes::parser_bms::parse_rank(const int value)
{
    switch (value)
    {
    case 0: return JudgeDifficulty::VERYHARD;
    case 1: return JudgeDifficulty::HARD;
    case 2: return JudgeDifficulty::NORMAL;
    case 3: return JudgeDifficulty::EASY;
    // LR2 uses NORMAL as a fallback on invalid values, including 4, but lr2oraja uses NORMAL explicitly.
    // https://github.com/wcko87/lr2oraja/blob/readme/README.md
    // TODO: parse as VERY EASY, handle on ruleset side.
    case 4: LOG_DEBUG << "[BMS] #RANK 4, using 2 instead"; return JudgeDifficulty::NORMAL;
    default: LOG_DEBUG << "[BMS] Invalid #RANK: " << value; return std::nullopt;
    }
    lunaticvibes::assert_failed("lunaticvibes::parser_bms::parse_rank");
}

ChartFormatBMS::ChartFormatBMS() : ChartFormatBMSMeta()
{
    wavFiles.resize(MAXSAMPLEIDX + 1);
    bgaFiles.resize(MAXSAMPLEIDX + 1);
    metres.resize(MAXBARIDX + 1);
}

ChartFormatBMS::ChartFormatBMS(const Path& filePath, uint64_t randomSeed) : ChartFormatBMS()
{
    initWithFile(filePath, randomSeed);
}

ChartFormatBMS::ChartFormatBMS(std::stringstream& bmsFile, eFileEncoding encoding, uint64_t randomSeed)
    : ChartFormatBMS()
{
    initWithText(bmsFile, encoding, randomSeed);
}

int ChartFormatBMS::initWithFile(const Path& filePath, uint64_t randomSeed)
{
    using err = ErrorCode;
    if (loaded)
    {
        // errorCode = err::ALREADY_INITIALIZED;
        // errorLine = 0;
        return 1;
    }

    fileName = filePath.filename();
    absolutePath = std::filesystem::absolute(filePath);
    std::ifstream ifsFile{absolutePath};
    if (ifsFile.fail())
    {
        errorCode = err::FILE_ERROR;
        errorLine = 0;
        LOG_WARNING << "[BMS] " << absolutePath << " File ERROR";
        return 1;
    }

    // copy the whole file into ram, once for all
    std::stringstream bmsFile;
    bmsFile << ifsFile.rdbuf();
    bmsFile.sync();
    ifsFile.close();

    auto encoding = getFileEncoding(bmsFile);

    LOG_DEBUG << "[BMS] File (" << getFileEncodingName(encoding) << "): " << absolutePath;

    if (lunaticvibes::iequals(lunaticvibes::s(filePath.extension().u8string()), ".pms"))
        isPMS = true;

    return initWithText(bmsFile, encoding, randomSeed);
}

int ChartFormatBMS::initWithText(std::stringstream& bmsFile, eFileEncoding encoding, uint64_t randomSeed)
{
    using err = ErrorCode;

    // 拉面早就看出bms有多难读，直接鸽了我5年

    unsigned srcLine = 0;

    // #RANDOM related parameters
    std::stack<int> randomValueMax;
    std::stack<int> randomValue;
    std::stack<std::set<int>> randomUsedValues;
    std::stack<std::stack<int>> ifStack;
    std::stack<int> ifValue;

    // implicit parameters
    bool hasDifficulty = false;
    bool only_check_note_existence = false; // TODO: check if it works when RANDOM is nested. It probably doesn't.

    for (StringContent lineBuf, lineBuf_; std::getline(bmsFile, lineBuf_);)
    {
        srcLine++;
        if (lineBuf_.length() <= 1)
            continue;

        lunaticvibes::trim_in_place(lineBuf_);
        lunaticvibes::to_utf8(lineBuf_, encoding, lineBuf);

        StringContentView buf(lineBuf);
        if (buf[0] != '#')
            continue;

        // parsing
        try
        {
            auto spacePos = std::min(buf.length(), buf.find_first_of(' '));

            // supporting single level control flow (#RANDOM, #IF, etc.), matching with LR2's capability
            if (!randomUsedValues.empty() && static_cast<int>(randomUsedValues.top().size()) < randomValue.top())
            {
                StringContentView v = buf;
                if (ifValue.empty() && !v.empty() && v[0] == '#' && !lunaticvibes::iequals(v.substr(0, 3), "#IF") &&
                    !lunaticvibes::iequals(v.substr(0, 6), "#ENDIF") &&
                    !lunaticvibes::iequals(v.substr(0, 10), "#ENDRANDOM"))
                {
                    LOG_WARNING
                        << "[BMS] definition found after all IF blocks finished, assuming #ENDRANDOM is missing. "
                        << absolutePath << "@" << srcLine;
                    randomValueMax.pop();
                    randomValue.pop();
                    randomUsedValues.pop();
                    ifStack.pop();
                }
            }

            if (spacePos > 1 && lunaticvibes::iequals(buf.substr(0, 7), "#RANDOM"))
            {
                StringContentView value = spacePos < buf.length() ? buf.substr(spacePos + 1) : "";
                int iValue = toInt(value);
                if (iValue == 0)
                {
                    LOG_WARNING << "[BMS] Invalid #RANDOM value found. " << absolutePath << "@" << srcLine;
                    continue;
                }

                haveRandom = true;
                std::mt19937_64 rng(randomSeed);
                int rngValue = (rng() % toInt(value)) + 1;
                randomValueMax.push(iValue);
                randomValue.push(rngValue);
                randomUsedValues.emplace();

                ifStack.push(ifValue);
                ifValue = std::stack<int>();

                continue;
            }

            if (!randomValue.empty())
            {
                // read IF headers outside of blocks
                if (spacePos > 1)
                {
                    StringContentView key = buf.substr(1, spacePos - 1);
                    StringContentView value = spacePos < buf.length() ? buf.substr(spacePos + 1) : "";
                    if (lunaticvibes::iequals(key, "IF"))
                    {
                        int ifBlockValue = toInt(value);
                        if (randomUsedValues.top().contains(ifBlockValue))
                        {
                            LOG_WARNING << "[BMS] duplicate #IF value found. " << absolutePath << "@" << srcLine;
                        }

                        // one level control flow
                        if (!ifValue.empty())
                        {
                            LOG_WARNING << "[BMS] unexpected #IF found, assuming #ENDIF is missing. " << absolutePath
                                        << "@" << srcLine;
                            randomUsedValues.top().emplace(ifValue.top());
                            ifValue.pop();
                        }

                        ifValue.push(ifBlockValue);

                        continue;
                    }
                    else if (lunaticvibes::iequals(key, "ENDIF"))
                    {
                        if (!ifValue.empty())
                        {
                            randomUsedValues.top().emplace(ifValue.top());
                            ifValue.pop();
                            only_check_note_existence = false;
                        }
                        else
                        {
                            LOG_WARNING << "[BMS] unexpected #ENDIF found. " << absolutePath << "@" << srcLine;
                        }

                        continue;
                    }
                    else if (lunaticvibes::iequals(key, "ENDRANDOM"))
                    {
                        if (!ifValue.empty())
                        {
                            LOG_WARNING << "[BMS] #ENDRANDOM found before #ENDIF. " << absolutePath << "@" << srcLine;
                        }
                        randomValueMax.pop();
                        randomValue.pop();
                        randomUsedValues.pop();

                        if (!ifStack.empty())
                        {
                            ifValue = ifStack.top();
                            ifStack.pop();
                        }
                        else
                        {
                            ifValue = std::stack<int>();
                        }

                        continue;
                    }
                }

                if (!randomUsedValues.empty() &&
                    static_cast<int>(randomUsedValues.top().size()) == randomValueMax.top())
                {
                    randomValueMax.pop();
                    randomValue.pop();
                    randomUsedValues.pop();

                    if (!ifStack.empty())
                    {
                        ifValue = ifStack.top();
                        ifStack.pop();
                    }
                    else
                    {
                        ifValue = std::stack<int>();
                    }
                }
                else if (ifValue.empty())
                {
                    // skip orphan blocks IF value blocks
                    continue;
                }
                else if (ifValue.top() != randomValue.top())
                {
                    only_check_note_existence = true;
                }
            }

            static constexpr auto is09az = [](const char c) {
                // std::isalnum can be affected by locale. We only need 0-9A-Za-z.
                return std::isupper(c) || std::islower(c) || std::isdigit(c);
            };

            static constexpr auto is_note_cmd = [](std::string_view cmd) {
                // (#[\d]{3}[0-9A-Za-z]{2}:.*)
                if (cmd.size() <= std::string_view{"#000AA:"}.size())
                    return false;
                return cmd[0] == '#' && std::isdigit(cmd[1]) && std::isdigit(cmd[2]) && std::isdigit(cmd[3]) &&
                       is09az(cmd[4]) && is09az(cmd[5]);
            };

            if (!is_note_cmd(buf))
            {
                auto spacePos = std::min(buf.length(), buf.find_first_of(' '));
                if (spacePos <= 1)
                    continue;

                StringContentView key = buf.substr(1, spacePos - 1);
                StringContentView value = spacePos < buf.length() ? buf.substr(spacePos + 1) : "";

                auto matches_36_key = [](std::string_view xx, std::string_view key) {
                    if (static constexpr auto idx_len = 2; key.length() != xx.length() + idx_len)
                        return false;
                    if (!lunaticvibes::iequals(key.substr(0, xx.length()), xx))
                        return false;
                    return is09az(key[key.length() - 1]) && is09az(key[key.length() - 2]);
                };

                if (key.empty())
                    continue;
                if (value.empty())
                    continue;

                // https://hitkey.nekokan.dyndns.info/cmds.htm
                // digits
                if (lunaticvibes::iequals(key, "PLAYER"))
                {
                    // https://hitkey.nekokan.dyndns.info/cmds.htm#PLAYER
                    if (const auto player = toInt(value); player != 1 && player != 3)
                    {
                        // NOTE: follows LR2, only one gauge is supported.
                        LOG_DEBUG << "[BMS] #PLAYER " << player
                                  << " is not supported, will guess either 1 or 3 from parsed channels instead";
                    }
                }
                else if (lunaticvibes::iequals(key, "RANK"))
                {
                    raw_rank = toInt(value);
                    rank = lunaticvibes::parser_bms::parse_rank(raw_rank);
                }
                else if (lunaticvibes::iequals(key, "DEFEXRANK"))
                {
                    static constexpr int default_defexrank{100};
                    if (toInt(value, -1) != default_defexrank)
                    {
                        // TODO(far future): support DEFEXRANK. Would need to support both LR2 and the new way.
                        LOG_DEBUG << "[BMS] DEFEXRANK is not supported";
                    }
                }
                else if (lunaticvibes::iequals(key, "TOTAL"))
                    total = toInt(value);
                else if (lunaticvibes::iequals(key, "PLAYLEVEL"))
                {
                    playLevel = toInt(value);
                    levelEstimated = double(playLevel);
                }
                else if (lunaticvibes::iequals(key, "DIFFICULTY"))
                {
                    difficulty = toInt(value);
                    hasDifficulty = true;
                }
                else if (lunaticvibes::iequals(key, "BPM"))
                    bpm = toDouble(value);

                // strings
                else if (lunaticvibes::iequals(key, "TITLE"))
                    title.assign(value.begin(), value.end());
                else if (lunaticvibes::iequals(key, "SUBTITLE"))
                    title2.assign(value.begin(), value.end());
                else if (lunaticvibes::iequals(key, "ARTIST"))
                    artist.assign(value.begin(), value.end());
                else if (lunaticvibes::iequals(key, "SUBARTIST"))
                    artist2.assign(value.begin(), value.end());
                else if (lunaticvibes::iequals(key, "GENRE"))
                    genre.assign(value.begin(), value.end());
                else if (lunaticvibes::iequals(key, "STAGEFILE"))
                    stagefile.assign(value.begin(), value.end());
                else if (lunaticvibes::iequals(key, "BANNER"))
                    banner.assign(value.begin(), value.end());
                else if (lunaticvibes::iequals(key, "PREVIEW"))
                {
                    dedicatedPreview.assign(value.begin(), value.end());
                }
                else if (lunaticvibes::iequals(key, "BACKBMP"))
                {
                    backbmp.assign(value.begin(), value.end());
                }
                else if (lunaticvibes::iequals(key, "COMMENT"))
                {
                    // https://hitkey.nekokan.dyndns.info/cmds.htm#COMMENT
                    // Usually surrounded by quotes, sometimes not.
                    _comments.emplace_back(lunaticvibes::trim(value, "\""));
                }
                else if (lunaticvibes::iequals(key, "LNTYPE"))
                {
                    if (toInt(value, -1) != 1)
                    {
                        LOG_DEBUG << "[BMS] LNTYPE '" << value << "' unhandled?";
                    }
                }
                else if (lunaticvibes::iequals(key, "LNOBJ") && value.length() >= 2)
                {
                    if (!lnobjSet.empty())
                    {
                        LOG_WARNING << "[BMS] Multiple #LNOBJ found. " << absolutePath << "@" << srcLine;
                        lnobjSet.clear();
                    }
                    lnobjSet.insert(base36(value[0], value[1]));
                }

                // #???xx
                else if (matches_36_key("wav", key))
                {
                    int idx = base36(key[3], key[4]);
                    wavFiles[idx].assign(value.begin(), value.end());
                    if (!ifStack.empty())
                        resourceStable = false;
                }
                else if (matches_36_key("bmp", key))
                {
                    int idx = base36(key[3], key[4]);
                    if (idx != 0)
                    {
                        bgaFiles[idx].assign(value.begin(), value.end());
                        if (!ifStack.empty())
                            resourceStable = false;
                    }
                }
                else if (matches_36_key("bpm", key))
                {
                    int idx = base36(key[3], key[4]);
                    if (idx != 0)
                        exBPM[idx] = toDouble(value);
                }
                else if (matches_36_key("stop", key))
                {
                    int idx = base36(key[4], key[5]);
                    if (idx != 0)
                        stop[idx] = toDouble(value);
                }
                else
                {
                    std::string msg;
                    msg += "[BMS] Unknown command key='";
                    msg += key;
                    msg += "' value='";
                    msg += value;
                    msg += "'";
                    lunaticvibes::verify_failed(msg.c_str());
                }
            }
            else // #zzzxy:......
            {
                StringContentView key = buf.substr(1, 5);
                StringContentView value = buf.substr(7);

                if (value.empty())
                {
                    LOG_WARNING << "[BMS] Empty note line detected. " << absolutePath << "@" << srcLine;
                    errorLine = srcLine;
                    errorCode = err::NOTE_LINE_ERROR;
                    return 1;
                }

                unsigned bar = toInt(key.substr(0, 3));
                if (lastBarIdx < bar)
                {
                    lastBarIdx = bar;
                }

                {
                    const auto x_ = static_cast<int>(base36(key[3]));
                    const auto _y = static_cast<int>(base36(key[4]));

                    if (x_ == 0) // 0x: basic info
                    {
                        if (!only_check_note_existence)
                        {
                            switch (_y)
                            {
                            case 1: // 01: BGM
                                seqToLane36(chBGM[bgmLayersCount[bar]][bar], value);
                                ++bgmLayersCount[bar];
                                break;
                            case 2: // 02: Bar Length
                                metres[bar] = toDouble(value);
                                haveMetricMod = true;
                                break;
                            case 3: // 03: BPM change
                                seqToLane16(chBPMChange[bar], value);
                                haveBPMChange = true;
                                break;
                            case 4: // 04: BGA Base
                                seqToLane36(chBGABase[bar], value);
                                haveBGA = true;
                                break;
                            case 6: // 06: BGA Poor
                                seqToLane36(chBGAPoor[bar], value);
                                haveBGA = true;
                                break;
                            case 7: // 07: BGA Layer
                                seqToLane36(chBGALayer[bar], value);
                                haveBGA = true;
                                break;
                            case 8: // 08: ExBPM
                                seqToLane36(chExBPMChange[bar], value);
                                haveBPMChange = true;
                                break;
                            case 9: // 09: Stop
                                seqToLane36(chStop[bar], value);
                                haveStop = true;
                                break;
                            default: LOG_DEBUG << "[BMS] Unhandled x=0 (basic info) << y=" << _y; break;
                            }
                        }
                    }
                    else // not 0x
                    {
                        if (!isPMS && _y == 7)
                        {
                            LOG_WARNING << "[BMS] #xxxX7 lanes are not supported. " << absolutePath << "@" << srcLine;
                            continue;
                        }

                        auto [side, idx] = isPMS ? getLaneIndexPMS(x_, _y) : getLaneIndexBME(x_, _y);
                        unsigned chIdx = idx + (side == 1 ? 10 : 0);
                        if (side >= 0)
                        {
                            if (!only_check_note_existence)
                            {
                                switch (x_)
                                {
                                case 1: // 1x: 1P visible
                                case 2: // 2x: 2P visible
                                    seqToLane36(chNotesRegular[chIdx][bar], value);
                                    haveNote = true;
                                    if (side == 1)
                                        haveAny_2 = true;
                                    break;
                                case 3: // 3x: 1P invisible
                                case 4: // 4x: 2P invisible
                                    seqToLane36(chNotesInvisible[chIdx][bar], value);
                                    haveInvisible = true;
                                    if (side == 1)
                                        haveAny_2 = true;
                                    break;
                                case 5: // 5x: 1P LN
                                case 6: // 6x: 2P LN
                                    haveLNchannels = true;
                                    if (!lnobjSet.empty())
                                    {
                                        // Note: there is so many possibilities of conflicting LN definition. Add all LN
                                        // channel notes as regular notes
                                        channel noteLane;
                                        seqToLane36(noteLane, value, channel::NoteParseValue::LN);
                                        unsigned scale =
                                            chNotesRegular[chIdx][bar].relax(noteLane.resolution) / noteLane.resolution;
                                        for (auto& note : noteLane.notes)
                                        {
                                            note.segment *= scale;
                                            bool noteInserted = false;
                                            channel& chTarget = chNotesRegular[chIdx][bar];
                                            for (auto it = chTarget.notes.begin(); it != chTarget.notes.end(); ++it)
                                            {
                                                if (it->segment > note.segment ||
                                                    (it->segment == note.segment && it->value > note.value))
                                                {
                                                    chTarget.notes.insert(it, note);
                                                    noteInserted = true;
                                                    break;
                                                }
                                            }
                                            if (!noteInserted)
                                            {
                                                chTarget.notes.push_back(note);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // #LNTYPE 1
                                        seqToLane36(chNotesLN[chIdx][bar], value, channel::NoteParseValue::LN);
                                        haveLN = true;
                                        if (side == 1)
                                            haveAny_2 = true;
                                    }
                                    break;
                                case 0xD: // Dx: 1P mine
                                case 0xE: // Ex: 2P mine
                                    seqToLane36(chMines[chIdx][bar], value);
                                    haveMine = true;
                                    break;
                                default: LOG_VERBOSE << "[BMS] Unhandled x=" << x_ << "; y=" << _y; break;
                                }
                            }

                            switch (idx)
                            {
                            case 6:
                            case 7:
                                if (side == 1)
                                    have67_2 = true;
                                else
                                    have67 = true;
                                break;
                            case 8:
                            case 9:
                                if (side == 1)
                                    have89_2 = true;
                                else
                                    have89 = true;
                                break;
                            }
                        }
                        else
                        {
                            LOG_VERBOSE << "[BMS] side<0; x=" << x_ << " y=" << _y;
                        }
                    }
                }
            }
        }
        catch (const std::invalid_argument&)
        {
            errorCode = err::TYPE_MISMATCH;
            throw;
        }
        catch (const std::out_of_range&)
        {
            errorCode = err::VALUE_ERROR;
            throw;
        }
        catch (const std::exception&)
        {
            errorLine = srcLine;
            return 1;
        }
    }

    // implicit subtitle
    if (title2.empty())
    {
        static constexpr std::pair<std::string_view, std::string_view> subtitle_delims[] = {
            {"-", "-"}, {"~", "~"}, {"(", ")"}, {"[", "]"}, {"<", ">"}, {"〜", "〜"},
            // Not sure if 〜 is needed. If not, this can also be converted to pair<char,char>.
        };
        for (auto [ld, rd] : subtitle_delims)
        {
            auto l = title.find(ld);
            if (l == StringContent::npos)
                continue;
            auto r = title.find(rd, l + 1);
            if (r == StringContent::npos || r + 1 != title.size())
                continue;
            auto v = std::string_view{title};
            title2 = v.substr(l, r);
            title = v.substr(0, l);
        }
    }

    // implicit difficulty
    if (!hasDifficulty)
    {
        static const std::pair<std::string_view, int> difficulties[] = {
            {"easy", 1},  {"beginner", 1}, {"light", 1},   {"normal", 2}, {"standard", 2}, {"hard", 3},
            {"hyper", 3}, {"ex", 4},       {"another", 4}, {"insane", 4}, {"lunatic", 4},  {"maniac", 4},
        };

        difficulty = 2; // defaults to normal

        for (auto [name, diff] : difficulties)
        {
            if (istrcontains(title2, name) || istrcontains(title, name))
            {
                difficulty = diff;
                break;
            }
        }
    }

    for (size_t i = 0; i <= lastBarIdx; i++)
        if (metres[i].toDouble() == 0.0)
            metres[i] = Metre(4, 4);

    // pick LNs out of notes for each lane
    if (!lnobjSet.empty() || haveLNchannels)
    {
        for (unsigned chIdx = 0; chIdx < 20; ++chIdx)
        {
            if (!chNotesRegular.contains(chIdx))
                continue;

            decltype(std::declval<channel>().notes.begin()) LNhead;
            unsigned bar_head = 0;
            unsigned resolution_head = 1;
            unsigned bar_curr = 0;
            bool hasHead = false;

            // find next LN head
            for (; bar_curr <= lastBarIdx; bar_curr++)
            {
                if (chNotesRegular[chIdx][bar_curr].notes.empty())
                    continue;

                auto& noteList = chNotesRegular[chIdx][bar_curr].notes;
                auto itNote = noteList.begin();
                while (itNote != noteList.end())
                {
                    // Regular note inside a LN (can be seen with o2mania + #LNTYPE 1) is not allowed. Handle any
                    // following note as LN tail.
                    if (hasHead && (lnobjSet.count(itNote->value) || (LNhead->flags & channel::NoteParseValue::LN)))
                    {
                        unsigned segment =
                            LNhead->segment * chNotesLN[chIdx][bar_head].relax(resolution_head) / resolution_head;
                        unsigned value = LNhead->value;
                        unsigned resolution_tail = chNotesRegular[chIdx][bar_curr].resolution;
                        unsigned segment2 =
                            itNote->segment * chNotesLN[chIdx][bar_curr].relax(resolution_tail) / resolution_tail;
                        unsigned value2 = itNote->value;
                        chNotesLN[chIdx][bar_head].notes.push_back({segment, value});
                        chNotesLN[chIdx][bar_curr].notes.push_back({segment2, value2});

                        haveLN = true;

                        chNotesRegular[chIdx][bar_head].notes.erase(LNhead);
                        auto itPrev = itNote;
                        bool resetItNote = (itNote == noteList.begin());
                        if (!resetItNote)
                            itPrev--;
                        chNotesRegular[chIdx][bar_curr].notes.erase(itNote);
                        itNote = resetItNote ? noteList.begin() : ++itPrev;

                        bar_head = 0;
                        resolution_head = 1;
                        hasHead = false;
                    }
                    else
                    {
                        LNhead = itNote;
                        bar_head = bar_curr;
                        resolution_head = chNotesRegular[chIdx][bar_curr].resolution;
                        hasHead = true;

                        ++itNote;
                    }
                }
            }

            if (bar_head <= lastBarIdx)
                chNotesLN[chIdx][bar_head].sortNotes();
            if (bar_curr <= lastBarIdx)
                chNotesLN[chIdx][bar_curr].sortNotes();
        }
    }

    // Get statistics
    if (haveNote)
    {
        for (unsigned bar = 0; bar <= lastBarIdx; bar++)
            notes_scratch += chNotesRegular[0][bar].notes.size() + chNotesRegular[10][bar].notes.size();

        for (unsigned lane = 0; lane < chNotesRegular.size(); ++lane)
        {
            if (lane == 0 || lane == 10)
                continue;
            for (const auto& [barIdx, ch] : chNotesRegular[lane])
                notes_key += ch.notes.size();
        }
        notes_total += notes_scratch + notes_key;
    }

    if (haveLN)
    {
        for (unsigned bar = 0; bar <= lastBarIdx; bar++)
            notes_scratch_ln += chNotesLN[0][bar].notes.size() + chNotesLN[10][bar].notes.size();
        notes_scratch_ln /= 2;

        for (unsigned lane = 0; lane < chNotesLN.size(); ++lane)
        {
            if (lane == 0 || lane == 10)
                continue;
            for (const auto& [barIdx, ch] : chNotesLN[lane])
                notes_key_ln += ch.notes.size();
        }
        notes_key_ln /= 2;

        notes_total += notes_scratch_ln + notes_key_ln;
    }

    for (const auto& [chIdx, chLane] : chMines)
        for (const auto& [barIdx, ch] : chLane)
            notes_mine += ch.notes.size();

    minBPM = bpm;
    maxBPM = bpm;
    startBPM = bpm;
    if (haveBPMChange)
    {
        for (unsigned m = 0; m <= lastBarIdx; m++)
        {
            for (const auto& ns : chBPMChange[m].notes)
            {
                if (ns.value > maxBPM)
                    maxBPM = ns.value;
                if (ns.value < minBPM)
                    minBPM = ns.value;
            }
            for (const auto& ns : chExBPMChange[m].notes)
            {
                if (exBPM[ns.value] > maxBPM)
                    maxBPM = exBPM[ns.value];
                if (exBPM[ns.value] < minBPM)
                    minBPM = exBPM[ns.value];
            }
        }
    }

    // Notes swapped by https://hitkey.nekokan.dyndns.info/cmds.htm#PMS
    // Sometimes there is a mix of those note channels, for example `Calamity Fortune [9 THOUGHTLESS]`.
    // LR2 seems to merge together at the same time.
    if (isPMS)
    {
        gamemode = 9;
        player = 1;

        // 11	12	13	14	15	18	19	16	17	not known or well known
        std::swap(chNotesRegular[6], chNotesRegular[8]);
        std::swap(chNotesRegular[7], chNotesRegular[9]);
        std::swap(chNotesInvisible[6], chNotesInvisible[8]);
        std::swap(chNotesInvisible[7], chNotesInvisible[9]);
        std::swap(chNotesLN[6], chNotesLN[8]);
        std::swap(chNotesLN[7], chNotesLN[9]);
        std::swap(chMines[6], chMines[8]);
        std::swap(chMines[7], chMines[9]);
        have67 = true;
        have67_2 = false;
        have89 = true;
        have89_2 = false;
        haveAny_2 = false;
        if (have67_2)
        {
            // 21	22	23	24	25	28	29	26	27	2P-side (right)
            LOG_DEBUG << "[BMS] 18KEYS is not supported, parsing as 9KEYS";
            std::swap(chNotesRegular[16], chNotesRegular[18]);
            std::swap(chNotesRegular[17], chNotesRegular[19]);
            std::swap(chNotesInvisible[16], chNotesInvisible[18]);
            std::swap(chNotesInvisible[17], chNotesInvisible[19]);
            std::swap(chNotesLN[16], chNotesLN[18]);
            std::swap(chNotesLN[17], chNotesLN[19]);
            std::swap(chMines[16], chMines[18]);
            std::swap(chMines[17], chMines[19]);
            have67_2 = true;
        }

        // 11	12	13	14	15	22	23	24	25	standard PMS
        auto imbue = [](LaneMap& lhs, LaneMap& rhs) {
            if (lhs.empty())
                std::swap(lhs, rhs);
            for (auto& [k, rv] : rhs)
                lhs[k].imbue(rv);
        };
        imbue(chNotesRegular[6], chNotesRegular[12]);
        imbue(chNotesRegular[7], chNotesRegular[13]);
        imbue(chNotesRegular[8], chNotesRegular[14]);
        imbue(chNotesRegular[9], chNotesRegular[15]);
        imbue(chNotesInvisible[6], chNotesInvisible[12]);
        imbue(chNotesInvisible[7], chNotesInvisible[13]);
        imbue(chNotesInvisible[8], chNotesInvisible[14]);
        imbue(chNotesInvisible[9], chNotesInvisible[15]);
        imbue(chNotesLN[6], chNotesLN[12]);
        imbue(chNotesLN[7], chNotesLN[13]);
        imbue(chNotesLN[8], chNotesLN[14]);
        imbue(chNotesLN[9], chNotesLN[15]);
        imbue(chMines[6], chMines[12]);
        imbue(chMines[7], chMines[13]);
        imbue(chMines[8], chMines[14]);
        imbue(chMines[9], chMines[15]);
    }
    else
    {
        player = (have67_2 || haveAny_2) ? 3 : 1;
        if (have67 || have67_2)
            gamemode = (player == 1 ? 7 : 14);
        else
            gamemode = (player == 1 ? 5 : 10);
    }

    fileHash = md5file(absolutePath);
    LOG_INFO << "[BMS] File (" << getFileEncodingName(encoding) << "): " << absolutePath
             << " MD5: " << fileHash.hexdigest();

    loaded = true;

    return 0;
}

int ChartFormatBMS::seqToLane36(channel& ch, StringContentView str, unsigned flags)
{
    const size_t length = str.length();
    if (length / 2 == 0)
    {
        LOG_VERBOSE << "[BMS] Invalid string length " << str;
        return 1;
    }
    for (auto c : str)
    {
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
        {
            LOG_VERBOSE << "[BMS] Invalid symbol in string " << str;
            break;
        }
    }

    const auto resolution = static_cast<unsigned>(length / 2);
    const unsigned scale = ch.relax(resolution) / resolution;
    for (unsigned i = 0; i < resolution; i++)
    {
        const unsigned segment = i * scale;
        const unsigned value = base36(str[i + i], str[i + i + 1]);
        if (value == 0)
            continue;
        ch.notes.push_back({segment, value, flags});
    }
    ch.sortNotes();

    return 0;
}

int ChartFormatBMS::seqToLane16(channel& ch, StringContentView str)
{
    const size_t length = str.length();
    if (length / 2 == 0)
    {
        LOG_VERBOSE << "[BMS] Invalid string length " << str;
        return 1;
    }
    for (auto c : str)
    {
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
        {
            LOG_VERBOSE << "[BMS] Invalid symbol in string " << str;
            break;
        }
    }

    const auto resolution = static_cast<unsigned>(length / 2);
    const unsigned scale = ch.relax(resolution) / resolution;
    for (unsigned i = 0; i < resolution; i++)
    {
        const unsigned segment = i * scale;
        const unsigned value = base16(str[i + i], str[i + i + 1]);
        if (value == 0)
            continue;
        ch.notes.push_back({segment, value});
    }
    ch.sortNotes();

    return 0;
}

std::pair<int, int> ChartFormatBMS::getLaneIndexBME(int x_, int _y)
{
    int side = 0;
    int idx = 0;
    switch (x_)
    {
    case 1:   // 1x: 1P visible
    case 3:   // 3x: 1P invisible
    case 5:   // 5x: 1P LN
    case 0xD: // Dx: 1P mine
        side = 0;
        break;
    case 2:   // 2x: 2P visible
    case 4:   // 4x: 2P invisible
    case 6:   // 6x: 2P LN
    case 0xE: // Ex: 2P mine
        side = 1;
        break;
    default:
        LOG_VERBOSE << "[BMS] getLaneIndexBME returning side=-1 for x_=" << x_ << "; unused _y=" << _y;
        side = -1;
        break;
    }
    if (side >= 0)
    {
        switch (_y)
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5: idx = _y; break;
        case 6: // SCR
            idx = 0;
            break;
        case 8: // 6
            idx = 6;
            break;
        case 9: // 7
            idx = 7;
            break;
        case 7: // Free zone / pedal
            idx = 9;
            break;
        }
    }
    return {side, idx};
}

std::pair<int, int> ChartFormatBMS::getLaneIndexPMS(int x_, int _y)
{
    int side = 0;
    int idx = 0;
    switch (x_)
    {
    case 1:   // 1x: 1P visible
    case 3:   // 3x: 1P invisible
    case 5:   // 5x: 1P LN
    case 0xD: // Dx: 1P mine
        side = 0;
        break;
    case 2:   // 2x: 2P visible
    case 4:   // 4x: 2P invisible
    case 6:   // 6x: 2P LN
    case 0xE: // Ex: 2P mine
        side = 1;
        break;
    default:
        LOG_VERBOSE << "[BMS] getLaneIndexPMS returning side=-1 for x_=" << x_ << "; unused _y=" << _y;
        side = -1;
        break;
    }
    if (side >= 0)
    {
        // return as-is and handle after parsing completed. PMS 6-9 lanes definition may vary
        idx = _y;
    }
    return {side, idx};
}

auto ChartFormatBMS::getLane(LaneCode code, unsigned chIdx, unsigned barIdx) const -> const channel&
{
    static const channel emptyChannel;
    if (barIdx <= lastBarIdx)
    {
        auto getCommonLane = [](const LaneMap& ch, unsigned barIdx) -> const channel& {
            if (auto it = ch.find(barIdx); it != ch.end())
                return it->second;
            return emptyChannel;
        };

        auto getNoteLane = [&](const std::map<unsigned, LaneMap>& ch, unsigned chIdx,
                               unsigned barIdx) -> const channel& {
            if (auto it = ch.find(chIdx); it != ch.end())
                return getCommonLane(it->second, barIdx);
            return emptyChannel;
        };

        using eC = LaneCode;
        switch (code)
        {
        case eC::BGM: return getNoteLane(chBGM, chIdx, barIdx);
        case eC::BPM: return getCommonLane(chBPMChange, barIdx);
        case eC::EXBPM: return getCommonLane(chExBPMChange, barIdx);
        case eC::STOP: return getCommonLane(chStop, barIdx);
        case eC::BGABASE: return getCommonLane(chBGABase, barIdx);
        case eC::BGALAYER: return getCommonLane(chBGALayer, barIdx);
        case eC::BGAPOOR: return getCommonLane(chBGAPoor, barIdx);
        case eC::NOTE1: return getNoteLane(chNotesRegular, chIdx, barIdx);
        case eC::NOTE2: return getNoteLane(chNotesRegular, chIdx + 10, barIdx);
        case eC::NOTEINV1: return getNoteLane(chNotesInvisible, chIdx, barIdx);
        case eC::NOTEINV2: return getNoteLane(chNotesInvisible, chIdx + 10, barIdx);
        case eC::NOTELN1: return getNoteLane(chNotesLN, chIdx, barIdx);
        case eC::NOTELN2: return getNoteLane(chNotesLN, chIdx + 10, barIdx);
        case eC::NOTEMINE1: return getNoteLane(chMines, chIdx, barIdx);
        case eC::NOTEMINE2: return getNoteLane(chMines, chIdx + 10, barIdx);
        default: break;
        }
    }

    lunaticvibes::verify_failed("ChartFormatBMS::getLane");
    return emptyChannel;
}

unsigned ChartFormatBMS::channel::relax(unsigned target_resolution)
{
    const unsigned target = std::lcm(target_resolution, resolution);
    const unsigned pow = target / resolution;
    if (pow == 1)
        return target;
    resolution = target;
    for (auto& n : notes)
    {
        n.segment *= pow;
    }
    return target;
}

void ChartFormatBMS::channel::imbue(channel& c)
{
    if (notes.empty())
    {
        resolution = c.resolution;
        std::swap(notes, c.notes);
        return;
    }
    relax(c.resolution);
    for (const auto& note : c.notes)
    {
        notes.push_back(note);
    }
    c.notes.clear();
    c.resolution = 1;
    sortNotes();
}

void ChartFormatBMS::channel::sortNotes()
{
    // NOTE: stable_sort
    notes.sort([](const NoteParseValue& lhs, const NoteParseValue& rhs) { return lhs.segment < rhs.segment; });
}
