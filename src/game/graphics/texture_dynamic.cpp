#include "texture_dynamic.h"

#include <common/log.h>
#include <common/thread_pool.h>
#include <game/graphics/graphics.h>

#include <boost/asio.hpp>

#include <filesystem>

// TODO: remove this and use Texture directly.
// NOTE: TextureDynamic doesn't use any of Texture fields directly, instead passes draw() calls to another (member)
// texture. Public Texture methods may still use this one's members though.
TextureDynamic::TextureDynamic() : Texture(nullptr, 0, 0) {}

static std::future<Image> asyncLoadImage(std::filesystem::path path)
{
    auto [pool, mutex] = lunaticvibes::get_thread_pool();
    std::unique_lock l{mutex};
    // NOTE: std::async always spawns threads which seems to cause lots of lag.
    // For future improvement: right now it's possible to queue up some amount of these calls, which will all be
    // executed even if their result is discarded.
    return boost::asio::post(pool, boost::asio::use_future([path = std::move(path)]() { return Image{path}; }));
}

void TextureDynamic::setPath(const Path& path)
{
    LOG_VERBOSE << "[TextureDynamic] setPath " << _loaded_path << " -> " << path;
    if (path == _loaded_path)
        return;
    loaded = false;
    _loaded_path = path;
    if (path.empty())
        return;
    _loadImage = asyncLoadImage(path);
}

void TextureDynamic::applyImageIfNeeded()
{
    if (!_loadImage.valid() || _loadImage.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    Image image = _loadImage.get();
    _dynTexture = std::make_unique<Texture>(image.build_texture());
    // For public Texture methods.
    textureRect = image.getRect();
    loaded = _dynTexture->isLoaded();
}

void TextureDynamic::draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode b, const bool filter,
                          const double angle, const Point* center) const
{
    _dynTexture->draw(srcRect, dstRect, c, b, filter, angle, center);
}
