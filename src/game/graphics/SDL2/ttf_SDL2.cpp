#include "ttf_SDL2.h"

#include <SDL_ttf.h>

#include <common/assert.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/u8.h>
#include <game/graphics/SDL2/graphics_SDL2.h>

#include <filesystem>
#include <functional>

struct TtfInitQuit
{
    TtfInitQuit()
    {
        LOG_INFO << "[TTF] Initializing SDL_ttf...";
        if (TTF_Init() == -1)
            panic("SDL_ttf init failed", TTF_GetError());
        LOG_INFO << "[TTF] Initialized SDL_ttf " << SDL_TTF_MAJOR_VERSION << '.' << SDL_TTF_MINOR_VERSION << "."
                 << SDL_TTF_PATCHLEVEL;
    }
    ~TtfInitQuit()
    {
        LOG_INFO << "[TTF] De-initializing SDL_ttf...";
        TTF_Quit();
    };
};

static void maybe_init_ttf_font()
{
    static TtfInitQuit _;
    (void)_;
}

TTFFont::TTFFont(const std::filesystem::path& filePath, int ptsize, int faceIndex)
    : _path(lunaticvibes::cs(filePath.u8string()))
{
    maybe_init_ttf_font();
    pushAndWaitMainThreadTask<void>([&]() { _data = TTF_OpenFontIndex(_path.c_str(), ptsize, faceIndex); });
    if (!_data)
    {
        LOG_WARNING << "[TTF] " << filePath << ": " << TTF_GetError();
    }
    else
    {
        _loaded = true;
    }
}

TTFFont::~TTFFont()
{
    if (!_loaded)
        return;
    pushAndWaitMainThreadTask<void>(std::bind_front(TTF_CloseFont, _data));
}

std::shared_ptr<Texture> TTFFont::build_texture(const char* text, const Color& c)
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!_loaded)
        return nullptr;

    SDL_Surface* surfaceText = TTF_RenderUTF8_Blended(_data, text, std::bit_cast<SDL_Color>(c));
    std::shared_ptr<Texture> pTexture = std::make_shared<Texture>(surfaceText);
    SDL_FreeSurface(surfaceText);
    return pTexture;
}
