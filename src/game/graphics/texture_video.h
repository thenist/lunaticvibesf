#include <game/graphics/graphics.h>

#include <cstdint>
#include <memory>
#include <shared_mutex>

using std::int64_t;

class sVideo;

class TextureVideo : public Texture
{
protected:
    std::shared_ptr<sVideo> pVideo;
    int64_t decoded_frames = ~0;
    PixelFormat format;
    bool updated = false;

public:
    TextureVideo(std::shared_ptr<sVideo> pv);
    ~TextureVideo() override;
    void start();
    void stop();
    void seek(int64_t sec);
    void setSpeed(double speed);
    void update();
    void reset();

    void draw(const Rect& srcRect, RectF dstRect, const Color c, const BlendMode blend, const bool filter,
              const double angleInDegrees, const Point* center) const override;

    void stopUpdate();

private:
    std::shared_ptr<std::shared_mutex> pTexMapMutex;               // ref counter guard
    std::shared_ptr<std::map<uintptr_t, TextureVideo*>> pTextures; // ref counter guard
public:
    static void updateAll();
};
