#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <common/beat.h>
#include <common/hash.h>
#include <common/types.h>

// Subset of chart formats.
// Currently including BMS

enum class eChartFormat
{
    UNKNOWN,

    BMS,
    BMSON,
};
eChartFormat analyzeChartType(const Path& p);

class ChartFormatBase
{
protected:
    eChartFormat _type = eChartFormat::UNKNOWN;

public:
    [[nodiscard]] constexpr eChartFormat type() const { return _type; }

public:
    ChartFormatBase() = default;
    virtual ~ChartFormatBase() = default;
    static std::shared_ptr<ChartFormatBase> createFromFile(const Path& path, uint64_t randomSeed);

protected:
    bool loaded = false;

public:
    [[nodiscard]] constexpr bool isLoaded() const { return loaded; }

    // following fields are generic info, which are stored in db
public:
    Path fileName;
    Path absolutePath;
    HashMD5 fileHash;
    HashMD5 folderHash;
    long long addTime = 0; // from epoch time
    unsigned gamemode = 7; // 5, 7, 9, 10, 14, 24?, 48?

    StringContent title;
    StringContent title2;
    StringContent artist;
    StringContent artist2;
    StringContent genre;
    StringContent version; // mostly known as difficulty name
    int playLevel = 0;
    unsigned difficulty = 3; // N/H/A
    double levelEstimated = 0.0;

    int totalLength = 0; // in seconds
    int totalNotes = 0;

    StringContent stagefile;
    StringContent backbmp;
    StringContent banner;

    static std::string formatReadmeText(const std::vector<std::pair<std::string, std::string>>&);
    // Are there readme files in chart directory.
    // NOTE: expensive to call.
    [[nodiscard]] bool checkHasReadme() const;
    // Pair of file names and their contents.
    // NOTE: expensive to call.
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> getReadmeFiles() const;

    BPM minBPM = INFINITY;
    BPM maxBPM = 0.0;
    BPM startBPM = 130.0;

    // following fields are filled during loading
public:
    std::vector<StringContent> wavFiles;
    std::vector<StringContent> bgaFiles;

    std::vector<Metre> metres;
    // std::vector<_Inherit_SpriteStatic_with_playbegin_timer_> _BGAsprites;

    bool resourceStable = true; // Some BMS come with WAV/BGA resources defined inside a #RANDOM block; This variable is
                                // to prevent incorrect caching.

public:
    [[nodiscard]] Path getDirectory() const;
};
