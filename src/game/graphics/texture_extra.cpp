#include "texture_extra.h"

#include <algorithm>
#include <future>
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
#include <game/graphics/video.h>
#include <game/scene/scene_context.h>

#include <boost/asio.hpp>

namespace r = std::ranges;

extern "C"
{
#include "libavformat/avformat.h"
}

TextureVideo::TextureVideo(std::shared_ptr<sVideo> pv_)
    : Texture(pv_->getW(), pv_->getH(), pv_->getFormat(), false), pVideo(std::move(pv_)), format(pVideo->getFormat())
{
    textureRect.x = 0;
    textureRect.y = 0;
    textureRect.w = pVideo->getW();
    textureRect.h = pVideo->getH();

    if (!texMapMutex)
        texMapMutex = std::make_shared<std::shared_mutex>();
    if (!textures)
        textures = std::make_shared<std::map<uintptr_t, TextureVideo*>>();

    pTexMapMutex = texMapMutex;
    pTextures = textures;

    std::unique_lock l(*texMapMutex);
    (*textures)[(uintptr_t)this] = this;
}

TextureVideo::~TextureVideo()
{
    if (pVideo->isPlaying())
        pVideo->stopPlaying();

    std::unique_lock l(*texMapMutex);
    textures->erase((uintptr_t)this);
}

void TextureVideo::start()
{
    if (!pVideo->isPlaying())
    {
        pVideo->startPlaying();
    }
}

void TextureVideo::stop()
{
    if (pVideo->isPlaying())
    {
        pVideo->stopPlaying();
    }
}

void TextureVideo::seek(int64_t sec)
{
    pVideo->seek(sec);
}

void TextureVideo::setSpeed(double speed)
{
    pVideo->setSpeed(speed);
}

void TextureVideo::update()
{
    if (!pVideo)
        return;
    if (!pVideo->haveVideo)
        return;
    if (!pVideo->isPlaying())
        return;

    auto vrfc = pVideo->getDecodedFrames();
    if (decoded_frames == vrfc)
        return;
    decoded_frames = vrfc;

    using namespace std::chrono_literals;

    std::shared_lock l(pVideo->video_frame_mutex, std::try_to_lock);
    if (l.owns_lock())
    {
        auto pf = pVideo->getFrame();
        if (!pf)
            return;

        switch (format)
        {
        case Texture::PixelFormat::IYUV:
            if (updateYUV(pf->data[0], pf->linesize[0], pf->data[1], pf->linesize[1], pf->data[2], pf->linesize[2]) ==
                0)
                updated = true;
            break;

        default: break;
        }
    }
}

void TextureVideo::stopUpdate()
{
    updated = false;
}

void TextureVideo::draw(RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                        const double angleInDegrees) const
{
    Texture::draw(dstRect, updated ? c : Color(0, 0, 0, c.a), blend, filter, angleInDegrees);
}
void TextureVideo::draw(RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                        const double angleInDegrees, const Point& center) const
{
    Texture::draw(dstRect, updated ? c : Color(0, 0, 0, c.a), blend, filter, angleInDegrees, center);
}
void TextureVideo::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                        const double angleInDegrees) const
{
    Texture::draw(srcRect, dstRect, updated ? c : Color(0, 0, 0, c.a), blend, filter, angleInDegrees);
}
void TextureVideo::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                        const double angleInDegrees, const Point& center) const
{
    Texture::draw(srcRect, dstRect, updated ? c : Color(0, 0, 0, c.a), blend, filter, angleInDegrees, center);
}

void TextureVideo::reset()
{
    seek(0);
    decoded_frames = 0;
}

std::shared_ptr<std::shared_mutex> TextureVideo::texMapMutex;
std::shared_ptr<std::map<uintptr_t, TextureVideo*>> TextureVideo::textures;

void TextureVideo::updateAll()
{
    if (texMapMutex)
    {
        std::shared_lock l(*texMapMutex);
        for (auto& t : *textures)
        {
            t.second->update();
        }
    }
}

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
            objs[idx].type = obj::Ty::PIC;
            objs[idx].pt = std::make_shared<Texture>(Image(pBmp));

            objs_layer[idx].type = obj::Ty::PIC;
            Image layerImg(pBmp);
            layerImg.setTransparentColorRGB(Color(0, 0, 0, 255));
            objs_layer[idx].pt = std::make_shared<Texture>(layerImg);
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

void TextureBmsBga::draw(const Rect& sr, RectF dr, const Color c, const BlendMode b, const bool f, const double a) const
{
    std::shared_lock l(idxLock);
    if (inPoor && poorIdx != INDEX_INVALID && objs.at(poorIdx).type != obj::Ty::EMPTY)
    {
        Rect srcRect = objs.at(poorIdx).pt ? objs.at(poorIdx).pt->getRect() : RECT_FULL;
        RectF dstRect = dr;
        lr2ScaleBgaRect(srcRect, dstRect);
        objs.at(poorIdx).pt->draw(srcRect, dstRect, c, b, f, a);
    }
    else
    {
        if (baseIdx != INDEX_INVALID && objs.at(baseIdx).type != obj::Ty::EMPTY)
        {
            Rect srcRect = objs.at(baseIdx).pt ? objs.at(baseIdx).pt->getRect() : RECT_FULL;
            RectF dstRect = dr;
            lr2ScaleBgaRect(srcRect, dstRect);
            objs.at(baseIdx).pt->draw(srcRect, dstRect, c, b, f, a);
        }

        if (layerIdx != INDEX_INVALID && objs.at(layerIdx).type != obj::Ty::EMPTY)
        {
            if (objs.at(layerIdx).type == obj::Ty::PIC && objs_layer.at(layerIdx).pt != nullptr)
            {
                Rect srcRect = objs_layer.at(layerIdx).pt ? objs_layer.at(layerIdx).pt->getRect() : RECT_FULL;
                RectF dstRect = dr;
                lr2ScaleBgaRect(srcRect, dstRect);
                objs_layer.at(layerIdx).pt->draw(srcRect, dstRect, c, b, f, a);
            }
            else
            {
                Rect srcRect = objs.at(layerIdx).pt ? objs.at(layerIdx).pt->getRect() : RECT_FULL;
                RectF dstRect = dr;
                lr2ScaleBgaRect(srcRect, dstRect);
                objs.at(layerIdx).pt->draw(srcRect, dstRect, c, b, f, a);
            }
        }
    }
}

void TextureBmsBga::draw(const Rect& sr, RectF dr, const Color c, const BlendMode b, const bool f, const double a,
                         const Point& ct) const
{
    std::shared_lock l(idxLock);
    if (inPoor && poorIdx != INDEX_INVALID && objs.at(poorIdx).type != obj::Ty::EMPTY)
    {
        Rect srcRect = objs.at(poorIdx).pt ? objs.at(poorIdx).pt->getRect() : RECT_FULL;
        RectF dstRect = dr;
        lr2ScaleBgaRect(srcRect, dstRect);
        objs.at(poorIdx).pt->draw(srcRect, dstRect, c, b, f, a, ct);
    }
    else
    {
        if (baseIdx != INDEX_INVALID && objs.at(baseIdx).type != obj::Ty::EMPTY)
        {
            Rect srcRect = objs.at(baseIdx).pt ? objs.at(baseIdx).pt->getRect() : RECT_FULL;
            RectF dstRect = dr;
            lr2ScaleBgaRect(srcRect, dstRect);
            objs.at(baseIdx).pt->draw(srcRect, dstRect, c, b, f, a, ct);
        }

        if (layerIdx != INDEX_INVALID && objs.at(layerIdx).type != obj::Ty::EMPTY)
        {
            if (objs.at(layerIdx).type == obj::Ty::PIC && objs_layer.at(layerIdx).pt != nullptr)
            {
                Rect srcRect = objs_layer.at(layerIdx).pt ? objs_layer.at(layerIdx).pt->getRect() : RECT_FULL;
                RectF dstRect = dr;
                lr2ScaleBgaRect(srcRect, dstRect);
                objs_layer.at(layerIdx).pt->draw(srcRect, dstRect, c, b, f, a, ct);
            }
            else
            {
                Rect srcRect = objs.at(layerIdx).pt ? objs.at(layerIdx).pt->getRect() : RECT_FULL;
                RectF dstRect = dr;
                lr2ScaleBgaRect(srcRect, dstRect);
                objs.at(layerIdx).pt->draw(srcRect, dstRect, c, b, f, a, ct);
            }
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

// TODO: remove this and use Texture directly.
// NOTE: TextureDynamic doesn't use any of Texture fields directly, instead passes draw() calls to another (member)
// texture. Public Texture methods may still use this one's members though.
TextureDynamic::TextureDynamic() : Texture(nullptr, 0, 0) {}

static std::future<Image> asyncLoadImage(Path path)
{
    auto [pool, mutex] = lunaticvibes::get_thread_pool();
    std::unique_lock l{mutex};
    // NOTE: std::async always spawns threads which seems to cause lots of lag.
    // For future improvement: right now it's possible to queue up some amount of these calls, which will all be
    // executed even if their result is discarded.
    return boost::asio::post(pool, boost::asio::use_future([path = std::move(path)]() { return Image{path}; }));
}

void TextureDynamic::setPath(const Path& path)
{
    LOG_VERBOSE << "[TextureDynamic] setPath " << _loaded_path << " -> " << path;
    if (path == _loaded_path)
        return;
    loaded = false;
    _loaded_path = path;
    if (path.empty())
        return;
    _loadImage = asyncLoadImage(path);
}

void TextureDynamic::applyImageIfNeeded()
{
    if (!_loadImage.valid() || _loadImage.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    Image image = _loadImage.get();
    _dynTexture = std::make_unique<Texture>(image);
    // For public Texture methods.
    textureRect = image.getRect();
    loaded = _dynTexture->isLoaded();
}

void TextureDynamic::draw(RectF dstRect, const Color c, const BlendMode b, const bool filter, const double angle) const
{
    _dynTexture->draw(dstRect, c, b, filter, angle);
}

void TextureDynamic::draw(RectF dstRect, const Color c, const BlendMode b, const bool filter, const double angle,
                          const Point& center) const
{
    _dynTexture->draw(dstRect, c, b, filter, angle, center);
}

void TextureDynamic::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode b, const bool filter,
                          const double angle) const
{
    _dynTexture->draw(srcRect, dstRect, c, b, filter, angle);
}

void TextureDynamic::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode b, const bool filter,
                          const double angle, const Point& center) const
{
    _dynTexture->draw(srcRect, dstRect, c, b, filter, angle, center);
}
