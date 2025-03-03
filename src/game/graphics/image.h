#pragma once

#include <game/graphics/color.h>
#include <game/graphics/rect.h>

#include <filesystem>
#include <string>

class Texture;

namespace lunaticvibes
{

class Image
{
private:
    class Impl;
    std::unique_ptr<Impl> _impl;

public:
    Image(const std::filesystem::path& path);
    Image(const char* filePath);

    Image(const Image&) = delete;
    Image(Image&&) noexcept;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) noexcept;
    ~Image();

    void setTransparentColorRGB(Color c);
    [[nodiscard]] bool hasAlphaLayer() const;

    [[nodiscard]] Rect getRect() const;
    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] const std::string& path() const;

    [[nodiscard]] Texture build_texture() const;
};

} // namespace lunaticvibes

using lunaticvibes::Image;
