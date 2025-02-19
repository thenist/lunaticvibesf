#include "skin.h"

#include <algorithm>
#include <execution>
#include <format>
#include <ranges>

#include <common/assert.h>
#include <common/beat.h>
#include <game/graphics/sprite_lane.h>
#include <game/graphics/sprite_video.h>
#include <game/scene/scene_context.h>
#include <game/skin/skin_lr2_debug.h>

// FIXME: get rid of preDefinedTextures.
std::map<std::string, std::shared_ptr<Texture>> SkinBase::preDefinedTextures;
std::map<std::string, std::shared_ptr<Texture>> SkinBase::textureNameMap;

namespace v = std::views;

SkinBase::SkinBase()
{
    _version = SkinVersion::UNDEF;
    if (preDefinedTextures.empty())
    {
        preDefinedTextures["Black"] = std::make_shared<TextureFull>(0x000000ff);
        preDefinedTextures["White"] = std::make_shared<TextureFull>(0xffffffff);
        preDefinedTextures["Error"] = std::make_shared<TextureFull>(0xff00ffff);
        preDefinedTextures["STAGEFILE"] = std::shared_ptr<Texture>(&gChartContext.texStagefile, [](Texture*) {});
        preDefinedTextures["BACKBMP"] = std::shared_ptr<Texture>(&gChartContext.texBackbmp, [](Texture*) {});
        preDefinedTextures["BANNER"] = std::shared_ptr<Texture>(&gChartContext.texBanner, [](Texture*) {});
        preDefinedTextures["THUMBNAIL"] = std::make_shared<Texture>(1920, 1080, Texture::PixelFormat::RGB24, true);
    }
    for (auto& [key, texture] : preDefinedTextures)
    {
        textureNameMap[key] = texture;
    }
}

SkinBase::~SkinBase()
{
    if (pSpriteTextEditing)
    {
        pSpriteTextEditing->stopEditing(false);
        pSpriteTextEditing = nullptr;
    }
}

void SkinBase::update(const lunaticvibes::Time& t)
{
    // current beat, measure
    if (gPlayContext.chartObj[PLAYER_SLOT_PLAYER] != nullptr)
    {
        const auto metre = gPlayContext.chartObj[PLAYER_SLOT_PLAYER]->getCurrentMetre();
        const auto bar = gPlayContext.chartObj[PLAYER_SLOT_PLAYER]->getCurrentBar();
        std::for_each(
            std::execution::par, _laneSprites.begin(), _laneSprites.end(),
            [metre, bar](const std::shared_ptr<SpriteLaneVertical>& s) { s->setCurrentMetreBar(metre, bar); });
    }

    std::for_each(std::execution::par, _sprites.begin(), _sprites.end(),
                  [t](const std::shared_ptr<SpriteBase>& s) { s->update(t); });
    for (auto& s : _sprites)
        s->update_on_main(t);
}

void SkinBase::update_mouse(int x, int y)
{
    if (!handleMouseEvents)
    {
        x = -99999999;
        y = -99999999; // LUL
    }

    auto clickSpriteLambda = [x, y](const std::shared_ptr<SpriteBase>& s) {
        if (s->isDraw() && !s->isHidden())
        {
            auto pS = std::dynamic_pointer_cast<iSpriteMouse>(s);
            LVF_DEBUG_ASSERT(pS != nullptr);
            pS->OnMouseMove(x, y);
        }
    };

    std::for_each(std::execution::par, _mouseSprites.begin(), _mouseSprites.end(), clickSpriteLambda);
}

void SkinBase::update_mouse_click(int x, int y)
{
    if (!handleMouseEvents)
        return;

    // sprite inserted last has priority
    pSpriteLastClicked = nullptr;

    if (lunaticvibes::g_enable_show_clicked_sprite)
    {
        for (const auto& s : v::reverse(_sprites))
        {
            if (s->type() != SpriteTypes::MOUSE_CURSOR && s->isDraw() && !s->isHidden())
            {
                const RectF& rc = s->_current.rect;
                if (x >= rc.x && y >= rc.y && x < rc.x + rc.w && y < rc.y + rc.h)
                {
                    createNotification(std::format("Clicked sprite ({},{})[{}x{}] (Line:{})", s->_current.rect.x,
                                                   s->_current.rect.y, s->_current.rect.w, s->_current.rect.h,
                                                   s->srcLine));
                    break;
                }
            }
        }
    }

    for (const auto& s : v::reverse(_mouseSprites))
    {
        if (s->isDraw() && !s->isHidden())
        {
            auto pS = std::dynamic_pointer_cast<iSpriteMouse>(s);
            LVF_DEBUG_ASSERT(pS != nullptr);
            if (pS->OnClick(x, y))
            {
                if (auto s_as_text = std::dynamic_pointer_cast<SpriteText>(s))
                {
                    if (pSpriteTextEditing)
                        pSpriteTextEditing->stopEditing(false);
                    pSpriteTextEditing = s_as_text;
                }
                pSpriteDragging = pS;
                pSpriteLastClicked = pS;
                break;
            }
        }
    }
}

void SkinBase::update_mouse_drag(int x, int y)
{
    if (!handleMouseEvents)
        return;

    if (pSpriteDragging != nullptr)
        pSpriteDragging->OnDrag(x, y);
}

void SkinBase::update_mouse_release()
{
    pSpriteDragging = nullptr;
}

void SkinBase::draw() const
{
    for (auto& s : _sprites)
        s->draw();
}

void SkinBase::startSpriteVideoPlayback()
{
    for (auto& p : _sprites)
    {
        if (p->type() == SpriteTypes::VIDEO)
        {
            auto v = std::reinterpret_pointer_cast<SpriteVideo>(p);
            v->startPlaying();
        }
    }
}

void SkinBase::stopSpriteVideoPlayback()
{
    for (auto& p : _sprites)
    {
        if (p->type() == SpriteTypes::VIDEO)
        {
            auto v = std::reinterpret_pointer_cast<SpriteVideo>(p);
            v->stopPlaying();
        }
    }
}

bool SkinBase::textEditSpriteClicked() const
{
    return pSpriteTextEditing != nullptr && pSpriteTextEditing == pSpriteLastClicked;
}

IndexText SkinBase::textEditType() const
{
    return pSpriteTextEditing ? pSpriteTextEditing->getInd() : IndexText::INVALID;
}

void SkinBase::startTextEdit(bool clear)
{
    if (pSpriteTextEditing)
        pSpriteTextEditing->startEditing(clear);
}

void SkinBase::stopTextEdit(bool modify)
{
    if (pSpriteTextEditing)
    {
        pSpriteTextEditing->stopEditing(modify);
        pSpriteTextEditing = nullptr;
    }
}

std::shared_ptr<Texture> SkinBase::getTextureCustomizeThumbnail()
{
    return textureNameMap["THUMBNAIL"];
}

void SkinBase::setThumbnailTextureSize(int w, int h)
{
    *textureNameMap["THUMBNAIL"] = Texture{w, h, Texture::PixelFormat::RGB24, true};
}
