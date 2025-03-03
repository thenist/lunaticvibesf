#include "texture_bms_bga.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <utility>

#include <common/assert.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/thread_pool.h>
#include <common/types.h>
#include <common/u8.h>
#include <common/utils.h>
#include <game/chart/chart_bms.h>
#include <game/graphics/image.h>
#include <game/graphics/texture_video.h>
#include <game/graphics/video.h>
#include <game/scene/scene_context.h>

namespace r = std::ranges;

bool TextureBmsBga::addBmp(size_t idx, Path pBmp)
{
    if (idx == size_t(-1))
        return false;

    if (!fs::exists(pBmp) && pBmp.has_extension() &&
        lunaticvibes::iequals(lunaticvibes::s(pBmp.extension().u8string()), ".bmp"))
    {
        pBmp = pBmp.parent_path() / Path(pBmp.filename().stem().u8string() + u8".jpg");

        if (!fs::exists(pBmp))
        {
            pBmp = pBmp.parent_path() / Path(pBmp.filename().stem().u8string() + u8".png");
        }
    }
    if (fs::exists(pBmp) && fs::is_regular_file(pBmp) && pBmp.has_extension())
    {
        if (lunaticvibes::is_video_file_path(pBmp))
        {
            objs[idx].type = obj::Ty::VIDEO;
            objs[idx].pt =
                std::make_shared<TextureVideo>(std::make_shared<sVideo>(pBmp, gSelectContext.pitchSpeed, false));
            LOG_DEBUG << "[TextureBmsBga] added video: " << pBmp;
            return true;
        }
        else
        {
            Image img(pBmp);
            objs[idx].type = obj::Ty::PIC;
            objs[idx].pt = std::make_shared<Texture>(img.build_texture());

            auto layerImg = std::move(img);
            layerImg.setTransparentColorRGB(Color(0, 0, 0, 255));
            objs_layer[idx].type = obj::Ty::PIC;
            objs_layer[idx].pt = std::make_shared<Texture>(layerImg.build_texture());

            LOG_DEBUG << "[TextureBmsBga] added pic: " << pBmp;
            return true;
        }
    }
    else
    {
        objs[idx].type = obj::Ty::EMPTY;
        objs_layer[idx].type = obj::Ty::EMPTY;
        LOG_DEBUG << "[TextureBmsBga] file not found, added dummy: " << pBmp;
    }
    return false;
}

bool TextureBmsBga::setSlot(size_t idx, const lunaticvibes::Time& time, bool base, bool layer, bool poor)
{
    if (base)
        baseSlot.emplace_back(time, idx);
    if (layer)
        layerSlot.emplace_back(time, idx);
    if (poor)
        poorSlot.emplace_back(time, idx);
    return true;
}

void TextureBmsBga::sortSlot()
{
    r::sort(baseSlot);
    r::sort(layerSlot);
    r::sort(poorSlot);
    baseIt = baseSlot.begin();
    layerIt = layerSlot.begin();
    poorIt = poorSlot.begin();
}

bool TextureBmsBga::setSlotFromBMS(ChartObjectBMS& bms)
{
    baseSlot.clear();
    layerSlot.clear();
    poorSlot.clear();
    const auto lBase = bms.getBgaBase();
    const auto lLayer = bms.getBgaLayer();
    const auto lPoor = bms.getBgaPoor();
    for (const auto& l : lBase)
        setSlot(l.dvalue, l.time, true, false, false);
    for (const auto& l : lLayer)
        setSlot(l.dvalue, l.time, false, true, false);
    for (const auto& l : lPoor)
        setSlot(l.dvalue, l.time, false, false, true);
    sortSlot();
    loaded = true; // FIXME: synchronize access
    return true;
}

void TextureBmsBga::seek(const lunaticvibes::Time& t)
{
    if (!isLoaded())
        return;

    auto seekSub = [&t, this](decltype(baseSlot)& slot, size_t& slotIdx, decltype(baseSlot.begin())& slotIt) {
        for (auto it = slot.begin(); it != slot.end(); ++it) // search from beginning
        {
            auto [time, idx] = *it;
            if (time <= t)
            {
                slotIdx = idx;
                slotIt = it;
                if (objs[idx].type == obj::Ty::VIDEO)
                {
                    auto pt = std::reinterpret_pointer_cast<TextureVideo>(objs[idx].pt);
                    pt->seek((t - time).norm() / 1000);
                    pt->update();
                }
                return;
            }
        }
        // slotIt = slot.end();	// not found
    };

    std::unique_lock l(idxLock);
    seekSub(baseSlot, baseIdx, baseIt);
    seekSub(layerSlot, layerIdx, layerIt);
    seekSub(poorSlot, poorIdx, poorIt);
    inPoor = false;
}

void TextureBmsBga::update(const lunaticvibes::Time& t, bool poor)
{
    if (!isLoaded())
        return;

    auto seekSub = [&t, this](decltype(baseSlot)& slot, size_t& slotIdx, decltype(baseSlot.begin())& slotIt) {
        auto it = slotIt;
        for (; it != slot.end(); ++it)
        {
            auto [time, idx] = *it;
            if (time <= t)
            {
                slotIdx = idx;
                slotIt = it;

                if (it != slot.end() && slotIdx != INDEX_INVALID && objs[slotIdx].type == obj::Ty::VIDEO)
                {
                    auto pt = std::reinterpret_pointer_cast<TextureVideo>(objs[it->second].pt);
                    pt->start();
                    // pt->update();	// Do NOT call update here; videos are updated in main thread with
                    // TextureVideo::updateAll()
                }
            }
        }
    };

    std::unique_lock l(idxLock);
    seekSub(baseSlot, baseIdx, baseIt);
    seekSub(layerSlot, layerIdx, layerIt);
    seekSub(poorSlot, poorIdx, poorIt);
    inPoor = poor;
}

// LR2 scales bga with some weird rules:
// 1. position: horizontally centered, vertically top
// 2. w < 256 / h < 256: do not scale up
// 3. w > 256 / h > 256: stretch down to 256, ignore aspect
static void lr2ScaleBgaRect(const Rect& srcRect, RectF& dstRect)
{
    if (srcRect.w < 256)
    {
        dstRect.x = dstRect.x + dstRect.w * (1.0 - (double)srcRect.w / 256) / 2;
        dstRect.w = dstRect.w * ((double)srcRect.w / 256);
    }
    if (srcRect.h < 256)
    {
        dstRect.h = dstRect.h * ((double)srcRect.h / 256);
    }
}

void TextureBmsBga::draw(const Rect& sr, RectF dr, const Color c, const BlendMode b, const bool f, const double a,
                         const Point* ct) const
{
    auto draw = [&](const TextureBmsBga::obj& obj) {
        Rect srcRect = obj.pt ? obj.pt->getRect() : RECT_FULL;
        RectF dstRect = dr;
        lr2ScaleBgaRect(srcRect, dstRect);
        obj.pt->draw(srcRect, dstRect, c, b, f, a, ct);
    };

    std::shared_lock l(idxLock);

    if (inPoor && poorIdx != INDEX_INVALID)
    {
        if (auto& obj = objs.at(poorIdx); obj.type != obj::Ty::EMPTY)
        {
            draw(obj);
            return;
        }
    }

    if (baseIdx != INDEX_INVALID)
    {
        if (auto& obj = objs.at(baseIdx); obj.type != obj::Ty::EMPTY)
            draw(obj);
    }

    if (layerIdx != INDEX_INVALID)
    {
        if (auto& obj = objs.at(layerIdx); obj.type != obj::Ty::EMPTY)
        {
            if (obj.type == obj::Ty::PIC && objs_layer.at(layerIdx).pt != nullptr)
                draw(objs_layer.find(layerIdx)->second);
            else
                draw(obj);
        }
    }
}

void TextureBmsBga::reset()
{
    baseIt = baseSlot.begin();
    layerIt = layerSlot.begin();
    poorIt = poorSlot.begin();

    std::unique_lock l(idxLock);
    {
        baseIdx = INDEX_INVALID;
        layerIdx = INDEX_INVALID;
        poorIdx = INDEX_INVALID;
    }

    auto resetSub = [this](decltype(baseSlot)& slot) {
        for (auto [time, idx] : slot) // search from beginning
        {
            if (objs[idx].type == obj::Ty::VIDEO)
            {
                auto pt = std::reinterpret_pointer_cast<TextureVideo>(objs[idx].pt);
                pt->stopUpdate();
                pt->stop();
            }
        }
        // slotIt = slot.end();	// not found
    };

    resetSub(baseSlot);
    resetSub(layerSlot);
    resetSub(poorSlot);
}

void TextureBmsBga::clear()
{
    LVF_ASSERT(IsMainThread());
    loaded = false;
    textureRect = Rect();
    baseSlot.clear();
    layerSlot.clear();
    poorSlot.clear();
    objs_layer.clear();
    objs.clear();
    inPoor = false;
    reset();
}

void TextureBmsBga::setLoaded()
{
    loaded = true;
}

void TextureBmsBga::stopUpdate()
{
    auto resetSub = [this](decltype(baseSlot)& slot) {
        for (auto [time, idx] : slot) // search from beginning
        {
            if (objs[idx].type == obj::Ty::VIDEO)
            {
                auto pt = std::reinterpret_pointer_cast<TextureVideo>(objs[idx].pt);
                pt->stopUpdate();
            }
        }
        // slotIt = slot.end();	// not found
    };

    resetSub(baseSlot);
    resetSub(layerSlot);
    resetSub(poorSlot);
}

void TextureBmsBga::setVideoSpeed()
{
    auto setSpeed = [this](decltype(baseSlot)& slot) {
        for (auto [time, idx] : slot) // search from beginning
        {
            if (objs[idx].type == obj::Ty::VIDEO)
            {
                auto pt = std::reinterpret_pointer_cast<TextureVideo>(objs[idx].pt);
                pt->setSpeed(gSelectContext.pitchSpeed);
            }
        }
        // slotIt = slot.end();	// not found
    };

    setSpeed(baseSlot);
    setSpeed(layerSlot);
    setSpeed(poorSlot);
}
