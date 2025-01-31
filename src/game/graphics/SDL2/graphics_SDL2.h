#pragma once

#include <SDL_filesystem.h>
#include <SDL_image.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <SDL_video.h>

#include <filesystem>
#include <memory>
#include <string>

#include <common/types.h>
#include <game/graphics/blend_mode.h> // IWYU pragma: export
#include <game/graphics/color.h>      // IWUY pragma: export
#include <game/graphics/point.h>      // IWUY pragma: export
#include <game/graphics/rect.h>       // IWUY pragma: export
#include <game/graphics/rectf.h>      // IWUY pragma: export

// global control pointer, do not modify
inline SDL_Renderer* gFrameRenderer;
inline SDL_Texture* gInternalRenderTarget;

////////////////////////////////////////////////////////////////////////////////
// SDL_Image loads pictures into SDL_Surface instances
// Run IMG_Init outside.
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
    [[nodiscard]] std::shared_ptr<SDL_Surface> surface() const { return _pSurface; }
};

////////////////////////////////////////////////////////////////////////////////
// Convert SDL_Surface into SDL_Texture with subarea specified.
class Texture
{
protected:
    std::shared_ptr<SDL_Texture> _texture;
    bool loaded = false;
    Rect textureRect;

public:
    // Inner draw function.
    virtual void draw(RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                      const double angleInDegrees) const;
    virtual void draw(RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                      const double angleInDegrees, const Point& center) const;
    virtual void draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                      const double angleInDegrees) const;
    virtual void draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode blend, const bool filter,
                      const double angleInDegrees, const Point& center) const;

public:
    enum class PixelFormat
    {
        UNKNOWN,
        UNSUPPORTED,

        RGB24,
        BGR24,

        YV12,        // 4:2:0 Y + V + U
        IYUV,        // 4:2:0 Y + U + V
        I420 = IYUV, // 4:2:0 Y + U + V
        YUY2,        // Y0 + U0 + Y1 + V0
        YUYV = YUY2, // Y0 + U0 + Y1 + V0
        UYVY,        // U0 + Y0 + V0 + Y1
        YVYU,        // Y0 + V0 + Y1 + U0
    };

public:
    Texture(const Image& srcImage);
    Texture(const SDL_Surface* pSurface);
    Texture(SDL_Texture* pTexture, int w, int h);
    Texture(int w, int h, PixelFormat fmt, bool target);
    virtual ~Texture() = default;

public:
    void* raw();
    [[nodiscard]] Rect getRect() const { return textureRect; }
    [[nodiscard]] bool isLoaded() const { return loaded; }
    int updateYUV(uint8_t* Y, int Ypitch, uint8_t* U, int Upitch, uint8_t* V, int Vpitch);
};

// Special texture class that always uses full texture size as output rect.
// That is, srcRect is ignored and replaced with textureRects.
// Useful when rendering BGs and Error-texture.
class TextureFull : public Texture
{
private:
    void draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode blend, const bool filter,
              const double angleInDegrees) const override;

public:
    TextureFull(const Color& srcColor);
    ~TextureFull() override;
};

////////////////////////////////////////////////////////////////////////////////
// SDL_ttf encapsulation. Mostly as same as Image
// Run TTF_Init outside.
class TTFFont
{
private:
    TTF_Font* _pFont = nullptr;
    std::string _filePath;
    bool loaded = false;

public:
    TTFFont(const Path& filePath, int ptsize);
    TTFFont(const Path& filePath, int ptsize, int faceIndex);
    ~TTFFont();

    std::shared_ptr<Texture> TextUTF8(const char* text, const Color& c);
    [[nodiscard]] bool isLoaded() const { return loaded; }
};
