#include <game/graphics/SDL2/graphics_SDL2.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string_view>

#include <SDL2_gfxPrimitives.h>

#include <common/assert.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/u8.h>
#include <common/utils.h>
#include <game/graphics/graphics.h>

#define SDL_LOAD_NOAUTOFREE 0
#define SDL_LOAD_AUTOFREE 1

Color::Color(uint32_t rgba)
{
    r = (rgba & 0xff000000) >> 24;
    g = (rgba & 0x00ff0000) >> 16;
    b = (rgba & 0x0000ff00) >> 8;
    a = (rgba & 0x000000ff);
}

Color::Color(int r, int g, int b, int a)
{
    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);
    a = std::clamp(a, 0, 255);
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

uint32_t Color::hex() const
{
    return r << 24 | g << 16 | b << 8 | a;
}

Color Color::operator+(const Color& rhs) const
{
    // always >=0
    Color c;
    c.r = (r + rhs.r <= 255) ? (r + rhs.r) : 255;
    c.g = (g + rhs.g <= 255) ? (g + rhs.g) : 255;
    c.b = (b + rhs.b <= 255) ? (b + rhs.b) : 255;
    c.a = (a + rhs.a <= 255) ? (a + rhs.a) : 255;
    return c;
}

Color Color::operator*(const double& rhs) const
{
    if (rhs < 0)
        return Color(0);
    Color c;
    c.r = (r * rhs <= 255) ? (Uint8)(r * rhs) : 255;
    c.g = (g * rhs <= 255) ? (Uint8)(g * rhs) : 255;
    c.b = (b * rhs <= 255) ? (Uint8)(b * rhs) : 255;
    c.a = (a * rhs <= 255) ? (Uint8)(a * rhs) : 255;
    return c;
}

Color Color::operator*(const Color& rhs) const
{
    if (hex() == 0xffffffff)
        return rhs;

    Color c;
    c.r = Uint8(r * (rhs.r / 255.0));
    c.g = Uint8(g * (rhs.g / 255.0));
    c.b = Uint8(b * (rhs.b / 255.0));
    c.a = Uint8(a * (rhs.a / 255.0));
    return c;
}

bool Color::operator==(const Color& rhs) const
{
    return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}
bool Color::operator!=(const Color& rhs) const
{
    return !(*this == rhs);
}

////////////////////////////////////////////////////////////////////////////////
// Image

static bool isTGA(const std::string_view filePath)
{
    if (filePath.length() < 4)
        return false;
    return lunaticvibes::iequals(filePath.substr(filePath.length() - 3), "tga");
}
static bool isPNG(const std::string_view filePath)
{
    if (filePath.length() < 4)
        return false;
    return lunaticvibes::iequals(filePath.substr(filePath.length() - 3), "png");
}
static bool isGIF(const std::string_view filePath)
{
    if (filePath.length() < 4)
        return false;
    return lunaticvibes::iequals(filePath.substr(filePath.length() - 3), "gif");
}

Image::Image(const std::filesystem::path& path) : Image(lunaticvibes::cs(path.u8string())) {}

// SDL2 leaks memory on non-main threads otherwise:
// SDL_GetErrBuf
// SDL_SetError_REAL
// SDL_RWFromFile_REAL
// SDL_RWFromFile
void ensure_tls_cleanup()
{
    static thread_local struct TlsCleanup
    {
        ~TlsCleanup() { SDL_TLSCleanup(); };
    } tls_cleanup;
}

void close_rwops(SDL_RWops* s)
{
    if (s)
        s->close(s);
    ensure_tls_cleanup();
}

Image::Image(const char* filePath)
    :

      Image(filePath, std::shared_ptr<SDL_RWops>(SDL_RWFromFile(filePath, "rb"), close_rwops))
{
}

Image::Image(const char* format, void* data, size_t size)
    : Image(format, std::shared_ptr<SDL_RWops>(SDL_RWFromMem(data, size), close_rwops))
{
}

Image::Image(const char* path, std::shared_ptr<SDL_RWops>&& rw) : _path(path), _pRWop(rw)
{
    if (!_pRWop && !_path.empty())
    {
        if (_path != "dummy")
        {
            LOG_WARNING << "[Image] Load image file error! " << SDL_GetError();
        }
        return;
    }
    if (_path.empty())
        return;
    LVF_DEBUG_ASSERT(_pRWop);
    const std::string_view pathView{path};
    if (isTGA(pathView))
    {
        _pSurface = std::shared_ptr<SDL_Surface>(IMG_LoadTGA_RW(_pRWop.get()), SDL_FreeSurface);
    }
    else if (isPNG(pathView))
    {
        _pSurface = std::shared_ptr<SDL_Surface>(IMG_LoadPNG_RW(_pRWop.get()), SDL_FreeSurface);
    }
    else if (isGIF(pathView))
    {
        _pSurface = std::shared_ptr<SDL_Surface>(IMG_LoadGIF_RW(_pRWop.get()), SDL_FreeSurface);
    }
    else
    {
        _pSurface = std::shared_ptr<SDL_Surface>(IMG_Load_RW(_pRWop.get(), SDL_LOAD_NOAUTOFREE), SDL_FreeSurface);
    }

    if (!_pSurface)
    {
        LOG_WARNING << "[Image] Build surface object error! " << IMG_GetError();
        return;
    }

    _haveAlphaLayer = !(_pSurface->format->Amask == 0 || isTGA(pathView));
    loaded = true;
    LOG_VERBOSE << "[Image] Load image file finished " << _path;
}

void Image::setTransparentColorRGB(Color c)
{
    if (_pSurface)
    {
        auto pSurfaceTmp = std::shared_ptr<SDL_Surface>(
            SDL_CreateRGBSurfaceWithFormat(0, _pSurface->w, _pSurface->h, 32, SDL_PIXELFORMAT_RGBA32), SDL_FreeSurface);
        SDL_SetSurfaceBlendMode(_pSurface.get(), SDL_BLENDMODE_NONE); // Required for SDL_SetSurfaceRLE.
        SDL_SetColorKey(&*_pSurface, SDL_TRUE, SDL_MapRGB(_pSurface->format, c.r, c.g, c.b));
        SDL_SetSurfaceRLE(_pSurface.get(), 1); // Improves blit performance 10x.
        SDL_Rect rc = _pSurface->clip_rect;
        SDL_BlitSurface(&*_pSurface, &rc, &*pSurfaceTmp, &rc);
        _pSurface = std::move(pSurfaceTmp);
    }
}

Rect Image::getRect() const
{
    return {0, 0, _pSurface->w, _pSurface->h};
}

////////////////////////////////////////////////////////////////////////////////
// Texture

Texture::Texture(const Image& srcImage)
{
    if (!srcImage.loaded)
        return;

    _texture = std::shared_ptr<SDL_Texture>(
        pushAndWaitMainThreadTask<SDL_Texture*>(
            std::bind_front(SDL_CreateTextureFromSurface, gFrameRenderer, &*srcImage._pSurface)),
        std::bind_front(pushAndWaitMainThreadTask<void, SDL_Texture*>, SDL_DestroyTexture));
    if (_texture)
    {
        textureRect = srcImage.getRect();
        loaded = true;
        LOG_VERBOSE << "[Texture] Build texture object finished. " << srcImage._path;
    }
    else
    {
        LOG_WARNING << "[Texture] Build texture object error! " << srcImage._path;
        LOG_WARNING << "[Texture] ^ " << SDL_GetError();
    }
}

Texture::Texture(const SDL_Surface* pSurface)
{
    _texture = std::shared_ptr<SDL_Texture>(
        pushAndWaitMainThreadTask<SDL_Texture*>(
            std::bind_front(SDL_CreateTextureFromSurface, gFrameRenderer, const_cast<SDL_Surface*>(pSurface))),
        std::bind_front(pushAndWaitMainThreadTask<void, SDL_Texture*>, SDL_DestroyTexture));
    if (!_texture)
        return;
    textureRect = pSurface->clip_rect;
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

static void do_draw(SDL_Texture* pTex, const Rect* srcRect, RectF dstRectF, const Color c, const BlendMode b,
                    const bool filter, const double angle, const Point* center)
{
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

    const SDL_ScaleMode mode = filter ? SDL_ScaleModeLinear : SDL_ScaleModeNearest;
    SDL_ScaleMode current_mode;
    SDL_GetTextureScaleMode(pTex, &current_mode);
    // SDL_SetTextureScaleMode is very expensive, while SDL_GetTextureScaleMode is practically free.
    if (mode != current_mode)
        SDL_SetTextureScaleMode(pTex, mode);

    int ssLevel = graphics_get_supersample_level();
    dstRectF.x *= ssLevel;
    dstRectF.y *= ssLevel;
    dstRectF.w *= ssLevel;
    dstRectF.h *= ssLevel;

    SDL_FPoint scenter;
    if (center)
        scenter = {(float)center->x * ssLevel, (float)center->y * ssLevel};

    if (b == BlendMode::INVERT)
    {
        // ... pls help
        const Rect rc = {0, 0, (int)std::ceil(dstRectF.w), (int)std::ceil(dstRectF.h)};

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

void Texture::draw(RectF dstRect, const Color c, const BlendMode b, const bool filter, const double angle) const
{
    do_draw(_texture.get(), nullptr, dstRect, c, b, filter, angle, nullptr);
}

void Texture::draw(RectF dstRect, const Color c, const BlendMode b, const bool filter, const double angle,
                   const Point& center) const
{
    do_draw(_texture.get(), nullptr, dstRect, c, b, filter, angle, &center);
}

void Texture::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode b, const bool filter,
                   const double angle) const
{
    Rect srcRectTmp(srcRect);
    if (srcRectTmp.w == RECT_FULL.w)
        srcRectTmp.w = textureRect.w;
    if (srcRectTmp.h == RECT_FULL.h)
        srcRectTmp.h = textureRect.h;
    do_draw(_texture.get(), &srcRectTmp, dstRect, c, b, filter, angle, nullptr);
}

void Texture::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode b, const bool filter,
                   const double angle, const Point& center) const
{
    Rect srcRectTmp(srcRect);
    if (srcRectTmp.w == RECT_FULL.w)
        srcRectTmp.w = textureRect.w;
    if (srcRectTmp.h == RECT_FULL.h)
        srcRectTmp.h = textureRect.h;
    do_draw(_texture.get(), &srcRectTmp, dstRect, c, b, filter, angle, &center);
}

////////////////////////////////////////////////////////////////////////////////
// TextureFull

TextureFull::TextureFull(const Color& c) : Texture(nullptr)
{
    auto surface = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 24, SDL_PIXELFORMAT_RGB24);
    textureRect = {0, 0, 1, 1};
    SDL_FillRect(&*surface, &textureRect, SDL_MapRGBA(surface->format, c.r, c.g, c.b, c.a));
    _texture = std::shared_ptr<SDL_Texture>(
        pushAndWaitMainThreadTask<SDL_Texture*>(std::bind_front(SDL_CreateTextureFromSurface, gFrameRenderer, surface)),
        std::bind_front(pushAndWaitMainThreadTask<void, SDL_Texture*>, SDL_DestroyTexture));
    loaded = true;
    SDL_FreeSurface(surface);
}

TextureFull::TextureFull(const Image& srcImage) : Texture(srcImage) {}

TextureFull::TextureFull(const SDL_Surface* pSurface) : Texture(pSurface) {}

TextureFull::TextureFull(SDL_Texture* pTexture, int w, int h) : Texture(pTexture, w, h) {}

TextureFull::~TextureFull() {}

void TextureFull::draw(const Rect& ignored, RectF dstRect, const Color c, const BlendMode b, const bool filter,
                       const double angle) const
{
    (void)ignored;
    do_draw(_texture.get(), nullptr, dstRect, c, b, filter, angle, nullptr);
}

void GraphLine::draw(Point p1, Point p2, Color c) const
{
    int ss = graphics_get_supersample_level();
    thickLineRGBA(gFrameRenderer, (Sint16)p1.x * ss, (Sint16)p1.y * ss, (Sint16)p2.x * ss, (Sint16)p2.y * ss,
                  _width * ss, c.r, c.g, c.b, c.a);
    SDL_SetRenderDrawColor(gFrameRenderer, 0, 0, 0, 255);
}
