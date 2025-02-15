#pragma once

#include <common/types.h>
#include <game/graphics/color.h>

#include <string>

using TTF_Font = struct _TTF_Font; // NOLINT(cert-dcl37-c,cert-dcl51-cpp,bugprone-reserved-identifier): SDL detail

class Texture;

class TTFFont
{
private:
    TTF_Font* _pFont = nullptr;
    std::string _filePath;
    bool loaded = false;

public:
    TTFFont(const Path& filePath, int ptsize, int faceIndex);
    ~TTFFont();

    std::shared_ptr<Texture> TextUTF8(const char* text, const Color& c);
    [[nodiscard]] bool isLoaded() const { return loaded; }
};
