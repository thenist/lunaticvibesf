#pragma once

#include "ruleset_bms.h"

#include <common/play_modifiers.h>
#include <game/scene/scene_context.h>

class RulesetBMSReplay : public RulesetBMS
{
public:
    // fiveKeyMapIndex - if not 5k, set to -1.
    RulesetBMSReplay(std::shared_ptr<ChartFormatBase> format, std::shared_ptr<ChartObjectBase> chart,
                     std::shared_ptr<ReplayChart> replay, PlayModifiers mods, GameModeKeys keys,
                     JudgeDifficulty difficulty, double health, PlaySide side, int fiveKeyMapIndex, double pitchSpeed);

public:
    double replayTimestampMultiplier = 1.0;

private:
    std::shared_ptr<ReplayChart> replay;
    std::vector<ReplayChart::Commands>::iterator itReplayCommand;
    InputMask keyPressing;
    const std::map<ReplayChart::Commands::Type, Input::Pad>* _inputDownMap;
    const std::map<ReplayChart::Commands::Type, Input::Pad>* _inputUpMap;
    bool isSkippingToEnd = false;

public:
    // Register to InputWrapper
    void updatePress(InputMask& pg, const lunaticvibes::Time& t, const lunaticvibes::InputMaskTimes& tt) override {}
    // Register to InputWrapper
    void updateHold(InputMask& hg, const lunaticvibes::Time& t) override {}
    // Register to InputWrapper
    void updateRelease(InputMask& rg, const lunaticvibes::Time& t) override {}
    // Register to InputWrapper
    void updateAxis(double s1, double s2, const lunaticvibes::Time& t) override {}
    // Called by ScenePlay
    void update(const lunaticvibes::Time& t) override;

    void fail() override;

    void updateGlobals() override;
};
