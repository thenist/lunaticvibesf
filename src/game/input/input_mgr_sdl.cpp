#ifndef _WIN32
#ifdef RENDER_SDL2

#include <common/assert.h>
#include <common/log.h>
#include <common/types.h>
#include <game/graphics/SDL2/input_SDL2.h>
#include <game/input/input_mgr.h>

#include <algorithm>
#include <array>
#include <mutex>
#include <shared_mutex>

#include <SDL.h>

struct SdlJoystickDeleter
{
    void operator()(SDL_GameController* joystick)
    {
        if (joystick != nullptr)
            SDL_GameControllerClose(joystick);
    }
};
using SdlJoystickPtr = std::unique_ptr<SDL_GameController, SdlJoystickDeleter>;

struct SdlInputHolder
{
    std::array<SDL_GameController*, InputMgr::MAX_JOYSTICK_COUNT> controllers;
    SdlInputHolder();
    SdlInputHolder(const SdlInputHolder&) = delete;
    SdlInputHolder(SdlInputHolder&&) = delete;
    SdlInputHolder& operator=(const SdlInputHolder&) = delete;
    SdlInputHolder& operator=(SdlInputHolder&&) = delete;
    ~SdlInputHolder();
};

SdlInputHolder::SdlInputHolder()
{
    LOG_INFO << "[SDL2] Initializing SDL_INIT_GAMECONTROLLER";
    SDL_Init(SDL_INIT_GAMECONTROLLER);
}

SdlInputHolder::~SdlInputHolder()
{
    controllers.fill(nullptr);
    LOG_INFO << "[SDL2] De-initializing SDL_INIT_GAMECONTROLLER";
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

[[nodiscard]] static SdlInputHolder& getInputState()
{
    static SdlInputHolder state;
    return state;
}

void initInput()
{
    LOG_WARNING << "[SDL2] Direct input isn't available, input precision is limited by draw FPS";
    (void)getInputState();
}

inline SDL_GameController* g_game_controller;

void refreshInputDevices()
{
    auto& state = getInputState();
    int ret = SDL_NumJoysticks();
    if (ret < 0)
    {
        LOG_ERROR << "[SDL2] SDL_NumJoysticks failed: " << SDL_GetError();
        return;
    }
    if (ret < 1)
    {
        LOG_INFO << "[SDL2] No game controller connected";
        return;
    }
    const int joystick_count = ret;
    LOG_INFO << "[SDL2] Found " << joystick_count << " joysticks";
    for (int i = 0; i < std::min(joystick_count, static_cast<int>(sdl::state::g_con_buttons.size())); ++i)
    {
        LOG_DEBUG << "[SDL2] Opening game controller " << i;
        auto j = SDL_GameControllerOpen(i);
        if (j == nullptr)
        {
            LOG_ERROR << "[SDL2] Failed to open game controller: " << SDL_GetError();
            continue;
        }

        const char* name = SDL_GameControllerName(j);
        if (name == nullptr)
            name = "(unnamed)";

        std::array<char, 64> guid{0};
        SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(SDL_GameControllerGetJoystick(j)), guid.data(), guid.size());

        state.controllers[i] = j;

        LOG_INFO << "[SDL2] Initialized game controller: " << name;
        LOG_DEBUG << "[SDL2] GUID: " << guid.data();
    }
}

void pollInput()
{
    // No-op.
    // We don't need to do anything, since all the updating is done by
    // window events.
}

bool isKeyPressed(Input::Keyboard key)
{
    std::shared_lock l{sdl::state::g_input_mutex};
    return sdl::state::g_keyboard_scancodes[sdl_key_from_common_scancode(key)];
}

bool isButtonPressed(Input::Joystick c, double deadzone)
{
    switch (c.type)
    {
    case Input::Joystick::Type::BUTTON: {
        std::shared_lock l{sdl::state::g_input_mutex};
        LVF_DEBUG_ASSERT(c.device < sdl::state::g_con_buttons.size());
        const auto& buttons = sdl::state::g_con_buttons[c.device];
        if (buttons.size() < c.index)
            return false;
        return buttons[c.index];
    }
    case Input::Joystick::Type::AXIS_RELATIVE_POSITIVE:
    case Input::Joystick::Type::AXIS_RELATIVE_NEGATIVE: return getJoystickAxis(c.device, c.type, c.index) >= deadzone;
    case Input::Joystick::Type::POV: return false; // Hat in SDL terminology. ENOTSUP.
    case Input::Joystick::Type::UNDEF:
    case Input::Joystick::Type::AXIS_ABSOLUTE: break;
    }
    lunaticvibes::verify_failed("isButtonPressed");
    return false;
}

double getJoystickAxis(size_t device, Input::Joystick::Type type, size_t index)
{
    //  Axis conversions are broken.
    /*
    LVF_DEBUG_ASSERT(device < sdl::state::g_con_axes.size());
    const auto& axes = sdl::state::g_con_axes[device];
    if (axes.size() < index)
        return -1.;
    std::shared_lock l{sdl::state::g_input_mutex};
    const auto axis = static_cast<int16_t>(axes[index]);
    l.unlock();
    if (axis == 0 || axis == std::numeric_limits<decltype(axis)>::min())
        return -1.;
    switch (type)
    {
    case Input::Joystick::Type::UNDEF:
    case Input::Joystick::Type::BUTTON:
    case Input::Joystick::Type::POV: break;
    case Input::Joystick::Type::AXIS_RELATIVE_POSITIVE: return axis / -32767.0;
    case Input::Joystick::Type::AXIS_RELATIVE_NEGATIVE: return axis / 32767.0;
    case Input::Joystick::Type::AXIS_ABSOLUTE: return axis / 65535.;
    }
    lunaticvibes::verify_failed("getJoystickAxis");
    */
    return -1.;
}

bool isMouseButtonPressed(int idx)
{
    // Lunaticvibes expects middle and right mouse buttons to be swapped.
    switch (idx)
    {
    case 2: idx = 3; break;
    case 3: idx = 2; break;
    default: break;
    }

    std::shared_lock l{sdl::state::g_input_mutex};
    return sdl::state::g_mouse_buttons[idx];
}

short getLastMouseWheelState()
{
    std::unique_lock l{sdl::state::g_input_mutex};
    auto state = sdl::state::g_mouse_wheel_delta;
    sdl::state::g_mouse_wheel_delta = 0;
    return state;
}

#endif // RENDER_SDL2
#endif // _WIN32
