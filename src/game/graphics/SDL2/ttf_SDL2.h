#pragma once

#include <game/graphics/color.h>

#include <filesystem>
#include <string>

class Texture;

namespace lunaticvibes
{

class Font
{
private:
    std::string _path;
    void* _data = nullptr; // Implementation-specific.
    bool _loaded = false;

public:
    Font(const std::filesystem::path& filePath, int ptsize, int faceIndex);
    ~Font();

    [[nodiscard]] std::shared_ptr<Texture> build_texture(const char* text, const Color& c);
    [[nodiscard]] bool isLoaded() const { return _loaded; }
};

} // namespace lunaticvibes

using TTFFont = lunaticvibes::Font;
