#include "soundset.h"

#include <common/utils.h>
#include <config/cfg_general.h>
#include <config/config_mgr.h>

Path vSoundSet::getPathBGMSelect() const
{
    return convertLR2Path(ConfigMgr::General()->get(cfg::E_LR2PATH, "."), "LR2files/Bgm/LR2 ver sta/select.wav");
}

Path vSoundSet::getPathBGMDecide() const
{
    return convertLR2Path(ConfigMgr::General()->get(cfg::E_LR2PATH, "."), "LR2files/Bgm/LR2 ver sta/decide.wav");
}
