#pragma once

// Don't include graphics platform here. It leaks a lot of details.
// TODO: actually don't do that, lol.
#include <game/graphics/SDL2/graphics_SDL2.h>
#include <game/graphics/SDL2/image_SDL2.h>
#include <game/graphics/SDL2/ttf_SDL2.h>

#include <common/types.h>
#include <game/graphics/blend_mode.h> // IWYU pragma: export
#include <game/graphics/color.h>      // IWYU pragma: export
#include <game/graphics/rect.h>       // IWYU pragma: export
#include <game/graphics/rectf.h>      // IWYU pragma: export

#include <functional>
#include <utility>
#include <vector>

namespace lunaticvibes
{
// TODO: enum class
enum GRAPHICS_WINDOW_MODE : int
{
    GRAPHICS_WINDOW_MODE_WINDOWED = 0,
    GRAPHICS_WINDOW_MODE_FULLSCREEN = 1,
    GRAPHICS_WINDOW_MODE_BORDERLESS = 2,
};
// TODO: enum class
enum GRAPHICS_VSYNC_MODE : int
{
    GRAPHICS_VSYNC_MODE_OFF = 0,
    GRAPHICS_VSYNC_MODE_VSYNC = 1,
    GRAPHICS_VSYNC_MODE_ADAPTIVE = 2,
};
} // namespace lunaticvibes

namespace lunaticvibes::window
{

int graphics_init();
void graphics_clear();
void graphics_flush();
int graphics_free();

void graphics_copy_screen_texture(Texture& texture);

// w, h
std::pair<int, int> graphics_get_desktop_resolution();
// w, h
std::vector<std::pair<int, int>> graphics_get_resolution_list();
void graphics_change_window_mode(lunaticvibes::GRAPHICS_WINDOW_MODE mode);
void graphics_resize_window(int x, int y);

// save passed windows size as the new one, without applying it (for that refer to graphics_resize_window)
void save_new_window_size(int x, int y);

void graphics_change_vsync(lunaticvibes::GRAPHICS_VSYNC_MODE mode);

// scaling functions
void graphics_set_supersample_level(int scale);
int graphics_get_supersample_level();

void graphics_resize_canvas(int x, int y);
double graphics_get_canvas_scale_x();
double graphics_get_canvas_scale_y();

void graphics_set_maxfps(int fps);

// text input support
void startTextInput(const RectF& textBox, const std::string& oldText,
                    std::function<void(const std::string&)> funUpdateText);
void stopTextInput();

void ImGuiNewFrame();

std::pair<int, int> get_mouse_pos();
void queue_screenshot(Path png);

// Returns true if quit event received.
bool event_handle();

} // namespace lunaticvibes::window

using lunaticvibes::window::graphics_change_vsync;
using lunaticvibes::window::graphics_change_window_mode;
using lunaticvibes::window::graphics_clear;
using lunaticvibes::window::graphics_copy_screen_texture;
using lunaticvibes::window::graphics_flush;
using lunaticvibes::window::graphics_free;
using lunaticvibes::window::graphics_get_canvas_scale_x;
using lunaticvibes::window::graphics_get_canvas_scale_y;
using lunaticvibes::window::graphics_get_desktop_resolution;
using lunaticvibes::window::graphics_get_resolution_list;
using lunaticvibes::window::graphics_get_supersample_level;
using lunaticvibes::window::graphics_init;
using lunaticvibes::window::graphics_resize_canvas;
using lunaticvibes::window::graphics_resize_window;
using lunaticvibes::window::graphics_set_maxfps;
using lunaticvibes::window::graphics_set_supersample_level;
using lunaticvibes::window::ImGuiNewFrame;
using lunaticvibes::window::startTextInput;
using lunaticvibes::window::stopTextInput;
