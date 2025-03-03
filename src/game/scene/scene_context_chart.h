#pragma once

#include <common/beat.h>
#include <common/chartformat/chartformat.h>
#include <common/hash.h>
#include <common/types.h>
#include <game/graphics/texture_dynamic.h>

#include <memory>
#include <mutex>

struct ChartContextParams
{
    Path path{};
    HashMD5 hash{};
    std::shared_ptr<ChartFormatBase> chart;
    std::shared_ptr<ChartFormatBase> chartMybest; // mybest obj is loaded with a different random seed

    // Fields may be modified by non-main thread.
    struct Concurrent_
    {
        std::mutex mutex;
        HashMD5 sampleLoadedHash;
        HashMD5 bgaLoadedHash;
    } concurrent;
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
