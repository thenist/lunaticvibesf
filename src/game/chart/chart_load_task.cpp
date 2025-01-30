#include "chart_load_task.h"

#include <common/assert.h>
#include <common/chartformat/chartformat.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/thread_pool.h>
#include <common/u8.h>
#include <common/utils.h>
#include <game/scene/scene_context.h>
#include <game/sound/sound_mgr.h>

#include <functional>
#include <shared_mutex>
#include <string>

namespace r = std::ranges;
namespace v = std::views;

// NOTE: no point in trying to make this less global, as it has load samples into the global audio driver.
void lunaticvibes::load_audio(ChartFormatBase& chart, const std::function<bool()>& should_discard)
{
    LVF_ASSERT(!IsMainThread());
    // FIXME: synchronize gChartContext access between threads

    gChartContext.isSampleLoaded = false;
    gChartContext.sampleLoadedHash.reset();
    SoundMgr::freeNoteSamples();
    auto chartDir = chart.getDirectory();

    const auto file_cond = std::not_fn(&std::string::empty);
    const auto total = r::count_if(chart.wavFiles, file_cond);
    if (total == 0)
    {
        LOG_VERBOSE << "[ChartLoadTask] No audio files in " << chartDir;
        gChartContext.isSampleLoaded = true;
        gChartContext.sampleLoadedHash = gChartContext.hash;
        std::unique_lock l{gPlayContext._mutex};
        gPlayContext.wavTotal = 0;
        gPlayContext.wavLoaded = 1;
        return;
    }

    LOG_VERBOSE << "[ChartLoadTask] Loading " << total << " audio files from " << chartDir;

    {
        std::unique_lock l{gPlayContext._mutex};
        gPlayContext.wavTotal = total;
        gPlayContext.wavLoaded = 0;
    }

    std::atomic<size_t> jobs;
    std::atomic<size_t> jobs_done;
    for (auto [i, wav] : chart.wavFiles | v::enumerate)
    {
        if (!file_cond(wav)) // Make sure to keep correct 'i', it includes skipped elements.
            continue;
        if (should_discard())
            break;
        jobs++;
        lunaticvibes::post_job(jobs_done, [&, i]() {
            if (should_discard())
                return;
            Path pWav = PathFromUTF8(wav);
            fs::path p{chartDir / pWav};
#ifndef _WIN32
            p = lunaticvibes::resolve_windows_path(lunaticvibes::u8str(p));
#endif // _WIN32
            SoundMgr::loadNoteSample(p, i);
            std::unique_lock l{gPlayContext._mutex};
            ++gPlayContext.wavLoaded;
        });
    }
    LOG_VERBOSE << "[ChartLoadTask] Waiting on " << jobs << " audio load jobs";
    while (jobs != jobs_done)
        std::this_thread::yield();
    LOG_VERBOSE << "[ChartLoadTask] Audio loaded";

    if (should_discard())
    {
        LOG_DEBUG << "[ChartLoadTask] Discarding loaded audio";
        return;
    }

    gChartContext.isSampleLoaded = true;
    gChartContext.sampleLoadedHash = chart.fileHash;
}

void lunaticvibes::load_video(ChartFormatBase& chart, const std::function<bool()>& should_discard)
{
    LVF_ASSERT(!IsMainThread());
    // FIXME: synchronize gChartContext access between threads

    pushAndWaitMainThreadTask<void>([]() { gPlayContext.bgaTexture->clear(); });

    gChartContext.isBgaLoaded = false;
    gChartContext.bgaLoadedHash.reset();

    auto chartDir = chart.getDirectory();
    const auto file_cond = std::not_fn(&std::string::empty);
    const auto total = r::count_if(chart.bgaFiles, file_cond);
    if (total == 0)
    {
        LOG_VERBOSE << "[ChartLoadTask] No BGA files in " << chartDir;
        gChartContext.isBgaLoaded = true;
        gChartContext.bgaLoadedHash = gChartContext.hash;
        std::unique_lock l{gPlayContext._mutex};
        gPlayContext.bmpTotal = 0;
        gPlayContext.bmpLoaded = 1;
        return;
    }

    LOG_VERBOSE << "[ChartLoadTask] Loading " << total << " BGA files from " << chartDir;

    {
        std::unique_lock l{gPlayContext._mutex};
        gPlayContext.bmpTotal = 0;
        gPlayContext.bmpLoaded = 0;
    }

    for (auto [i, bmp] : chart.bgaFiles | v::enumerate)
    {
        if (!file_cond(bmp)) // Make sure to keep correct 'i', it includes skipped elements.
            continue;
        if (should_discard())
            break;
        const auto pBmp = PathFromUTF8(bmp);
        fs::path p{chartDir / pBmp};
#ifndef _WIN32
        p = lunaticvibes::resolve_windows_path(lunaticvibes::u8str(p));
#endif // _WIN32
        gPlayContext.bgaTexture->addBmp(i, p);
        std::unique_lock l{gPlayContext._mutex};
        ++gPlayContext.bmpLoaded;
    }

    LOG_VERBOSE << "[ChartLoadTask] BGA loaded";

    if (should_discard())
    {
        LOG_DEBUG << "[ChartLoadTask] State changed, discarding BGA";
        return;
    }

    if (std::shared_lock l{gPlayContext._mutex}; gPlayContext.bmpLoaded > 0)
        gPlayContext.bgaTexture->setLoaded();
    gPlayContext.bgaTexture->setSlotFromBMS(
        *std::reinterpret_pointer_cast<ChartObjectBMS>(gPlayContext.chartObj[PLAYER_SLOT_PLAYER]));
    gChartContext.isBgaLoaded = true;
    gChartContext.bgaLoadedHash = gChartContext.hash;
}
