#include "sprite_video.h"

#include <common/utils.h>
#include <game/graphics/texture_video.h>
#include <game/graphics/video.h>

extern "C"
{
#include <libavutil/frame.h>
}

SpriteVideo::SpriteVideo(int w, int h, const std::shared_ptr<sVideo>& pVid, int srcLine)
    : SpriteStatic(std::make_shared<TextureVideo>(pVid), {0, 0, w, h}, srcLine)
{
    _type = SpriteTypes::VIDEO;
}

void SpriteVideo::startPlaying()
{
    auto pVid = std::reinterpret_pointer_cast<TextureVideo>(pTexture);
    if (!pVid)
        return;
    pVid->start();
}

void SpriteVideo::stopPlaying()
{
    auto pVid = std::reinterpret_pointer_cast<TextureVideo>(pTexture);
    if (!pVid)
        return;
    pVid->stop();
}
