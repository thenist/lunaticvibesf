#include "texture_video.h"

#include <game/graphics/video.h>
#include <mutex>

extern "C"
{
#include "libavformat/avformat.h"
}

static std::shared_ptr<std::shared_mutex> s_tex_map_mutex;
static std::shared_ptr<std::map<uintptr_t, TextureVideo*>> s_textures;

TextureVideo::TextureVideo(std::shared_ptr<sVideo> pv_)
    : Texture(pv_->getW(), pv_->getH(), pv_->getFormat(), false), pVideo(std::move(pv_)), format(pVideo->getFormat())
{
    textureRect.x = 0;
    textureRect.y = 0;
    textureRect.w = pVideo->getW();
    textureRect.h = pVideo->getH();

    if (!s_tex_map_mutex)
        s_tex_map_mutex = std::make_shared<std::shared_mutex>();
    if (!s_textures)
        s_textures = std::make_shared<std::map<uintptr_t, TextureVideo*>>();

    pTexMapMutex = s_tex_map_mutex;
    pTextures = s_textures;

    std::unique_lock l(*s_tex_map_mutex);
    (*s_textures)[(uintptr_t)this] = this;
}

TextureVideo::~TextureVideo()
{
    if (pVideo->isPlaying())
        pVideo->stopPlaying();

    std::unique_lock l(*s_tex_map_mutex);
    s_textures->erase((uintptr_t)this);
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

void TextureVideo::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                        const double angleInDegrees, const Point* center) const
{
    Texture::draw(srcRect, dstRect, updated ? c : Color(0, 0, 0, c.a), blend, filter, angleInDegrees, center);
}

void TextureVideo::reset()
{
    seek(0);
    decoded_frames = 0;
}

void TextureVideo::updateAll()
{
    if (s_tex_map_mutex)
    {
        std::shared_lock l(*s_tex_map_mutex);
        for (auto& t : *s_textures)
        {
            t.second->update();
        }
    }
}
