#pragma once

#include "sprite.h"

#include <game/graphics/graphics.h>

#include <memory>

class sVideo;
struct AVFrame;

class SpriteVideo : public SpriteStatic
{
public:
    SpriteVideo(int w, int h, const std::shared_ptr<sVideo>& pVid, int srcLine = -1);
    ~SpriteVideo() override = default;

public:
    void startPlaying();
    void stopPlaying();
};
