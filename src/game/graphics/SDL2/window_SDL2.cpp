#ifdef RENDER_SDL2

#include <algorithm>
#include <climits>
#include <cstdint>
#include <mutex>
#include <ranges>
#include <string>
#include <utility>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_syswm.h>
#include <SDL_ttf.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include <common/assert.h>
#include <common/encoding.h>
#include <common/log.h>
#include <common/meta.h>
#include <common/sysutil.h>
#include <common/u8.h>
#include <config/config_mgr.h>
#include <game/graphics/SDL2/graphics_SDL2.h>
#include <game/graphics/SDL2/input.h>
#include <game/graphics/SDL2/window_SDL2.h>
#include <game/graphics/graphics.h>
#include <game/graphics/video.h>

namespace v = std::views;

static SDL_Rect canvasRect;
static SDL_Rect windowRect;
static double canvasScaleX = 1.0;
static double canvasScaleY = 1.0;

int graphics_init()
{
    LOG_INFO << "[SDL2] Initializing...";

    // SDL2
    {
        if (SDL_Init(SDL_INIT_VIDEO))
        {
            LOG_FATAL << "[SDL2] Library init ERROR! " << SDL_GetError();
            return -99;
        }
        LOG_INFO << "[SDL2] Library version " << SDL_MAJOR_VERSION << '.' << SDL_MINOR_VERSION << "." << SDL_PATCHLEVEL;

        std::string title;
        title += PROJECT_NAME;
#ifndef NDEBUG
        title += " Debug";
#endif

#ifdef _WIN32
        // I have tested several renderers on Windows and each of them has its own issues.
        // direct3d (D3D9): skin "filter" parameters are affected by textures
        // direct3d11: such a simple 2D game should not be so picky about hardware...
        // direct3d12: same as above; plus the dll from the official site does not include this renderer
        // opengl: does not support certain custom blend mode
        // opengles2: does not support virtual textures created by hand (e.g. black dot)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#else
        // SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif

        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

        Uint32 flags = SDL_WINDOW_HIDDEN; // window created with opengles2 will destroy itself when creating Renderer,
                                          // resulting in flashes
        flags |= SDL_WINDOW_RESIZABLE;
        auto mode = ConfigMgr::get('V', cfg::V_WINMODE, cfg::V_WINMODE_WINDOWED);
        if (lunaticvibes::iequals(mode, cfg::V_WINMODE_BORDERLESS))
        {
            flags |= SDL_WINDOW_BORDERLESS;
        }
        else if (lunaticvibes::iequals(mode, cfg::V_WINMODE_FULL))
        {
            flags |= SDL_WINDOW_FULLSCREEN;
        }
        else
        {
            // fallback to windowed
        }
        windowRect.w = ConfigMgr::get('V', cfg::V_DISPLAY_RES_X, CANVAS_WIDTH);
        windowRect.h = ConfigMgr::get('V', cfg::V_DISPLAY_RES_Y, CANVAS_HEIGHT);
        gFrameWindow = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowRect.w,
                                        windowRect.h, flags);
        if (!gFrameWindow)
        {
            LOG_FATAL << "[SDL2] Init window ERROR! " << SDL_GetError();
            return -1;
        }

        // canvasRect.w = ConfigMgr::get('V', cfg::V_RES_X, CANVAS_WIDTH);
        // canvasRect.h = ConfigMgr::get('V', cfg::V_RES_Y, CANVAS_HEIGHT);
        // graphics_resize_canvas(canvasRect.w, canvasRect.h);
        graphics_resize_canvas(windowRect.w, windowRect.h);

        int ss = ConfigMgr::get('V', cfg::V_RES_SUPERSAMPLE, 1);
        graphics_set_supersample_level(ss);

        int maxFPS = ConfigMgr::get('V', cfg::V_MAXFPS, 480);
        if (maxFPS < 30 && maxFPS != 0)
            maxFPS = 30;
        graphics_set_maxfps(maxFPS);

        unsigned renderer_flags = SDL_RENDERER_TARGETTEXTURE;
        if (ConfigMgr::get('V', cfg::V_VSYNC, 0) != 0)
            renderer_flags += SDL_RENDERER_PRESENTVSYNC;
        gFrameRenderer = SDL_CreateRenderer(gFrameWindow, -1, renderer_flags);
        if (!gFrameRenderer)
        {
            LOG_FATAL << "[SDL2] Init renderer ERROR! " << SDL_GetError();
            return -2;
        }
        SDL_SetRenderDrawColor(gFrameRenderer, 0, 0, 0, 255);

        SDL_ShowWindow(gFrameWindow);

        SDL_RendererInfo rendererInfo;
        if (SDL_GetRendererInfo(gFrameRenderer, &rendererInfo) == 0)
        {
            LOG_INFO << "[SDL2] Renderer driver: " << rendererInfo.name;
        }

        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(gFrameWindow, &wmInfo) == SDL_TRUE)
        {
#ifdef _WIN32
            setWindowHandle((void*)&wmInfo.info.win.window);
#endif
        }

        gInternalRenderTarget = SDL_CreateTexture(gFrameRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
                                                  CANVAS_WIDTH_MAX, CANVAS_HEIGHT_MAX);
        if (!gInternalRenderTarget)
        {
            LOG_FATAL << "[SDL2] Init Target Texture Error! " << SDL_GetError();
            return -3;
        }
        // TODO: an option to set SDL_ScaleModeBest (anisotropic filtering).
        // Nearest neighbour as the default makes most sense, as that's what LR2 does (in windowed mode).
        SDL_SetTextureScaleMode(gInternalRenderTarget, SDL_ScaleModeNearest);

        SDL_SetRenderTarget(gFrameRenderer, gInternalRenderTarget);

        SDL_ShowCursor(SDL_DISABLE);

        LOG_INFO << "[SDL2] SDL2 init finished.";
    }

    // SDL_Image
    {
        LOG_INFO << "[SDL2] Initializing SDL2_Image...";

        auto flags = IMG_INIT_JPG | IMG_INIT_PNG;
        if (flags != IMG_Init(flags))
        {
            // error handling
            LOG_FATAL << "[SDL2] SDL2_Image init failed. " << IMG_GetError();
            return 1;
        }
        LOG_INFO << "[SDL2] SDL2_Image init finished. Version " << SDL_IMAGE_MAJOR_VERSION << '.'
                 << SDL_IMAGE_MINOR_VERSION << "." << SDL_IMAGE_PATCHLEVEL;
    }
    // SDL_ttf
    {
        LOG_INFO << "[SDL2] Initializing SDL2_TTF...";

        if (-1 == TTF_Init())
        {
            // error handling
            LOG_FATAL << "[SDL2] SDL2_TTF init failed. " << TTF_GetError();
            return 2;
        }
        LOG_INFO << "[SDL2] SDL2_TTF init finished. Version " << SDL_TTF_MAJOR_VERSION << '.' << SDL_TTF_MINOR_VERSION
                 << "." << SDL_TTF_PATCHLEVEL;
    }

    // libav
    video_init();

    // imgui
    LOG_INFO << "Initializing ImGui for SDL renderer...";
    if (!ImGui_ImplSDL2_InitForSDLRenderer(gFrameWindow, gFrameRenderer) ||
        !ImGui_ImplSDLRenderer2_Init(gFrameRenderer))
    {
        LOG_FATAL << "ImGui init failed.";
        return 3;
    }
    LOG_INFO << "ImGui init finished.";

    // Draw a black frame to prevent flashbang
    graphics_clear();
    graphics_flush();

    return 0;
}

void graphics_clear()
{
    SDL_RenderClear(gFrameRenderer);
}

static int maxFPS = 0;
static bool s_should_present_imgui = false;
static std::chrono::nanoseconds desiredFrameTimeBetweenFrames;
static std::chrono::high_resolution_clock::time_point frameTimestampPrev;
static Path screenshotPath;
void graphics_flush()
{
    SDL_SetRenderTarget(gFrameRenderer, nullptr);
    {
        // TODO scale internal canvas
        Rect ssRect = canvasRect;
        int ssLevel = graphics_get_supersample_level();
        ssRect.w *= ssLevel;
        ssRect.h *= ssLevel;

        // render internal canvas texture
        SDL_RenderCopy(gFrameRenderer, gInternalRenderTarget, &ssRect, &windowRect);

        if (s_should_present_imgui)
        {
            if (auto* pData = ImGui::GetDrawData(); pData != nullptr)
                ImGui_ImplSDLRenderer2_RenderDrawData(pData, gFrameRenderer);
            s_should_present_imgui = false;
        }

        // Save screenshot before presenting per SDL_RenderReadPixels docs.
        if (!screenshotPath.empty())
        {
            int ret;
            fs::create_directories(screenshotPath.parent_path());
            SDL_Surface* sshot = SDL_CreateRGBSurface(0, windowRect.w, windowRect.h, 32, 0, 0, 0, 0);
            if (sshot == nullptr)
            {
                LOG_ERROR << "[SDL2] SDL_CreateRGBSurface error: " << SDL_GetError();
            }
            ret = SDL_RenderReadPixels(gFrameRenderer, nullptr, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
            if (ret < 0)
            {
                LOG_ERROR << "[SDL2] SDL_RenderReadPixels error: " << SDL_GetError();
            }
            ret = IMG_SavePNG(sshot, lunaticvibes::cs(screenshotPath.u8string()));
            if (ret < 0)
            {
                LOG_ERROR << "[SDL2] IMG_SavePNG error: " << IMG_GetError();
            }
            SDL_FreeSurface(sshot);
            screenshotPath.clear();
        }
        SDL_RenderPresent(gFrameRenderer);
    }
    SDL_SetRenderTarget(gFrameRenderer, gInternalRenderTarget);

    if (maxFPS != 0)
    {
        long long sleep_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(frameTimestampPrev + desiredFrameTimeBetweenFrames -
                                                                 std::chrono::high_resolution_clock::now())
                .count();
        preciseSleep(sleep_ns);
    }
    frameTimestampPrev = std::chrono::high_resolution_clock::now();
}

int graphics_free()
{
    LOG_INFO << "Shutting down ImGui module...";
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    LOG_INFO << "Releasing SDL resources...";
    SDL_DestroyTexture(gInternalRenderTarget);
    gInternalRenderTarget = nullptr;
    SDL_DestroyRenderer(gFrameRenderer);
    gFrameRenderer = nullptr;
    SDL_DestroyWindow(gFrameWindow);
    gFrameWindow = nullptr;

    LOG_INFO << "[SDL2] De-initializing TTF module...";
    TTF_Quit();
    LOG_INFO << "[SDL2] De-initializing Image module...";
    IMG_Quit();
    LOG_INFO << "[SDL2] De-initializing SDL2...";
    SDL_Quit();

    LOG_INFO << "Graphics shutdown complete.";

    return 0;
}

void graphics_copy_screen_texture(Texture& texture)
{
    LVF_DEBUG_ASSERT(IsMainThread());

    SDL_SetRenderTarget(gFrameRenderer, reinterpret_cast<SDL_Texture*>(texture.raw()));
    SDL_RenderClear(gFrameRenderer);
    const SDL_Rect origRect = texture.getRect();
    const SDL_Rect rect{0, 0, origRect.w * graphics_get_supersample_level(),
                        origRect.h * graphics_get_supersample_level()};
    SDL_RenderCopy(gFrameRenderer, gInternalRenderTarget, &rect, &canvasRect);
    SDL_SetRenderTarget(gFrameRenderer, gInternalRenderTarget);
}

int graphics_get_monitor_index()
{
    return SDL_GetWindowDisplayIndex(gFrameWindow);
}

std::pair<int, int> graphics_get_desktop_resolution()
{
    int index = graphics_get_monitor_index();
    SDL_DisplayMode mode{};
    SDL_GetDesktopDisplayMode(index, &mode);
    return {mode.w, mode.h};
}

std::vector<std::tuple<int, int, int>> graphics_get_resolution_list()
{
    int index = graphics_get_monitor_index();
    int modes = SDL_GetNumDisplayModes(index);
    std::vector<std::tuple<int, int, int>> res;
    for (int i = 0; i < modes; ++i)
    {
        SDL_DisplayMode mode{};
        SDL_GetDisplayMode(index, i, &mode);
        LOG_DEBUG << mode.w << 'x' << mode.h << ' ' << mode.refresh_rate << "Hz";
        res.emplace_back(mode.w, mode.h, mode.refresh_rate);
    }
    return res;
}

void graphics_change_window_mode(int mode)
{
    LOG_WARNING << "Setting window mode to " << mode;
    switch (mode)
    {
    case 0:
        SDL_SetWindowFullscreen(gFrameWindow, 0);
        SDL_SetWindowBordered(gFrameWindow, SDL_TRUE);
        break;
    case 1: SDL_SetWindowFullscreen(gFrameWindow, SDL_WINDOW_FULLSCREEN); break;
    case 2:
        SDL_SetWindowFullscreen(gFrameWindow, 0);
        SDL_SetWindowBordered(gFrameWindow, SDL_FALSE);
        break;
    case 3:
        SDL_SetWindowFullscreen(gFrameWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_SetWindowBordered(gFrameWindow, SDL_FALSE);
        break;
    }
}

void graphics_resize_window(int x, int y)
{
    LOG_WARNING << "Resizing window mode to " << x << 'x' << y;
    lunaticvibes::graphics::save_new_window_size(x, y);
    SDL_SetWindowSize(gFrameWindow, windowRect.w, windowRect.h);
    SDL_SetWindowPosition(gFrameWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void lunaticvibes::graphics::save_new_window_size(int width, int height)
{
    LOG_DEBUG << "saving new window size " << width << 'x' << height;
    windowRect.w = width;
    windowRect.h = height;
    canvasScaleX = (double)width / canvasRect.w;
    canvasScaleY = (double)height / canvasRect.h;
    ConfigMgr::set('V', cfg::V_DISPLAY_RES_X, width);
    ConfigMgr::set('V', cfg::V_DISPLAY_RES_Y, height);
}

void graphics_change_vsync(int mode)
{
    LOG_WARNING << "Setting vsync mode to " << mode;
    SDL_RenderSetVSync(gFrameRenderer, mode);
}

static int superSampleLevel = 1;
void graphics_set_supersample_level(int level)
{
    LOG_WARNING << "Setting supersample level to " << level;
    // assert(canvasRect.w * level <= 3840);
    superSampleLevel = level;
}
int graphics_get_supersample_level()
{
    return superSampleLevel;
}

void graphics_resize_canvas(int x, int y)
{
    LOG_WARNING << "Resizing canvas to " << x << 'x' << y;
    canvasRect.w = x;
    canvasRect.h = y;
    canvasScaleX = (double)windowRect.w / x;
    canvasScaleY = (double)windowRect.h / y;
}
double graphics_get_canvas_scale_x()
{
    return canvasScaleX;
}
double graphics_get_canvas_scale_y()
{
    return canvasScaleY;
}

void graphics_set_maxfps(int fps)
{
    LOG_WARNING << "Setting max fps to " << fps;
    maxFPS = fps;
    if (maxFPS != 0)
    {
        desiredFrameTimeBetweenFrames = std::chrono::nanoseconds(static_cast<long long>(std::round(1e9 / maxFPS)));
    }
}

///////////////////////////////////////////////////////////////////////////

static bool isEditing = false;
static std::string textBuf, textBufSuffix;
static void funEditing(const SDL_TextEditingEvent& e);
static void funInput(const SDL_TextInputEvent& e);
static void funKeyDown(const SDL_KeyboardEvent& e);

bool lunaticvibes::event_handle()
{
    auto i16_from_i32 = [](std::int32_t value) {
        return static_cast<std::int16_t>(std::clamp(value, SHRT_MIN, SHRT_MAX));
    };

    bool quit = false;
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch (e.type)
        {
        case SDL_QUIT:
            LOG_WARNING << "[Event] SDL_QUIT";
            quit = true;
            break;

        case SDL_WINDOWEVENT:
            switch (e.window.event)
            {
            case SDL_WINDOWEVENT_FOCUS_GAINED: SetWindowForeground(true); break;
            case SDL_WINDOWEVENT_FOCUS_LOST: SetWindowForeground(false); break;
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED: {
                const auto height = static_cast<unsigned>(e.window.data2);
                const auto width = static_cast<unsigned>(e.window.data1);
                lunaticvibes::graphics::save_new_window_size(width, height);
                break;
            }
            default: break;
            }
            break;

        case SDL_TEXTINPUT:
            if (isEditing)
                funInput(e.text);
            break;

        case SDL_TEXTEDITING:
            if (isEditing)
                funEditing(e.edit);
            break;

        case SDL_KEYDOWN: {
            std::unique_lock l{sdl::state::g_input_mutex};
            sdl::state::g_keyboard_scancodes[e.key.keysym.scancode] = true;
            if (isEditing)
                funKeyDown(e.key);
            break;
        }

        case SDL_KEYUP: {
            std::unique_lock l{sdl::state::g_input_mutex};
            sdl::state::g_keyboard_scancodes[e.key.keysym.scancode] = false;
            break;
        }

        case SDL_MOUSEMOTION: {
            std::unique_lock l{sdl::state::g_input_mutex};
            sdl::state::g_mouse_x = e.motion.x;
            sdl::state::g_mouse_y = e.motion.y;
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            std::unique_lock l{sdl::state::g_input_mutex};
            sdl::state::g_mouse_buttons[e.button.button] = true;
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            std::unique_lock l{sdl::state::g_input_mutex};
            sdl::state::g_mouse_buttons[e.button.button] = false;
            break;
        }

        case SDL_MOUSEWHEEL: {
            std::unique_lock l{sdl::state::g_input_mutex};
            sdl::state::g_mouse_wheel_delta = i16_from_i32(e.wheel.y);
            break;
        }

        case SDL_CONTROLLERAXISMOTION: {
            std::unique_lock l{sdl::state::g_input_mutex};
            LVF_DEBUG_ASSERT(e.caxis.which < static_cast<int>(sdl::state::g_con_axes.size()));
            auto& axes = sdl::state::g_con_axes[e.caxis.which];
            if (e.caxis.axis >= axes.size())
            {
                LOG_WARNING << "[SDL2] Controller axis is out of range";
                break;
            }
            axes[e.caxis.axis] = e.caxis.value;
            break;
        }

        case SDL_CONTROLLERBUTTONDOWN: {
            std::unique_lock l{sdl::state::g_input_mutex};
            LVF_DEBUG_ASSERT(e.cbutton.which < static_cast<int>(sdl::state::g_con_buttons.size()));
            auto& buttons = sdl::state::g_con_buttons[e.cbutton.which];
            if (e.cbutton.button >= buttons.size())
            {
                LOG_WARNING << "[SDL2] Controller button is out of range";
                break;
            }
            buttons[e.cbutton.button] = true;
            break;
        }

        case SDL_CONTROLLERBUTTONUP: {
            std::unique_lock l{sdl::state::g_input_mutex};
            LVF_DEBUG_ASSERT(e.cbutton.which < static_cast<int>(sdl::state::g_con_buttons.size()));
            auto& buttons = sdl::state::g_con_buttons[e.cbutton.which];
            if (e.cbutton.button >= buttons.size())
            {
                LOG_WARNING << "[SDL2] Controller button is out of range";
                break;
            }
            buttons[e.cbutton.button] = false;
            break;
        }

        default: break;
        }
    }
    return quit;
}

void ImGuiNewFrame()
{
    SDL_SetRenderTarget(gFrameRenderer, nullptr);
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    SDL_SetRenderTarget(gFrameRenderer, gInternalRenderTarget);
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    s_should_present_imgui = true;
}

static std::function<void(const std::string&)> funUpdateText;
void startTextInput(const RectF& textBox, const std::string& oldText,
                    std::function<void(const std::string&)> funUpdateText)
{
    LOG_DEBUG << "Start Text Input";

    textBuf = oldText;
    textBuf.reserve(32);

    ::funUpdateText = std::move(funUpdateText);

    SDL_Rect r;
    const RectF scaledTextBox{
        static_cast<float>(textBox.x * canvasScaleX),
        static_cast<float>(textBox.y * canvasScaleY),
        static_cast<float>(textBox.w * canvasScaleX),
        static_cast<float>(textBox.h * canvasScaleY),
    };
    r.x = static_cast<int>(std::floor(scaledTextBox.x));
    r.y = static_cast<int>(std::floor(scaledTextBox.y));
    r.w = static_cast<int>(std::ceil(scaledTextBox.w));
    r.h = static_cast<int>(std::ceil(scaledTextBox.h));
    SDL_SetTextInputRect(&r);
    // Do *not* start and stop text inputs. They are enabled by default in SDL2.
    // https://github.com/ocornut/imgui/issues/6306
    // TODO(SDL3): uncomment this, it won't longer be enabled by default.
    // SDL_StartTextInput();
    isEditing = true;

    ::funUpdateText(textBuf);
}

void stopTextInput()
{
    LOG_DEBUG << "Stop Text Input";

    isEditing = false;
    funUpdateText = [](const std::string&) {};
    // See the comment for SDL_StartTextInput above.
    // TODO(SDL3): uncomment this.
    // SDL_StopTextInput();
}

void funEditing(const SDL_TextEditingEvent& e)
{
    LOG_DEBUG << "Editing " << e.start << " " << e.length << " " << e.text;
    if (e.length == 0)
    {
        textBuf += textBufSuffix;
        textBufSuffix.clear();
        funUpdateText(textBuf);
    }
    else
    {
        funUpdateText(textBuf + e.text + textBufSuffix);
    }
}

void funInput(const SDL_TextInputEvent& e)
{
    textBuf = textBuf + e.text + textBufSuffix;
    textBufSuffix.clear();
    funUpdateText(textBuf);
}

// \param c First byte of UTF-8 character (which may be up to 4 bytes)
static int get_utf8_char_size(char c)
{
    if ((c & 0x80) == 0)
        return 1;
    if ((c & 0xE0) == 0xC0)
        return 2;
    if ((c & 0xF0) == 0xE0)
        return 3;
    if ((c & 0xF8) == 0xF0)
        return 4;
    return -1;
}

static void remove_last_utf8_char(std::string& str)
{
    if (str.empty())
        return;
    const auto s_size = static_cast<int>(str.size());
    for (const int size :
         str | v::reverse | v::transform(get_utf8_char_size) | v::filter([](int size) { return size > 0; }))
    {
        str.resize(s_size < size ? 0 : s_size - size);
        break;
    }
}

void funKeyDown(const SDL_KeyboardEvent& e)
{
    if (e.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        switch (e.keysym.sym)
        {
        case SDLK_v: {
            std::stringstream ss;
            char* pText = SDL_GetClipboardText();
            ss << pText;
            SDL_free(pText);
            std::string textLine;
            std::getline(ss, textLine);
            textBuf += textLine;
            funUpdateText(textBuf + textBufSuffix);
            break;
        }
        }
    }
    else
    {
        switch (e.keysym.sym)
        {
        case SDLK_BACKSPACE:
            if (textBufSuffix.empty() && !textBuf.empty())
            {
                remove_last_utf8_char(textBuf);
                funUpdateText(textBuf + textBufSuffix);
            }
            break;
        }
    }
}

#endif

void lunaticvibes::graphics::queue_screenshot(Path png)
{
    LOG_INFO << "Screenshot: " << png;
    screenshotPath = std::move(png);
}
