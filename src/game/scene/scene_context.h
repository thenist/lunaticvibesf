#pragma once

#include <game/scene/scene.h>
#include <game/scene/scene_context_chart.h>
#include <game/scene/scene_context_customize.h>
#include <game/scene/scene_context_keyconfig.h>
#include <game/scene/scene_context_overlay.h>
#include <game/scene/scene_context_play.h>
#include <game/scene/scene_context_select.h>

#include <atomic>
#include <memory>

class ScoreDB;
class SongDB;

inline std::atomic<bool> gAppIsExiting;
inline std::atomic<bool> gQuitOnFinish;

inline SceneType gNextScene = SceneType::SELECT;
inline bool gCustomizeSceneChanged = false;
inline bool gExitingCustomize = false;
inline bool gInCustomize = false;

inline ChartContextParams gChartContext;
inline CustomizeContextParams gCustomizeContext;
inline KeyConfigContextParams gKeyconfigContext;
inline OverlayContextParams gOverlayContext;
inline PlayContextParams gPlayContext;
inline SelectContextParams gSelectContext;

inline std::shared_ptr<ScoreDB> g_pScoreDB;
inline std::shared_ptr<SongDB> g_pSongDB;
