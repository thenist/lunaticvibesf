#include <game/graphics/graphics.h>

#include <filesystem>
#include <future>
#include <memory>

class TextureDynamic final : public Texture
{
private:
    std::future<Image> _loadImage;
    std::unique_ptr<Texture> _dynTexture;
    std::filesystem::path _loaded_path;

public:
    TextureDynamic();
    ~TextureDynamic() override = default;

    void setPath(const std::filesystem::path& path); // Asynchronously load the image from disk.
    void applyImageIfNeeded();                       // Synchronously upload the image to GPU.

    void draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode blend, const bool filter,
              const double angleInDegrees, const Point* center) const override;
};
