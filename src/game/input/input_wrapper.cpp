#include "input_wrapper.h"

#include <functional>
#include <mutex>
#include <utility>

#include <common/assert.h>
#include <common/beat.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/utils.h>
#include <game/runtime/generic_info.h>

InputWrapper::InputWrapper(unsigned rate, bool background)
    : _background(background), _looper{"InputLoop", std::bind_front(&InputWrapper::loopAsync, this), rate}
{
}

InputWrapper::~InputWrapper()
{
    LVF_ASSERT(!_looper.isRunning());
}

void InputWrapper::loopAsync()
{
    const auto now = lunaticvibes::Time::now();
    auto [curr, scratch_data] = InputMgr::detect();

    {
        std::unique_lock l(_inputMutex);
        _prev = _curr;
        _curr = curr;
    }

    // detect key / button
    InputMask p{0}, h{0}, r{0};
    lunaticvibes::InputMaskTimes pTimes{};
    if (!_background && !IsWindowForeground())
    {
        curr.reset();
    }
    if (mergeInput)
    {
        curr |= (curr >> Input::S2L) & INPUT_MASK_1P;
        curr &= ~INPUT_MASK_2P;
    }
    for (size_t i = Input::S1L; i < Input::KEY_COUNT; ++i)
    {
        auto& [ms, stat] = _inputBuffer[i];
        if (curr[i] && !stat)
        {
            ms = now.norm();
            stat = true;
            p.set(i);
            pTimes[i] = ms; // TODO: take time from the input manager lol.
        }
        else if (!curr[i] && stat)
        {
            if (_releaseBuffer[i] == -1)
            {
                _releaseBuffer[i] = now.norm();
            }
            else if (now.norm() - _releaseBuffer[i] >= release_delay_ms)
            {
                ms = now.norm();
                stat = false;
                _releaseBuffer[i] = -1;
                r.set(i);
            }
        }
        else if (stat)
        {
            h.set(i);
        }
    }

    // detect absolute axis
    scratchAxisPrev[0] = scratchAxisCurr[0];
    scratchAxisPrev[1] = scratchAxisCurr[1];
    scratchAxisCurr[0] = scratch_data.first;
    scratchAxisCurr[1] = scratch_data.second;
    if (scratchAxisSet)
    {
        double d1 = normalizeLinearGrowth(scratchAxisPrev[0], scratchAxisCurr[0]) * InputMgr::getDeadzone(Input::S1A);
        double d2 = normalizeLinearGrowth(scratchAxisPrev[1], scratchAxisCurr[1]) * InputMgr::getDeadzone(Input::S2A);
        if (d1 != 0. || d2 != 0.)
        {
            std::unique_lock l(_inputQueueMutex);
            _scratch_events.emplace(now, d1, d2);
        }
    }
    scratchAxisSet = true;

    // mouse pos
    {
        int x, y;
        InputMgr::getMousePos(x, y);
        std::unique_lock l(_inputMutex);
        _cursor_x = x;
        _cursor_y = y;
    }

    // key config callbacks
    // FIXME: make this thread-safe. Accumulate input and invoke callbacks in processInput().
    if (_background || IsWindowForeground())
    {
        if (!_keyboardCallbackMap.empty())
        {
            KeyboardMask mask;
            _kbprev = _kbcurr;
            for (auto i = static_cast<unsigned>(Input::Keyboard::K_ESC);
                 i != static_cast<unsigned>(Input::Keyboard::K_COUNT); ++i)
            {
                const auto k = static_cast<Input::Keyboard>(i);
                if (isKeyPressed(k))
                    mask.set(i);
            }
            _kbcurr = mask;

            KeyboardMask p;
            for (auto ki = static_cast<unsigned>(Input::Keyboard::K_ESC);
                 ki != static_cast<unsigned>(Input::Keyboard::K_COUNT); ++ki)
            {
                if (_kbcurr[ki] && !_kbprev[ki])
                    p.set(ki);
            }
            if (p.any())
            {
                for (auto& [cbname, callback] : _keyboardCallbackMap)
                    callback(mask, now);
            }
        }

        if (!_joystickCallbackMap.empty())
        {
            _joyprev = _joycurr;
            for (size_t device = 0; device < InputMgr::MAX_JOYSTICK_COUNT; ++device)
            {
                JoystickMask mask;

                Input::Joystick j;
                j.device = device;
                size_t base = 0;

                j.type = Input::Joystick::Type::BUTTON;
                for (j.index = 0; j.index < InputMgr::MAX_JOYSTICK_BUTTON_COUNT; ++j.index)
                {
                    if (isButtonPressed(j))
                        mask.set(base + j.index);
                }
                base += InputMgr::MAX_JOYSTICK_BUTTON_COUNT;

                j.type = Input::Joystick::Type::POV;
                for (size_t idxPOV = 0; idxPOV < InputMgr::MAX_JOYSTICK_POV_COUNT; ++idxPOV)
                {
                    j.index = idxPOV | (1ul << 31);
                    if (isButtonPressed(j))
                        mask.set(base + idxPOV * 4 + 0);
                    j.index = idxPOV | (1ul << 30);
                    if (isButtonPressed(j))
                        mask.set(base + idxPOV * 4 + 1);
                    j.index = idxPOV | (1ul << 29);
                    if (isButtonPressed(j))
                        mask.set(base + idxPOV * 4 + 2);
                    j.index = idxPOV | (1ul << 28);
                    if (isButtonPressed(j))
                        mask.set(base + idxPOV * 4 + 3);
                }
                base += InputMgr::MAX_JOYSTICK_POV_COUNT * 4;

                j.type = Input::Joystick::Type::AXIS_RELATIVE_POSITIVE;
                for (j.index = 0; j.index < InputMgr::MAX_JOYSTICK_AXIS_COUNT; ++j.index)
                {
                    if (isButtonPressed(j, 0.7) && !_joyprev[device][base + j.index])
                        mask.set(base + j.index);
                }
                base += InputMgr::MAX_JOYSTICK_AXIS_COUNT;

                j.type = Input::Joystick::Type::AXIS_RELATIVE_NEGATIVE;
                for (j.index = 0; j.index < InputMgr::MAX_JOYSTICK_AXIS_COUNT; ++j.index)
                {
                    if (isButtonPressed(j, 0.7) && !_joyprev[device][base + j.index])
                        mask.set(base + j.index);
                }
                base += InputMgr::MAX_JOYSTICK_AXIS_COUNT;

                _joycurr[device] = mask;

                JoystickMask p;
                for (size_t ki = 0; ki < MAX_JOYSTICK_MASK_BIT_COUNT; ++ki)
                {
                    if (_joycurr[device][ki] && !_joyprev[device][ki])
                        p.set(ki);
                }
                if (p.any())
                {
                    for (auto& [cbname, callback] : _joystickCallbackMap)
                        callback(mask, device, now);
                }
            }
        }

        if (!_absaxisCallbackMap.empty())
        {
            _joyaxisprev = _joyaxiscurr;
            for (size_t device = 0; device < InputMgr::MAX_JOYSTICK_COUNT; ++device)
            {
                JoystickAxis mask;
                mask.fill(-1.0);
                bool moved = false;
                for (size_t index = 0; index < InputMgr::MAX_JOYSTICK_AXIS_COUNT; ++index)
                {
                    double axis = getJoystickAxis(device, Input::Joystick::Type::AXIS_ABSOLUTE, index);
                    if (axis != -1.0)
                    {
                        double delta = normalizeLinearGrowth(_joyaxisprev[device][index], axis);
                        if (std::abs(delta) > 0.05)
                        {
                            moved = true;
                            _joyaxiscurr[device][index] = axis;
                            mask[index] = axis;
                        }
                    }
                }
                if (moved)
                {
                    for (auto& [cbname, callback] : _absaxisCallbackMap)
                        callback(mask, device, now);
                }
            }
        }
    }

    if (p.any())
    {
        std::unique_lock l(_inputQueueMutex);
        _input_press_events.emplace(pTimes, now, p);
    }
    if (h.any())
    {
        std::unique_lock l(_inputQueueMutex);
        _input_hold_events.emplace(now, h);
    }
    if (r.any())
    {
        std::unique_lock l(_inputQueueMutex);
        _input_release_events.emplace(now, r);
    }

    // Also count sleeping which is done outside of this function.
    auto update_end = lunaticvibes::Time::now();
    lunaticvibes::g_feed_frametime(lunaticvibes::FrameTimeType::Input, update_end - _prevUpdateEnd);
    _prevUpdateEnd = update_end;
}

void InputWrapper::processInput()
{
    {
        std::unique_lock l(_inputQueueMutex);
        auto drain = []<typename T>(std::queue<T>& q, std::vector<T>& v) {
            while (!q.empty())
            {
                v.push_back(std::move(q.front()));
                q.pop();
            }
        };
        drain(_input_press_events, _drained_input_press_events);
        drain(_input_hold_events, _drained_input_hold_events);
        drain(_input_release_events, _drained_input_release_events);
        drain(_scratch_events, _drained_scratch_events);
    }

    for (auto& event : _drained_input_press_events)
    {
        for (auto& [cbname, callback] : _pCallbackMap)
            callback(event.mask, event.t);
        for (auto& [cbname, callback] : _pCallbackMapNew)
            callback(event.mask, event.t, event.times);
    }
    _drained_input_press_events.clear();

    for (auto& event : _drained_input_hold_events)
        for (auto& [cbname, callback] : _hCallbackMap)
            callback(event.mask, event.t);
    _drained_input_hold_events.clear();

    for (auto& event : _drained_input_release_events)
        for (auto& [cbname, callback] : _rCallbackMap)
            callback(event.mask, event.t);
    _drained_input_release_events.clear();

    for (auto& event : _drained_scratch_events)
        for (auto& [cbname, callback] : _aCallbackMap)
        {
            if (mergeInput)
                callback(event.delta1 + event.delta2, 0.0, event.t);
            else
                callback(event.delta1, event.delta2, event.t);
        }
    _drained_scratch_events.clear();
}

double InputWrapper::getJoystickAxis(size_t device, Input::Joystick::Type type, size_t index)
{
    return ::getJoystickAxis(device, type, index);
}

bool InputWrapper::_register(unsigned type, const std::string& key, INPUTCALLBACK f)
{
    LVF_ASSERT(!_looper.isRunning());
    switch (type)
    {
    case 0:
        LVF_ASSERT(!_pCallbackMap.contains(key));
        _pCallbackMap[key] = std::move(f);
        return true;
    case 1:
        LVF_ASSERT(!_hCallbackMap.contains(key));
        _hCallbackMap[key] = std::move(f);
        return true;
    case 2:
        LVF_ASSERT(!_rCallbackMap.contains(key));
        _rCallbackMap[key] = std::move(f);
        return true;
    default: lunaticvibes::assert_failed("InputWrapper::_register");
    }
}

bool InputWrapper::register_p_new(const std::string& key, lunaticvibes::InputCallback f)
{
    LVF_ASSERT(!_looper.isRunning());
    LVF_ASSERT(!_pCallbackMapNew.contains(key));
    _pCallbackMapNew[key] = std::move(f);
    return true;
}

bool InputWrapper::register_a(const std::string& key, AXISPLUSCALLBACK f)
{
    LVF_ASSERT(!_looper.isRunning());
    LVF_ASSERT(!_aCallbackMap.contains(key));
    _aCallbackMap[key] = std::move(f);
    return true;
}

bool InputWrapper::register_kb(const std::string& key, KEYBOARDCALLBACK f)
{
    LVF_ASSERT(!_looper.isRunning());
    LVF_ASSERT(!_keyboardCallbackMap.contains(key));
    _keyboardCallbackMap[key] = std::move(f);
    return true;
}

bool InputWrapper::register_joy(const std::string& key, JOYSTICKCALLBACK f)
{
    LVF_ASSERT(!_looper.isRunning());
    LVF_ASSERT(!_joystickCallbackMap.contains(key));
    _joystickCallbackMap[key] = std::move(f);
    return true;
}

bool InputWrapper::register_aa(const std::string& key, ABSAXISCALLBACK f)
{
    LVF_ASSERT(!_looper.isRunning());
    LVF_ASSERT(!_absaxisCallbackMap.contains(key));
    _absaxisCallbackMap[key] = std::move(f);
    return true;
}
