#pragma once

#include <game/scene/scene.h>

#include <memory>

class SkinMgr;

namespace lunaticvibes
{

std::shared_ptr<SceneBase> buildScene(const std::shared_ptr<SkinMgr>& skinMgr, SceneType);

} // namespace lunaticvibes
