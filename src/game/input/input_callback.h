#pragma once

#include <common/beat.h>
#include <common/keymap.h>
#include <game/input/input_mgr.h>

#include <array>
#include <bitset>
#include <functional>

namespace lunaticvibes
{

using InputMask = std::bitset<Input::Pad::KEY_COUNT>;
using INPUTCALLBACK = std::function<void(InputMask&, const lunaticvibes::Time&)>;

using KeyboardMask = std::bitset<Input::keyboardKeyCount>;
using KEYBOARDCALLBACK = std::function<void(KeyboardMask&, const lunaticvibes::Time&)>;

using AXISPLUSCALLBACK = std::function<void(double, double, const lunaticvibes::Time&)>;

constexpr size_t MAX_JOYSTICK_MASK_BIT_COUNT =
    InputMgr::MAX_JOYSTICK_BUTTON_COUNT + InputMgr::MAX_JOYSTICK_POV_COUNT * 4 + InputMgr::MAX_JOYSTICK_AXIS_COUNT * 2;
using JoystickMask = std::bitset<MAX_JOYSTICK_MASK_BIT_COUNT>;
using JOYSTICKCALLBACK = std::function<void(JoystickMask&, size_t, const lunaticvibes::Time&)>;

using JoystickAxis = std::array<double, InputMgr::MAX_JOYSTICK_AXIS_COUNT>;
using ABSAXISCALLBACK = std::function<void(JoystickAxis&, size_t, const lunaticvibes::Time&)>;

using InputMaskTimes = std::array<lunaticvibes::Time, Input::Pad::KEY_COUNT>;
// This will eventually replace ::INPUTCALLBACK.
using InputCallback = std::function<void(InputMask&, const lunaticvibes::Time&, const lunaticvibes::InputMaskTimes&)>;

// FUNC:                                           BRDUEHDI><v^543210987654321_
inline constexpr InputMask INPUT_MASK_FUNC{"0000000111111111111111111111111111100000000000000000000000000000000"};
// 1P:                                                                                       sDUEA987654321SS
inline constexpr InputMask INPUT_MASK_1P{"0000000000000000000000000000000000000000000000000001111111111111111"};
// 2P:                                                                       sDUEA987654321SS
inline constexpr InputMask INPUT_MASK_2P{"0000000000000000000000000000000000011111111111111110000000000000000"};
// Mouse:                                    DU54321
inline constexpr InputMask INPUT_MASK_MOUSE{"1111111000000000000000000000000000000000000000000000000000000000000"};

//                                                    Return >                     2P: 9 7 5 3 1  1P: 9 7 5 3 1
inline constexpr InputMask INPUT_MASK_DECIDE{"0000000010000001000000000000000000000000010101010100000010101010100"};
//                                                   Bksp     <                    2P:  8 6 4 2   1P:  8 6 4 2
inline constexpr InputMask INPUT_MASK_CANCEL{"0000000100000000100000000000000000000000001010101000000001010101000"};
//                                                              ^                  2P:           S1P:           S
inline constexpr InputMask INPUT_MASK_NAV_UP{"0000000000000000001000000000000000000000000000000001000000000000001"};
//                                                             v                   2P:          S 1P:          S
inline constexpr InputMask INPUT_MASK_NAV_DN{"0000000000000000010000000000000000000000000000000010000000000000010"};

//                                                       Return >                     2P:     5      1P:     5
inline constexpr InputMask INPUT_MASK_DECIDE_9K{"0000000010000001000000000000000000000000000001000000000000001000000"};
//                                                      Bksp     <                    2P:   7   3    1P:   7   3
inline constexpr InputMask INPUT_MASK_CANCEL_9K{"0000000100000000100000000000000000000000000100010000000000100010000"};
//                                                                 ^                  2P:      4    S1P:      4    S
inline constexpr InputMask INPUT_MASK_NAV_UP_9K{"0000000000000000001000000000000000000000000000100001000000000100001"};
//                                                                v                   2P:    6     S 1P:    6     S
inline constexpr InputMask INPUT_MASK_NAV_DN_9K{"0000000000000000010000000000000000000000000010000010000000010000010"};

} // namespace lunaticvibes

using lunaticvibes::ABSAXISCALLBACK;
using lunaticvibes::AXISPLUSCALLBACK;
using lunaticvibes::INPUT_MASK_1P;
using lunaticvibes::INPUT_MASK_2P;
using lunaticvibes::INPUT_MASK_CANCEL;
using lunaticvibes::INPUT_MASK_CANCEL_9K;
using lunaticvibes::INPUT_MASK_DECIDE;
using lunaticvibes::INPUT_MASK_DECIDE_9K;
using lunaticvibes::INPUT_MASK_FUNC;
using lunaticvibes::INPUT_MASK_MOUSE;
using lunaticvibes::INPUT_MASK_NAV_DN;
using lunaticvibes::INPUT_MASK_NAV_DN_9K;
using lunaticvibes::INPUT_MASK_NAV_UP;
using lunaticvibes::INPUT_MASK_NAV_UP_9K;
using lunaticvibes::INPUTCALLBACK;
using lunaticvibes::InputMask;
using lunaticvibes::JoystickAxis;
using lunaticvibes::JOYSTICKCALLBACK;
using lunaticvibes::JoystickMask;
using lunaticvibes::KEYBOARDCALLBACK;
using lunaticvibes::KeyboardMask;
using lunaticvibes::MAX_JOYSTICK_MASK_BIT_COUNT;
