#include "skin_lr2_number.h"

#include <game/ruleset/ruleset_bms.h>
#include <game/runtime/index/number.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>

#include <cmath>

int lunaticvibes::get_number(IndexNumber ind)
{
    switch (ind)
    {
    case IndexNumber::PLAY_1P_SCORE:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); ruleset)
            return static_cast<int>(std::round(ruleset->getMoneyScoreAnimation().animate({})));
        return 0;
    case IndexNumber::PLAY_2P_SCORE:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_TARGET]); ruleset)
            return static_cast<int>(std::round(ruleset->getMoneyScoreAnimation().animate({})));
        return 0;
    case IndexNumber::PLAY_1P_GROOVEGAUGE:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); ruleset)
            return static_cast<int>(ruleset->getHealthAnimation().animate({}) * 100);
        break; // might still also be in State::get
    case IndexNumber::PLAY_1P_GROOVEGAUGE_AFTER_DOT:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); ruleset)
            return static_cast<int>(ruleset->getHealthAnimation().animate({}) * 100'00) % 100;
        break; // might still also be in State::get
    case IndexNumber::PLAY_2P_GROOVEGAUGE:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_TARGET]); ruleset)
            return static_cast<int>(ruleset->getHealthAnimation().animate({}) * 100);
        break; // might still also be in State::get
    default: break;
    }
    return State::get(ind);
}
