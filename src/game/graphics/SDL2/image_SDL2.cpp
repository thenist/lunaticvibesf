#include "game/graphics/SDL2/image_SDL2.h"

#include <common/assert.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/u8.h>
#include <common/utils.h>
#include <game/graphics/SDL2/graphics_SDL2.h>
#include <game/graphics/color.h>
#include <game/graphics/image.h>
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

struct ImgInitQuit
{
    ImgInitQuit()
    {
        LOG_INFO << "[Image] Initializing SDL_image...";
        constexpr auto flags = IMG_INIT_JPG | IMG_INIT_PNG;
        if (IMG_Init(flags) != flags)
            panic("SDL_image init failed", IMG_GetError());
        LOG_INFO << "[Image] Initialized SDL_Image " << SDL_IMAGE_MAJOR_VERSION << '.' << SDL_IMAGE_MINOR_VERSION << "."
                 << SDL_IMAGE_PATCHLEVEL;
    }

    ~ImgInitQuit()
    {
        LOG_INFO << "[Image] De-initializing SDL_image...";
        IMG_Quit();
    };
};

static void maybe_init_image()
{
    static ImgInitQuit _;
    (void)_;
}

namespace lunaticvibes
{

class Image::Impl
{
private:
    std::string _path;
    std::shared_ptr<SDL_RWops> _pRWop;
    std::shared_ptr<SDL_Surface> _pSurface;
    bool loaded = false;
    bool _haveAlphaLayer = false;

private:
    Impl(const char* path, std::shared_ptr<SDL_RWops>&& rw);

public:
    Impl(const std::filesystem::path& path);
    Impl(const char* filePath);
    void setTransparentColorRGB(Color c);
    [[nodiscard]] bool hasAlphaLayer() const { return _haveAlphaLayer; }

    [[nodiscard]] Rect getRect() const;
    [[nodiscard]] bool isLoaded() const { return loaded; }
    [[nodiscard]] const std::string& path() const { return _path; }

    [[nodiscard]] Texture build_texture() const;
};

Image::Impl::Impl(const std::filesystem::path& path) : Image::Impl(lunaticvibes::cs(path.u8string())) {}

Image::Impl::Impl(const char* filePath)
    : Image::Impl(filePath, std::shared_ptr<SDL_RWops>(SDL_RWFromFile(filePath, "rb"), close_rwops))
{
}

Image::Impl::Impl(const char* path, std::shared_ptr<SDL_RWops>&& rw) : _path(path), _pRWop(rw)
{
    if (!_pRWop && !_path.empty())
    {
        if (_path != "dummy")
        {
            LOG_WARNING << "[Image::Impl] Load image file error! " << SDL_GetError();
        }
        return;
    }
    if (_path.empty())
        return;
    LVF_ASSERT(_pRWop);

    maybe_init_image();

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
        LOG_WARNING << "[Image::Impl] Build surface object error! " << IMG_GetError();
        return;
    }

    _haveAlphaLayer = !(_pSurface->format->Amask == 0 || is_file_extension(pathView, "tga"));
    loaded = true;
    LOG_VERBOSE << "[Image::Impl] Load image file finished " << _path;
}

void Image::Impl::setTransparentColorRGB(Color c)
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

Rect Image::Impl::getRect() const
{
    if (_pSurface == nullptr)
        return {};
    return {0, 0, _pSurface->w, _pSurface->h};
}

Texture Image::Impl::build_texture() const
{
    return Texture{_pSurface.get()};
}

Image::Image(Image&&) noexcept = default;
Image& Image::operator=(Image&&) noexcept = default;
Image::~Image() = default;

Image::Image(const std::filesystem::path& path) : _impl(std::make_unique<Image::Impl>(path)) {}
Image::Image(const char* filePath) : _impl(std::make_unique<Image::Impl>(filePath)) {}
void Image::setTransparentColorRGB(Color c)
{
    _impl->setTransparentColorRGB(c);
}
bool Image::hasAlphaLayer() const
{
    return _impl->hasAlphaLayer();
}
Rect Image::getRect() const
{
    return _impl->getRect();
}
bool Image::isLoaded() const
{
    return _impl->isLoaded();
}
const std::string& Image::path() const
{
    return _impl->path();
}
Texture Image::build_texture() const
{
    return _impl->build_texture();
};

} // namespace lunaticvibes

bool lunaticvibes::save_into_png(SDL_Surface* surface, const std::filesystem::path& path)
{
    maybe_init_image();
    int ret = IMG_SavePNG(surface, lunaticvibes::cs(path.u8string()));
    if (ret < 0)
    {
        LOG_ERROR << "[SDL2] IMG_SavePNG error: " << IMG_GetError();
        return false;
    }
    return true;
}
