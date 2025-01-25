#pragma once

#include <common/asynclooper.h>
#include <common/beat.h>
#include <game/input/input_callback.h>
#include <game/input/input_mgr.h>

#include <array>
#include <map>
#include <shared_mutex>
#include <utility>

// InputWrapper
//  Start a process to check input upon 1000hz polling.
// Interface:
//  Pressed / Holding / Released FULL bitset
//  Pressed / Holding / Released per key
class InputWrapper
{
public:
    unsigned release_delay_ms = 5;

private:
    struct InputPressEvent
    {
        lunaticvibes::InputMaskTimes times;
        lunaticvibes::Time t;
        InputMask mask;
    };
    struct InputHoldEvent
    {
        lunaticvibes::Time t;
        InputMask mask;
    };
    struct InputReleaseEvent
    {
        lunaticvibes::Time t;
        InputMask mask;
    };
    struct ScratchEvent
    {
        lunaticvibes::Time t;
        double delta1;
        double delta2;
    };
    std::queue<InputHoldEvent> _input_hold_events;
    std::queue<InputPressEvent> _input_press_events;
    std::queue<InputReleaseEvent> _input_release_events;
    std::queue<ScratchEvent> _scratch_events;

    mutable std::shared_mutex _inputMutex;
    AsyncLooper _looper;
    lunaticvibes::Time _prevUpdateEnd;

protected:
    std::array<std::pair<long long, bool>, Input::KEY_COUNT> _inputBuffer{{{0, false}}};
    std::array<long long, Input::KEY_COUNT> _releaseBuffer{-1};
    int _cursor_x = 0, _cursor_y = 0;
    bool _background = false;

    InputMask _prev = 0;
    InputMask _curr = 0;

    bool scratchAxisSet = false;
    double scratchAxisPrev[2] = {0.};
    double scratchAxisCurr[2] = {0.};

    bool mergeInput = false;

public:
    InputWrapper(unsigned rate = 1000, bool background = false);
    ~InputWrapper();

public:
    void loopEnd() { _looper.loopEnd(); }
    void loopStart() { _looper.loopStart(); }

    [[nodiscard]] bool isRunning() const { return _looper.isRunning(); }

    [[nodiscard]] unsigned getRate() { return _looper.getRate(); }
    void setRate(unsigned rate_per_sec) { _looper.setRate(rate_per_sec); }

private:
    void loopAsync();

public:
    // Invoke callbacks for detected input.
    // Locks '_inputMutex'.
    void processInput();

    [[nodiscard]] InputMask Holding() const
    {
        // FIXME: this will probably recursively lock if this function is called from a callback in processInput.
        // std::shared_lock l(_inputMutex);
        return _prev & _curr;
    }

    [[nodiscard]] std::pair<int, int> getCursorPos() const
    {
        // FIXME: this will probably recursively lock if this function is called from a callback in processInput.
        // std::shared_lock l(_inputMutex);
        return {_cursor_x, _cursor_y};
    }

    [[nodiscard]] double getJoystickAxis(size_t device, Input::Joystick::Type type, size_t index);

    // Merge 2P button inputs into 1P. Note that abs axis are ALSO merged.
    void setMergeInput() { mergeInput = true; }

private:
    std::map<const std::string, lunaticvibes::InputCallback> _pCallbackMapNew;
    std::map<const std::string, INPUTCALLBACK> _pCallbackMap;
    std::map<const std::string, INPUTCALLBACK> _hCallbackMap;
    std::map<const std::string, INPUTCALLBACK> _rCallbackMap;
    std::map<const std::string, AXISPLUSCALLBACK> _aCallbackMap;

    bool _register(unsigned type, const std::string& key, INPUTCALLBACK);

public:
    // These must only be called before input loop starts.
    bool register_p(const std::string& key, INPUTCALLBACK f) { return _register(0, key, std::move(f)); }
    bool register_p_new(const std::string& key, lunaticvibes::InputCallback f);
    bool register_h(const std::string& key, INPUTCALLBACK f) { return _register(1, key, std::move(f)); }
    bool register_r(const std::string& key, INPUTCALLBACK f) { return _register(2, key, std::move(f)); }
    bool register_a(const std::string& key, AXISPLUSCALLBACK f);

    // Should only used for keyconfig
private:
    KeyboardMask _kbcurr = 0;
    KeyboardMask _kbprev = 0;
    std::array<JoystickAxis, InputMgr::MAX_JOYSTICK_COUNT> _joyaxiscurr{};
    std::array<JoystickAxis, InputMgr::MAX_JOYSTICK_COUNT> _joyaxisprev{};
    std::array<JoystickMask, InputMgr::MAX_JOYSTICK_COUNT> _joycurr{};
    std::array<JoystickMask, InputMgr::MAX_JOYSTICK_COUNT> _joyprev{};
    std::map<const std::string, ABSAXISCALLBACK> _absaxisCallbackMap;
    std::map<const std::string, JOYSTICKCALLBACK> _joystickCallbackMap;
    std::map<const std::string, KEYBOARDCALLBACK> _keyboardCallbackMap;

public:
    // These must only be called before input loop starts.
    bool register_kb(const std::string& key, KEYBOARDCALLBACK f);
    bool register_joy(const std::string& key, JOYSTICKCALLBACK f);
    bool register_aa(const std::string& key, ABSAXISCALLBACK f);
};
