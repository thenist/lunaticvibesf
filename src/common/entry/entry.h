// Song list entry definition

#pragma once

#include "common/hash.h"
#include "common/types.h"

enum class eEntryType
{
    UNKNOWN,
    NEW_SONG_FOLDER,
    FOLDER,
    SONG,  // all charts in folder
    CHART, // one chart in folder
    CUSTOM_FOLDER,
    COURSE_FOLDER,
    RANDOM_CHART,
    RIVAL,
    RIVAL_SONG,
    RIVAL_CHART,
    NEW_COURSE,
    COURSE,
    RANDOM_COURSE,

    ARENA_FOLDER,
    ARENA_COMMAND,
    ARENA_LOBBY,

    CHART_LINK,
    REPLAY,
};

class EntryBase
{
protected:
    eEntryType _type = eEntryType::UNKNOWN;

public:
    EntryBase() = default;
    virtual ~EntryBase() = default;

public:
    // Used to uniquely identify this entry in the list.
    HashMD5 md5;
    StringContent _name;
    StringContent _name2;
    unsigned long long _addTime = 0; // from epoch time

public:
    virtual Path getPath() { return {}; }
    [[nodiscard]] constexpr eEntryType type() const { return _type; }
};
