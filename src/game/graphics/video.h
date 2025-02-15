#pragma once

#include <future>
#include <memory>
#include <shared_mutex>

#include <common/types.h>
#include <game/graphics/graphics.h>

extern "C"
{
    struct AVFormatContext;
    struct AVCodec;
    struct AVCodecContext;
    struct AVFrame;
    struct AVPacket;
}
class SceneBase;
class SkinBase;

namespace lunaticvibes
{
[[nodiscard]] bool is_video_file_path(const Path& p);

struct AVCodecContextDeleter
{
    void operator()(AVCodecContext*);
};
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
struct AVFormatContextDeleter
{
    void operator()(AVFormatContext*);
};
using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
struct AVFrameDeleter
{
    void operator()(AVFrame*);
};
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
struct AVPacketDeleter
{
    void operator()(AVPacket*);
};
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
} // namespace lunaticvibes

// libav decoder wrap
class sVideo
{
    friend class SceneBase;
    friend class SkinBase;

public:
    Path file;
    bool haveVideo = false;

private:
    // decoder params
    lunaticvibes::AVFormatContextPtr pFormatCtx;
    const AVCodec* pCodec = nullptr;
    lunaticvibes::AVCodecContextPtr pCodecCtx;
    lunaticvibes::AVFramePtr pFrame;
    lunaticvibes::AVPacketPtr pPacket;
    int videoIndex = -1;
    int64_t decoded_frames = 0;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::future<void> decodeEnd;

    // render properties
    Path filePath;
    double speed = 1.0;
    int w = -1, h = -1; // set in setVideo()
    bool playing = false;
    bool finished = false;
    bool decoding = false;
    bool firstFrame = true;
    bool valid = false;
    bool loop_playback = false;

public:
    sVideo() = default;
    sVideo(const Path& file, double speed = 1.0, bool loop = false) { setVideo(file, speed, loop); }
    virtual ~sVideo();
    int setVideo(const Path& file, double speed, bool loop = false);
    int unsetVideo();
    [[nodiscard]] int getW() const { return w; }
    [[nodiscard]] int getH() const { return h; }
    [[nodiscard]] bool isPlaying() const { return playing; }

public:
    // properties
    Texture::PixelFormat getFormat();

public:
    // video playback control
    void startPlaying();
    void stopPlaying();
    void decodeLoop();

    [[nodiscard]] int64_t getDecodedFrames() const { return decoded_frames; }
    [[nodiscard]] AVFrame* getFrame() const { return valid ? pFrame.get() : nullptr; }

public:
    std::shared_mutex video_frame_mutex;

public:
    void setSpeed(double speed) { this->speed = speed; }
    void seek(int64_t second, bool backwards = false);
};
