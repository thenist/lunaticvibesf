#pragma once
#include "sprite.h"
#include "sprite_imagetext.h"
#include <array>
#include <cstdint>
#include <memory>

using uint8_t = std::uint8_t;

// Exhaustive.
// Iterated on using underlying value.
enum class BarType : uint8_t
{
    SONG,
    NEW_SONG,
    FOLDER,
    CUSTOM_FOLDER,
    NEW_SONG_FOLDER,
    RIVAL,
    SONG_RIVAL,
    COURSE_FOLDER,
    NEW_COURSE,
    COURSE,
    RANDOM_COURSE,
    TYPE_COUNT
};

enum class BarLevelType : uint8_t
{
    UNDEF,    // WHITE
    BEGINNER, // GREEN
    NORMAL,   // BLUE
    HYPER,    // YELLOW
    ANOTHER,  // RED
    INSANE,   // PURPLE
    IRRANK,   // GREY
    LEVEL_TYPE_COUNT
};

enum class BarLampType : uint8_t
{
    NOPLAY,
    FAILED,
    EASY,
    NORMAL,
    HARD,
    // PATK_OR_GATK, // LR2Skin actually uses 5 as Fullcombo index
    FULLCOMBO,
    PERFECT,
    EXHARD,
    ASSIST_EASY,
    LAMP_TYPE_COUNT
};

enum class BarRankType : uint8_t
{
    NONE,
    F,
    E,
    D,
    C,
    B,
    A,
    AA,
    AAA,
    MAX,
    RANK_TYPE_COUNT
};

enum class BarRivalType : uint8_t
{
    WIN,
    LOSE,
    DRAW,
    NOPLAY,
    RIVAL_TYPE_COUNT
};

enum class BarTitleType : uint8_t
{
    NORMAL,
    NEW_SONG,
    FOLDER, // LVF extension.
    TITLE_TYPE_COUNT
};

class SkinLR2;

// Bar Entry sprite:
// select screen song bar. Have many sub-parts
// The parent-child chain is a bit odd, it looks like this:
//      BODY -> [sprite] -> TITLE/LEVEL/LAMP/RANK/etc.
class SpriteBarEntry : public SpriteBase, public iSpriteMouse
{
    using psAnimated = std::shared_ptr<SpriteAnimated>;
    using psNumber = std::shared_ptr<SpriteNumber>;
    using psOption = std::shared_ptr<SpriteOption>;
    using psText = std::shared_ptr<SpriteText>;

    friend class SkinLR2;

protected:
    size_t index = 0;
    bool available = false;
    std::array<psAnimated, static_cast<size_t>(BarType::TYPE_COUNT)> sBodyOff{nullptr};
    std::array<psAnimated, static_cast<size_t>(BarType::TYPE_COUNT)> sBodyOn{nullptr};
    std::array<psText, static_cast<size_t>(BarTitleType::TITLE_TYPE_COUNT)> sTitle{nullptr};
    psAnimated sFlash{nullptr};
    std::array<psNumber, static_cast<size_t>(BarLevelType::LEVEL_TYPE_COUNT)> sLevel{nullptr};
    std::array<psAnimated, static_cast<size_t>(BarLampType::LAMP_TYPE_COUNT)> sLamp{nullptr};
    std::array<psAnimated, static_cast<size_t>(BarRankType::RANK_TYPE_COUNT)> sRank{nullptr};
    std::array<psAnimated, static_cast<size_t>(BarRivalType::RIVAL_TYPE_COUNT)> sRivalWinLose{nullptr};
    std::array<psAnimated, static_cast<size_t>(BarLampType::LAMP_TYPE_COUNT)> sRivalLampSelf{nullptr};
    std::array<psAnimated, static_cast<size_t>(BarLampType::LAMP_TYPE_COUNT)> sRivalLampRival{nullptr};
    bool drawBodyOn = false;
    bool drawTitle = false;
    bool drawFlash = false;
    bool drawLevel = false;
    bool drawLamp = false;
    bool drawRank = false;
    bool drawRival = false;
    bool drawRivalLampSelf = false;
    bool drawRivalLampRival = false;
    size_t drawBodyType = 0;
    size_t drawTitleType = 0;
    size_t drawLevelType = 0;
    size_t drawLampType = 0;
    size_t drawRankType = 0;
    size_t drawRivalType = 0;
    size_t drawRivalLampSelfType = 0;
    size_t drawRivalLampRivalType = 0;

public:
    SpriteBarEntry(size_t idx) : SpriteBase(SpriteTypes::BAR_ENTRY, -1), index(idx) {}
    ~SpriteBarEntry() override = default;
    int setBody(BarType type, const SpriteAnimated::SpriteAnimatedBuilder& builder);
    int setFlash(const SpriteAnimated::SpriteAnimatedBuilder& builder);
    int setLevel(BarLevelType type, const SpriteNumber::SpriteNumberBuilder& builder);
    int setLamp(BarLampType type, const SpriteAnimated::SpriteAnimatedBuilder& builder);
    int setTitle(BarTitleType type, const SpriteText::SpriteTextBuilder& builder);
    int setTitle(BarTitleType type, const SpriteImageText::SpriteImageTextBuilder& builder);
    int setRank(BarRankType type, const SpriteAnimated::SpriteAnimatedBuilder& builder);
    int setRivalWinLose(BarRivalType type, const SpriteAnimated::SpriteAnimatedBuilder& builder);
    int setRivalLampSelf(BarLampType type, const SpriteAnimated::SpriteAnimatedBuilder& builder);
    int setRivalLampRival(BarLampType type, const SpriteAnimated::SpriteAnimatedBuilder& builder);

    void postProcess();

public:
    bool update(const lunaticvibes::Time& time) override;
    void setMotionLoopTo(int t) override;
    void setMotionStartTimer(IndexTimer t) override;
    void appendMotionKeyFrame(const MotionKeyFrame& f) override;
    void draw() const override;

public:
    auto getSpriteBodyOff(BarType type) { return sBodyOff[static_cast<size_t>(type)]; }
    auto getSpriteBodyOn(BarType type) { return sBodyOn[static_cast<size_t>(type)]; }
    auto getSpriteFlash() { return sFlash; }
    auto getSpriteTitle(BarTitleType type) { return sTitle[static_cast<size_t>(type)]; }
    auto getSpriteLevel(BarLevelType type) { return sLevel[static_cast<size_t>(type)]; }
    auto getSpriteLamp(BarLampType type) { return sLamp[static_cast<size_t>(type)]; }
    auto getSpriteRank(BarRankType type) { return sRank[static_cast<size_t>(type)]; }
    auto getSpriteRivalWinLose(BarRivalType type) { return sRivalWinLose[static_cast<size_t>(type)]; }
    auto getSpriteRivalLampSelf(BarLampType type) { return sRivalLampSelf[static_cast<size_t>(type)]; }
    auto getSpriteRivalLampRival(BarLampType type) { return sRivalLampRival[static_cast<size_t>(type)]; }

    void setRectOffsetAnim(float x, float y);
    void setRectOffsetBarIndex(float x, float y);

public:
    void setAvailable(bool c) { available = c; }
    void OnMouseMove(int x, int y) override {}
    bool OnClick(int x, int y) override;
    bool OnDrag(int x, int y) override { return false; }
};
