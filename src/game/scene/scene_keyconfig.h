#pragma once
#include "common/beat.h"
#include "scene.h"

#include <common/keymap.h>
#include <common/types.h>

#include <shared_mutex>

class SceneKeyConfig final : public SceneBase
{
public:
    explicit SceneKeyConfig(const std::shared_ptr<SkinMgr>& skinMgr);
    ~SceneKeyConfig() override;

protected:
    enum class SceneKeyConfigState : uint8_t
    {
        START,
        MAIN,
        FADEOUT,
    };
    SceneKeyConfigState _state;
    void update_fixed(const lunaticvibes::Time& t) override;
    void updateStart(const lunaticvibes::Time& t);
    void updateMain(const lunaticvibes::Time& t);
    void updateFadeout(const lunaticvibes::Time& t);

protected:
    std::map<Input::Pad, long long> forceBargraphTriggerTimestamp;
    void updateForceBargraphs(const lunaticvibes::Time& t);

    void updateInfo(KeyMap k, int slot);
    void updateAllText();

    std::map<size_t, JoystickMask> joystickPrev;
    std::array<double, 2> playerTurntableAngleAdd{0};

protected:
    // Register to InputWrapper: judge / keysound
    void inputGamePress(InputMask&, const lunaticvibes::Time&);
    void inputGameAxis(double s1, double s2, const lunaticvibes::Time&);
    void inputGamePressKeyboard(KeyboardMask&, const lunaticvibes::Time&);
    void inputGamePressJoystick(JoystickMask&, size_t device, const lunaticvibes::Time&);
    void inputGameAbsoluteAxis(JoystickAxis&, size_t device, const lunaticvibes::Time&);

public:
    static void setInputBindingText(GameModeKeys keys, Input::Pad pad);

private:
    bool exiting = false;
    std::shared_mutex _mutex;
};
