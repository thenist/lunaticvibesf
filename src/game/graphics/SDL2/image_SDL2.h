#pragma once

#include <game/graphics/color.h>
#include <game/graphics/rect.h>

#include <filesystem>
#include <memory>
#include <string>

struct SDL_RWops;
struct SDL_Surface;

class Texture;

// Under the hood SDL_Image loads pictures into SDL_Surface instances.
class Image
{
private:
    std::string _path;
    std::shared_ptr<SDL_RWops> _pRWop;
    std::shared_ptr<SDL_Surface> _pSurface;
    bool loaded = false;
    bool _haveAlphaLayer = false;

private:
    Image(const char* path, std::shared_ptr<SDL_RWops>&& rw);

public:
    Image(const std::filesystem::path& path);
    Image(const char* filePath);
    void setTransparentColorRGB(Color c);
    [[nodiscard]] bool hasAlphaLayer() const { return _haveAlphaLayer; }

    [[nodiscard]] Rect getRect() const;
    [[nodiscard]] bool isLoaded() const { return loaded; }
    [[nodiscard]] const std::string& path() const { return _path; }

    [[nodiscard]] Texture build_texture() const;
};

namespace lunaticvibes
{

// Returns false on error
bool save_into_png(SDL_Surface* surface, const std::filesystem::path& path);

} // namespace lunaticvibes
