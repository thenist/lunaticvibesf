#include "ttf_SDL2.h"

#include <SDL_ttf.h>

#include <common/assert.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/types.h>
#include <common/u8.h>
#include <game/graphics/SDL2/graphics_SDL2.h>

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

TTFFont::TTFFont(const Path& filePath, int ptsize, int faceIndex) : _filePath(lunaticvibes::cs(filePath.u8string()))
{
    maybe_init_ttf_font();
    pushAndWaitMainThreadTask<void>([&]() { _pFont = TTF_OpenFontIndex(_filePath.c_str(), ptsize, faceIndex); });
    if (!_pFont)
    {
        LOG_WARNING << "[TTF] " << filePath << ": " << TTF_GetError();
    }
    else
    {
        loaded = true;
    }
}

TTFFont::~TTFFont()
{
    if (!loaded)
        return;
    pushAndWaitMainThreadTask<void>(std::bind_front(TTF_CloseFont, _pFont));
}

std::shared_ptr<Texture> TTFFont::TextUTF8(const char* text, const Color& c)
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!loaded)
        return nullptr;

    SDL_Surface* surfaceText = TTF_RenderUTF8_Blended(_pFont, text, std::bit_cast<SDL_Color>(c));
    std::shared_ptr<Texture> pTexture = std::make_shared<Texture>(surfaceText);
    SDL_FreeSurface(surfaceText);
    return pTexture;
}
