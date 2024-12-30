#pragma once

namespace lunaticvibes
{
inline bool g_enable_imgui_debug_monitor = false;
inline bool g_enable_show_clicked_sprite = false;
} // namespace lunaticvibes

inline bool imguiShowMonitorLR2DST = false;
inline bool imguiShowMonitorNumber = false;
inline bool imguiShowMonitorOption = false;
inline bool imguiShowMonitorSlider = false;
inline bool imguiShowMonitorSwitch = false;
inline bool imguiShowMonitorText = false;
inline bool imguiShowMonitorBargraph = false;
inline bool imguiShowMonitorTimer = false;
void imguiMonitorLR2DST();
void imguiMonitorNumber();
void imguiMonitorOption();
void imguiMonitorSlider();
void imguiMonitorSwitch();
void imguiMonitorText();
void imguiMonitorBargraph();
void imguiMonitorTimer();
