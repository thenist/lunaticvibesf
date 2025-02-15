#include "graphics_SDL2.h"

#include <common/assert.h>
#include <common/log.h>
#include <common/u8.h>
#include <common/utils.h>
#include <game/graphics/color.h>
#include <game/graphics/rect.h>

#include <filesystem>
#include <memory>
#include <string_view>

#include <SDL.h>
#include <SDL_image.h>

static bool is_file_extension(const std::string_view filePath, const std::string_view extension)
{
    if (filePath.length() < extension.length() + 1)
        return false;
    return lunaticvibes::iequals(filePath.substr(filePath.length() - extension.length()), extension);
}

// SDL2 leaks memory on non-main threads otherwise:
// SDL_GetErrBuf
// SDL_SetError_REAL
// SDL_RWFromFile_REAL
// SDL_RWFromFile
static void ensure_tls_cleanup()
{
    static thread_local struct TlsCleanup
    {
        ~TlsCleanup() { SDL_TLSCleanup(); };
    } tls_cleanup;
}

static void close_rwops(SDL_RWops* s)
{
    if (s)
        s->close(s);
    ensure_tls_cleanup();
}

Image::Image(const std::filesystem::path& path) : Image(lunaticvibes::cs(path.u8string())) {}

Image::Image(const char* filePath)
    : Image(filePath, std::shared_ptr<SDL_RWops>(SDL_RWFromFile(filePath, "rb"), close_rwops))
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
    static constexpr int LEAVE_RWOP_OPEN = 0;
    if (is_file_extension(pathView, "tga"))
        _pSurface = std::shared_ptr<SDL_Surface>(IMG_LoadTGA_RW(_pRWop.get()), SDL_FreeSurface);
    else if (is_file_extension(pathView, "png"))
        _pSurface = std::shared_ptr<SDL_Surface>(IMG_LoadPNG_RW(_pRWop.get()), SDL_FreeSurface);
    else if (is_file_extension(pathView, "gif"))
        _pSurface = std::shared_ptr<SDL_Surface>(IMG_LoadGIF_RW(_pRWop.get()), SDL_FreeSurface);
    else
        _pSurface = std::shared_ptr<SDL_Surface>(IMG_Load_RW(_pRWop.get(), LEAVE_RWOP_OPEN), SDL_FreeSurface);

    if (!_pSurface)
    {
        LOG_WARNING << "[Image] Build surface object error! " << IMG_GetError();
        return;
    }

    _haveAlphaLayer = !(_pSurface->format->Amask == 0 || is_file_extension(pathView, "tga"));
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
    if (_pSurface == nullptr)
        return {};
    return {0, 0, _pSurface->w, _pSurface->h};
}
