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
    std::shared_mutex _inputMutex;
    AsyncLooper _looper;

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
    [[nodiscard]] InputMask Holding() const { return _prev & _curr; }

    [[nodiscard]] std::pair<int, int> getCursorPos() const { return {_cursor_x, _cursor_y}; }

    [[nodiscard]] double getJoystickAxis(size_t device, Input::Joystick::Type type, size_t index);

    // Merge 2P button inputs into 1P. Note that abs axis are ALSO merged.
    void setMergeInput() { mergeInput = true; }

private:
    // Callback function maps
    std::map<const std::string, lunaticvibes::InputCallback> _pCallbackMapNew;
    std::map<const std::string, INPUTCALLBACK> _pCallbackMap;
    std::map<const std::string, INPUTCALLBACK> _hCallbackMap;
    std::map<const std::string, INPUTCALLBACK> _rCallbackMap;
    std::map<const std::string, AXISPLUSCALLBACK> _aCallbackMap;

private:
    // Callback registering
    bool _register(unsigned type, const std::string& key, INPUTCALLBACK);
    bool _unregister(unsigned type, const std::string& key);

public:
    bool register_p(const std::string& key, INPUTCALLBACK f) { return _register(0, key, std::move(f)); }
    bool register_p_new(const std::string& key, lunaticvibes::InputCallback f);
    bool register_h(const std::string& key, INPUTCALLBACK f) { return _register(1, key, std::move(f)); }
    bool register_r(const std::string& key, INPUTCALLBACK f) { return _register(2, key, std::move(f)); }
    bool unregister_p(const std::string& key) { return _unregister(0, key); }
    bool unregister_h(const std::string& key) { return _unregister(1, key); }
    bool unregister_r(const std::string& key) { return _unregister(2, key); }
    bool register_a(const std::string& key, AXISPLUSCALLBACK f);
    bool unregister_a(const std::string& key);

    // Should only used for keyconfig
protected:
    KeyboardMask _kbprev = 0;
    KeyboardMask _kbcurr = 0;

private:
    std::map<const std::string, KEYBOARDCALLBACK> _keyboardCallbackMap;

public:
    bool register_kb(const std::string& key, KEYBOARDCALLBACK f);
    bool unregister_kb(const std::string& key);

protected:
    std::array<JoystickMask, InputMgr::MAX_JOYSTICK_COUNT> _joyprev{};
    std::array<JoystickMask, InputMgr::MAX_JOYSTICK_COUNT> _joycurr{};

private:
    std::map<const std::string, JOYSTICKCALLBACK> _joystickCallbackMap;

public:
    bool register_joy(const std::string& key, JOYSTICKCALLBACK f);
    bool unregister_joy(const std::string& key);

protected:
    std::array<JoystickAxis, InputMgr::MAX_JOYSTICK_COUNT> _joyaxisprev{};
    std::array<JoystickAxis, InputMgr::MAX_JOYSTICK_COUNT> _joyaxiscurr{};

private:
    std::map<const std::string, ABSAXISCALLBACK> _absaxisCallbackMap;

public:
    bool register_aa(const std::string& key, ABSAXISCALLBACK f);
    bool unregister_aa(const std::string& key);
};
