#include "scene_mgr.h"

#include <common/assert.h>
#include <game/scene/scene.h>
#include <game/scene/scene_course_result.h>
#include <game/scene/scene_customize.h>
#include <game/scene/scene_decide.h>
#include <game/scene/scene_exit_trans.h>
#include <game/scene/scene_keyconfig.h>
#include <game/scene/scene_play.h>
#include <game/scene/scene_play_course_trans.h>
#include <game/scene/scene_play_retry_trans.h>
#include <game/scene/scene_pre_select.h>
#include <game/scene/scene_result.h>
#include <game/scene/scene_select.h>
#include <game/skin/skin_mgr.h>

#include <memory>
#include <optional>

std::shared_ptr<SceneBase> lunaticvibes::buildScene(const std::shared_ptr<SkinMgr>& skinMgr, SceneType type)
{
    switch (type)
    {
    case SceneType::EXIT:
    case SceneType::NOT_INIT: break;
    case SceneType::PRE_SELECT: return std::make_shared<ScenePreSelect>();
    case SceneType::SELECT: return std::make_shared<SceneSelect>(skinMgr);
    case SceneType::DECIDE: return std::make_shared<SceneDecide>(skinMgr);
    case SceneType::PLAY: return std::make_shared<ScenePlay>(skinMgr);
    case SceneType::RETRY_TRANS: return std::make_shared<ScenePlayRetryTrans>();
    case SceneType::RESULT: return std::make_shared<SceneResult>(skinMgr);
    case SceneType::COURSE_TRANS: return std::make_shared<ScenePlayCourseTrans>();
    case SceneType::KEYCONFIG: return std::make_shared<SceneKeyConfig>(skinMgr);
    case SceneType::CUSTOMIZE: return std::make_shared<SceneCustomize>(skinMgr, std::nullopt);
    case SceneType::COURSE_RESULT: return std::make_shared<SceneCourseResult>(skinMgr);
    case SceneType::EXIT_TRANS: return std::make_shared<SceneExitTrans>();
    }
    lunaticvibes::assert_failed("lunaticvibes::buildScene");
}
