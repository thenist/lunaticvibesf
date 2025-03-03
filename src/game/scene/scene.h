#pragma once

#include <common/types.h>
#include <game/input/input_wrapper.h>
#include <game/runtime/index/switch.h>
#include <game/runtime/index/timer.h>
#include <game/skin/skin.h>

#include <array>
#include <memory>

class SpriteStatic;
class SkinMgr;

enum class SceneType : uint8_t
{
    NOT_INIT,
    PRE_SELECT,
    SELECT,
    DECIDE,
    PLAY,
    RESULT,
    COURSE_TRANS,
    RETRY_TRANS,
    KEYCONFIG,
    CUSTOMIZE,
    COURSE_RESULT,
    EXIT_TRANS,
    EXIT
};

inline SceneType getSceneFromSkinType(SkinType m)
{
    switch (m)
    {
    case SkinType::MUSIC_SELECT: return SceneType::SELECT;
    case SkinType::DECIDE: return SceneType::DECIDE;
    case SkinType::THEME_SELECT: return SceneType::CUSTOMIZE;
    case SkinType::KEY_CONFIG: return SceneType::KEYCONFIG;
    case SkinType::PLAY5:
    case SkinType::PLAY5_2:
    case SkinType::PLAY7:
    case SkinType::PLAY7_2:
    case SkinType::PLAY9:
    case SkinType::PLAY9_2:
    case SkinType::PLAY10:
    case SkinType::PLAY14: return SceneType::PLAY;
    case SkinType::RESULT: return SceneType::RESULT;
    case SkinType::COURSE_RESULT: return SceneType::COURSE_RESULT;
    default: return SceneType::NOT_INIT;
    }
}

namespace lunaticvibes
{
class Font;
// Update global state to next course stage.
[[nodiscard]] SceneType advanceCourseStage(const SceneType exitScene, const SceneType playScene);
} // namespace lunaticvibes

// Parent class of scenes, defines how an object being stored and drawn.
// Every classes of scenes should inherit this class.
class SceneBase
{
protected:
    SceneType _type;
    std::shared_ptr<SkinBase> pSkin;
    InputWrapper _input;
    bool inTextEdit = false;
    unsigned _rate;

    std::shared_ptr<lunaticvibes::Font> _fNotifications;
    std::shared_ptr<Texture> _texNotificationsBG;
    std::array<std::shared_ptr<SpriteText>,
               size_t(IndexText::_OVERLAY_NOTIFICATION_MAX) - size_t(IndexText::_OVERLAY_NOTIFICATION_0) + 1>
        _sNotifications;
    std::array<std::shared_ptr<SpriteStatic>,
               size_t(IndexText::_OVERLAY_NOTIFICATION_MAX) - size_t(IndexText::_OVERLAY_NOTIFICATION_0) + 1>
        _sNotificationsBG;

protected:
    static bool queuedScreenshot;
    static bool queuedFPS;

    static bool showFPS;

public:
    SceneBase() = delete;
    SceneBase(const std::shared_ptr<SkinMgr>& skinMgr, SkinType skinType, unsigned rate = 240,
              bool backgroundInput = false);
    virtual ~SceneBase();
    void inputLoopStart() { _input.loopStart(); }
    void inputLoopEnd() { _input.loopEnd(); }
    void processInput() { _input.processInput(); };
    void disableMouseInput() { pSkin->setHandleMouseEvents(false); }
    [[nodiscard]] unsigned getRate() const { return _rate; };

public:
    virtual void update();                                     // Called once before rendering, includes skin updating.
    virtual void update_fixed(const lunaticvibes::Time& t) {}; // Called 'rate' amount of times.
    void MouseClick(InputMask& m, const lunaticvibes::Time& t);
    void MouseDrag(InputMask& m, const lunaticvibes::Time& t);
    void MouseRelease(InputMask& m, const lunaticvibes::Time& t);
    virtual void draw() const;

    [[nodiscard]] SkinBase::skinInfo getSkinInfo() const { return pSkin ? pSkin->info : SkinBase::skinInfo(); }

protected:
    [[nodiscard]] virtual bool shouldShowImgui() const;
    virtual void updateImgui();
    void DebugToggle(InputMask& m, const lunaticvibes::Time& t);

    [[nodiscard]] bool isInTextEdit() const;
    [[nodiscard]] IndexText textEditType() const;
    virtual bool checkAndStartTextEdit() { return false; }
    virtual void startTextEdit(bool clear);
    virtual void stopTextEdit(bool modify);

    void GlobalFuncKeys(InputMask& m, const lunaticvibes::Time& t);
};

////////////////////////////////////////////////////////////////////////////////

struct InputTimerSwitchMap
{
    IndexTimer tm;
    IndexSwitch sw;
};

inline const InputTimerSwitchMap InputGamePressMap[] = {
    {IndexTimer::S1L_DOWN, IndexSwitch::S1L_DOWN},           {IndexTimer::S1R_DOWN, IndexSwitch::S1R_DOWN},
    {IndexTimer::K11_DOWN, IndexSwitch::K11_DOWN},           {IndexTimer::K12_DOWN, IndexSwitch::K12_DOWN},
    {IndexTimer::K13_DOWN, IndexSwitch::K13_DOWN},           {IndexTimer::K14_DOWN, IndexSwitch::K14_DOWN},
    {IndexTimer::K15_DOWN, IndexSwitch::K15_DOWN},           {IndexTimer::K16_DOWN, IndexSwitch::K16_DOWN},
    {IndexTimer::K17_DOWN, IndexSwitch::K17_DOWN},           {IndexTimer::K18_DOWN, IndexSwitch::K18_DOWN},
    {IndexTimer::K19_DOWN, IndexSwitch::K19_DOWN},           {IndexTimer::K1START_DOWN, IndexSwitch::K1START_DOWN},
    {IndexTimer::K1SELECT_DOWN, IndexSwitch::K1SELECT_DOWN}, {IndexTimer::K1SPDUP_DOWN, IndexSwitch::K1SPDUP_DOWN},
    {IndexTimer::K1SPDDN_DOWN, IndexSwitch::K1SPDDN_DOWN},   {IndexTimer::S2L_DOWN, IndexSwitch::S2L_DOWN},
    {IndexTimer::S2R_DOWN, IndexSwitch::S2R_DOWN},           {IndexTimer::K21_DOWN, IndexSwitch::K21_DOWN},
    {IndexTimer::K22_DOWN, IndexSwitch::K22_DOWN},           {IndexTimer::K23_DOWN, IndexSwitch::K23_DOWN},
    {IndexTimer::K24_DOWN, IndexSwitch::K24_DOWN},           {IndexTimer::K25_DOWN, IndexSwitch::K25_DOWN},
    {IndexTimer::K26_DOWN, IndexSwitch::K26_DOWN},           {IndexTimer::K27_DOWN, IndexSwitch::K27_DOWN},
    {IndexTimer::K28_DOWN, IndexSwitch::K28_DOWN},           {IndexTimer::K29_DOWN, IndexSwitch::K29_DOWN},
    {IndexTimer::K2START_DOWN, IndexSwitch::K2START_DOWN},   {IndexTimer::K2SELECT_DOWN, IndexSwitch::K2SELECT_DOWN},
    {IndexTimer::K2SPDUP_DOWN, IndexSwitch::K2SPDUP_DOWN},   {IndexTimer::K2SPDDN_DOWN, IndexSwitch::K2SPDDN_DOWN},
};

inline const InputTimerSwitchMap InputGameReleaseMap[] = {
    {IndexTimer::S1L_UP, IndexSwitch::S1L_DOWN},           {IndexTimer::S1R_UP, IndexSwitch::S1R_DOWN},
    {IndexTimer::K11_UP, IndexSwitch::K11_DOWN},           {IndexTimer::K12_UP, IndexSwitch::K12_DOWN},
    {IndexTimer::K13_UP, IndexSwitch::K13_DOWN},           {IndexTimer::K14_UP, IndexSwitch::K14_DOWN},
    {IndexTimer::K15_UP, IndexSwitch::K15_DOWN},           {IndexTimer::K16_UP, IndexSwitch::K16_DOWN},
    {IndexTimer::K17_UP, IndexSwitch::K17_DOWN},           {IndexTimer::K18_UP, IndexSwitch::K18_DOWN},
    {IndexTimer::K19_UP, IndexSwitch::K19_DOWN},           {IndexTimer::K1START_UP, IndexSwitch::K1START_DOWN},
    {IndexTimer::K1SELECT_UP, IndexSwitch::K1SELECT_DOWN}, {IndexTimer::K1SPDUP_UP, IndexSwitch::K1SPDUP_DOWN},
    {IndexTimer::K1SPDDN_UP, IndexSwitch::K1SPDDN_DOWN},   {IndexTimer::S2L_UP, IndexSwitch::S2L_DOWN},
    {IndexTimer::S2R_UP, IndexSwitch::S2R_DOWN},           {IndexTimer::K21_UP, IndexSwitch::K21_DOWN},
    {IndexTimer::K22_UP, IndexSwitch::K22_DOWN},           {IndexTimer::K23_UP, IndexSwitch::K23_DOWN},
    {IndexTimer::K24_UP, IndexSwitch::K24_DOWN},           {IndexTimer::K25_UP, IndexSwitch::K25_DOWN},
    {IndexTimer::K26_UP, IndexSwitch::K26_DOWN},           {IndexTimer::K27_UP, IndexSwitch::K27_DOWN},
    {IndexTimer::K28_UP, IndexSwitch::K28_DOWN},           {IndexTimer::K29_UP, IndexSwitch::K29_DOWN},
    {IndexTimer::K2START_UP, IndexSwitch::K2START_DOWN},   {IndexTimer::K2SELECT_UP, IndexSwitch::K2SELECT_DOWN},
    {IndexTimer::K2SPDUP_UP, IndexSwitch::K2SPDUP_DOWN},   {IndexTimer::K2SPDDN_UP, IndexSwitch::K2SPDDN_DOWN},
};

////////////////////////////////////////////////////////////////////////////////
