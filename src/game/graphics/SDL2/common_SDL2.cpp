#include <game/graphics/SDL2/graphics_SDL2.h>

#include <bit>
#include <cmath>
#include <memory>

#include <common/assert.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/u8.h>
#include <common/utils.h>
#include <game/graphics/graphics.h>

#include <SDL_filesystem.h>
#include <SDL_image.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <SDL_video.h>

////////////////////////////////////////////////////////////////////////////////
// Texture

Texture::Texture(const SDL_Surface* pSurface)
{
    _texture = std::shared_ptr<SDL_Texture>(
        pushAndWaitMainThreadTask<SDL_Texture*>(
            std::bind_front(SDL_CreateTextureFromSurface, gFrameRenderer, const_cast<SDL_Surface*>(pSurface))),
        std::bind_front(pushAndWaitMainThreadTask<void, SDL_Texture*>, SDL_DestroyTexture));
    if (!_texture)
        return;
    textureRect = std::bit_cast<Rect>(pSurface->clip_rect);
    loaded = true;
}

Texture::Texture(SDL_Texture* pTexture, int w, int h)
{
    _texture = std::shared_ptr<SDL_Texture>(
        pTexture, std::bind_front(pushAndWaitMainThreadTask<void, SDL_Texture*>, SDL_DestroyTexture));
    if (!pTexture)
        return;
    textureRect = {0, 0, w, h};
    loaded = true;
}

Texture::Texture(int w, int h, PixelFormat fmt, bool target)
{
    SDL_PixelFormatEnum sdlfmt = SDL_PIXELFORMAT_UNKNOWN;
    switch (fmt)
    {
    case PixelFormat::RGB24: sdlfmt = SDL_PIXELFORMAT_RGB24; break;
    case PixelFormat::BGR24: sdlfmt = SDL_PIXELFORMAT_BGR24; break;
    case PixelFormat::YV12: sdlfmt = SDL_PIXELFORMAT_YV12; break;
    case PixelFormat::IYUV: sdlfmt = SDL_PIXELFORMAT_IYUV; break;
    case PixelFormat::YUY2: sdlfmt = SDL_PIXELFORMAT_YUY2; break;
    case PixelFormat::UYVY: sdlfmt = SDL_PIXELFORMAT_UYVY; break;
    case PixelFormat::YVYU: sdlfmt = SDL_PIXELFORMAT_YVYU; break;
    case PixelFormat::UNKNOWN:
    case PixelFormat::UNSUPPORTED: break;
    }

    if (sdlfmt != SDL_PIXELFORMAT_UNKNOWN)
    {
        _texture = std::shared_ptr<SDL_Texture>(
            pushAndWaitMainThreadTask<SDL_Texture*>(
                std::bind_front(SDL_CreateTexture, gFrameRenderer, sdlfmt,
                                target ? SDL_TEXTUREACCESS_TARGET : SDL_TEXTUREACCESS_STREAMING, w, h)),
            std::bind_front(pushAndWaitMainThreadTask<void, SDL_Texture*>, SDL_DestroyTexture));
        if (_texture)
        {
            textureRect = {0, 0, w, h};
            loaded = true;
        }
    }
}

void* Texture::raw()
{
    return _texture.get();
}

int Texture::updateYUV(uint8_t* Y, int Ypitch, uint8_t* U, int Upitch, uint8_t* V, int Vpitch)
{
    LVF_DEBUG_ASSERT(IsMainThread());

    if (!loaded)
        return -1;
    if (!Ypitch || !Upitch || !Vpitch)
        return -2;
    if (_texture)
        SDL_UpdateYUVTexture(_texture.get(), nullptr, Y, Ypitch, U, Upitch, V, Vpitch);
    return 0;
}

static void do_draw(SDL_Texture* pTex, const Rect* srcRect_, const RectF dstRectF_, const Color c, const BlendMode b,
                    const double angle, const Point* center)
{
    const auto srcRect_val = srcRect_ ? std::bit_cast<SDL_Rect>(*srcRect_) : SDL_Rect{};
    const auto* srcRect = srcRect_ ? &srcRect_val : nullptr;
    auto dstRectF = std::bit_cast<SDL_FRect>(dstRectF_);
    int flipFlags = 0;
    if (dstRectF.w < 0)
    {
        dstRectF.w = -dstRectF.w;
        dstRectF.x -= dstRectF.w; /*flipFlags |= SDL_FLIP_HORIZONTAL;*/
    }
    if (dstRectF.h < 0)
    {
        dstRectF.h = -dstRectF.h;
        dstRectF.y -= dstRectF.h; /*flipFlags |= SDL_FLIP_VERTICAL;*/
    }

    SDL_SetTextureColorMod(pTex, c.r, c.g, c.b);

    int ssLevel = lunaticvibes::window::graphics_get_supersample_level();
    dstRectF.x *= ssLevel;
    dstRectF.y *= ssLevel;
    dstRectF.w *= ssLevel;
    dstRectF.h *= ssLevel;

    SDL_FPoint scenter;
    if (center)
        scenter = {static_cast<float>(center->x) * ssLevel, static_cast<float>(center->y) * ssLevel};

    if (b == BlendMode::INVERT)
    {
        // ... pls help
        const SDL_Rect rc = {0, 0, static_cast<int>(std::ceil(dstRectF.w)), static_cast<int>(std::ceil(dstRectF.h))};

        static auto pTextureInverted = std::shared_ptr<SDL_Texture>(
            SDL_CreateTexture(gFrameRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, rc.w, rc.h),
            std::bind_front(pushAndWaitMainThreadTask<void, SDL_Texture*>, SDL_DestroyTexture));

        auto oldTarget = SDL_GetRenderTarget(gFrameRenderer);
        SDL_SetRenderTarget(gFrameRenderer, &*pTextureInverted);

        uint8_t r, g, b, a;
        SDL_GetRenderDrawColor(gFrameRenderer, &r, &g, &b, &a);
        SDL_SetRenderDrawColor(gFrameRenderer, 255, 255, 255, 255);
        SDL_RenderFillRect(gFrameRenderer, &rc);
        SDL_SetRenderDrawColor(gFrameRenderer, r, g, b, a);

        static const auto blendMode = SDL_ComposeCustomBlendMode(
            SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO,
            SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
        SDL_SetTextureBlendMode(pTex, blendMode);
        SDL_SetTextureAlphaMod(pTex, c.a);
        SDL_RenderCopy(gFrameRenderer, pTex, srcRect, &rc);

        SDL_SetRenderTarget(gFrameRenderer, oldTarget);
        SDL_SetTextureBlendMode(&*pTextureInverted, SDL_BLENDMODE_BLEND);
        SDL_RenderCopyExF(gFrameRenderer, &*pTextureInverted, &rc, &dstRectF, angle, center ? &scenter : nullptr,
                          SDL_RendererFlip(flipFlags));
        return;
    }
    else if (b == BlendMode::MULTIPLY_INVERTED_BACKGROUND)
    {
        if (c.a <= 1)
            return; // do not draw
        // FIXME lmao

        static const SDL_BlendMode blendMode = SDL_ComposeCustomBlendMode(
            SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE,
            SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);

        SDL_SetTextureBlendMode(pTex, blendMode);
    }
    else
    {
        SDL_SetTextureAlphaMod(pTex, b == BlendMode::NONE ? 255 : c.a);

        auto to_sdl_blend_mode = [](const BlendMode b) -> SDL_BlendMode {
            switch (b)
            {
            case BlendMode::NONE: // Do not use SDL_BLENDMODE_NONE, set alpha=255 instead
            case BlendMode::ALPHA: return SDL_BLENDMODE_BLEND;
            case BlendMode::ADD: return SDL_BLENDMODE_ADD;
            case BlendMode::MOD: return SDL_BLENDMODE_MOD;
            case BlendMode::SUBTRACT: {
                static const auto mode = SDL_ComposeCustomBlendMode(
                    SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_REV_SUBTRACT,
                    SDL_BLENDFACTOR_ONE_MINUS_DST_ALPHA, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                return mode;
            }
            case BlendMode::INVERT:                                                     // unreachable
            case BlendMode::MULTIPLY_INVERTED_BACKGROUND: return SDL_BLENDMODE_INVALID; // unreachable
            case BlendMode::MULTIPLY_WITH_ALPHA: {
                // FIXME(rustbell): this is not correct.
                static const auto mode = SDL_ComposeCustomBlendMode(
                    SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
                    SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
                return mode;
            }
            }
            lunaticvibes::assert_failed("to_sdl_blend_mode");
        };

        if (const auto blend_mode = to_sdl_blend_mode(b); blend_mode != SDL_BLENDMODE_INVALID)
        {
            SDL_SetTextureBlendMode(pTex, blend_mode);
        }
    }

    SDL_RenderCopyExF(gFrameRenderer, pTex, srcRect, &dstRectF, angle, center ? &scenter : nullptr,
                      SDL_RendererFlip(flipFlags));

    // #ifndef NDEBUG
    //     SDL_FRect& d = dstRectF;
    //     SDL_FPoint lines[5] = { {d.x, d.y}, {d.x + d.w, d.y}, {d.x + d.w, d.y + d.h}, {d.x, d.y + d.h}, {d.x, d.y} };
    //     SDL_SetRenderDrawColor(gFrameRenderer, 255, 255, 255, 255);
    //     SDL_RenderDrawLinesF(gFrameRenderer, lines, 5);
    //     SDL_SetRenderDrawColor(gFrameRenderer, 0, 0, 0, 255);
    // #endif
}

void Texture::maybe_set_filtering(bool filter) const
{
    // NOTE: only the last call to SDL_SetTextureScaleMode within a rendering cycle applies.
    if (_filtering_set)
        return;
    _filtering_set = true;

    const SDL_ScaleMode mode = filter ? SDL_ScaleModeLinear : SDL_ScaleModeNearest;
    SDL_ScaleMode current_mode;
    SDL_GetTextureScaleMode(_texture.get(), &current_mode);
    // SDL_SetTextureScaleMode is very expensive, while SDL_GetTextureScaleMode is practically free.
    if (mode != current_mode)
        SDL_SetTextureScaleMode(_texture.get(), mode);
}

void Texture::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode b, const bool filter,
                   const double angle, const Point* center) const
{
    Rect srcRectTmp = srcRect;
    if (srcRectTmp.w == RECT_FULL.w)
        srcRectTmp.w = textureRect.w;
    if (srcRectTmp.h == RECT_FULL.h)
        srcRectTmp.h = textureRect.h;
    maybe_set_filtering(filter);
    do_draw(_texture.get(), &srcRectTmp, dstRect, c, b, angle, center);
}

////////////////////////////////////////////////////////////////////////////////
// TextureFull

struct SdlSurfaceDeleter
{
    void operator()(SDL_Surface* surface) { SDL_FreeSurface(surface); }
};

static std::unique_ptr<SDL_Surface, SdlSurfaceDeleter> make_full(const Color& c)
{
    auto surface = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 24, SDL_PIXELFORMAT_RGB24);
    const SDL_Rect textureRect = {0, 0, 1, 1};
    SDL_FillRect(&*surface, &textureRect, SDL_MapRGBA(surface->format, c.r, c.g, c.b, c.a));
    return std::unique_ptr<SDL_Surface, SdlSurfaceDeleter>{surface};
}

TextureFull::TextureFull(const Color& c) : Texture(make_full(c).get()) {}
TextureFull::~TextureFull() = default;
void TextureFull::draw(const Rect& ignored, RectF dstRect, const Color c, const BlendMode b, const bool filter,
                       const double angle, const Point* center) const
{
    (void)ignored;
    (void)filter;
    do_draw(_texture.get(), nullptr, dstRect, c, b, angle, center);
}
