#pragma once

#include <game/graphics/color.h>

#include <filesystem>
#include <string>

using TTF_Font = struct _TTF_Font; // NOLINT(cert-dcl37-c,cert-dcl51-cpp,bugprone-reserved-identifier): SDL detail

class Texture;

class TTFFont
{
private:
    std::string _path;
    TTF_Font* _data = nullptr;
    bool _loaded = false;

public:
    TTFFont(const std::filesystem::path& filePath, int ptsize, int faceIndex);
    ~TTFFont();

    [[nodiscard]] std::shared_ptr<Texture> build_texture(const char* text, const Color& c);
    [[nodiscard]] bool isLoaded() const { return _loaded; }
};
