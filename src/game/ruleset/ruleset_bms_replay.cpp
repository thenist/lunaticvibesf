#include "ruleset_bms_replay.h"

#include <iterator>
#include <utility>

#include <common/assert.h>
#include <common/play_modifiers.h>
#include <game/input/input_wrapper.h>

RulesetBMSReplay::RulesetBMSReplay(std::shared_ptr<ChartFormatBase> format_, std::shared_ptr<ChartObjectBase> chart_,
                                   std::shared_ptr<ReplayChart> replay_, const PlayModifiers mods, GameModeKeys keys,
                                   JudgeDifficulty difficulty, double health, PlaySide side, const int fiveKeyMapIndex,
                                   const double pitchSpeed)
    : RulesetBase(format_, chart_),
      RulesetBMS(std::move(format_), std::move(chart_), mods, keys, difficulty, health, side, fiveKeyMapIndex, nullptr),
      replay(std::move(replay_))
{
    LVF_DEBUG_ASSERT(side == PlaySide::AUTO || side == PlaySide::AUTO_DOUBLE || side == PlaySide::AUTO_2P ||
                     side == PlaySide::RIVAL || side == PlaySide::MYBEST);

    if (fiveKeyMapIndex == -1)
    {
        _inputDownMap = &REPLAY_CMD_INPUT_DOWN_MAP;
        _inputUpMap = &REPLAY_CMD_INPUT_UP_MAP;
    }
    else
    {
        LVF_DEBUG_ASSERT(fiveKeyMapIndex >= 0);
        LVF_DEBUG_ASSERT(fiveKeyMapIndex < static_cast<int>(std::size(REPLAY_CMD_INPUT_DOWN_MAP_5K)));
        LVF_DEBUG_ASSERT(fiveKeyMapIndex < static_cast<int>(std::size(REPLAY_CMD_INPUT_UP_MAP_5K)));
        _inputDownMap = &REPLAY_CMD_INPUT_DOWN_MAP_5K[fiveKeyMapIndex];
        _inputUpMap = &REPLAY_CMD_INPUT_UP_MAP_5K[fiveKeyMapIndex];
    }

    itReplayCommand = replay->commands.begin();
    showJudge = (_side == PlaySide::AUTO || _side == PlaySide::AUTO_DOUBLE || _side == PlaySide::AUTO_2P);

    if (replay->pitchValue != 0)
    {
        static const double tick = std::pow(2, 1.0 / 12);
        replayTimestampMultiplier = std::pow(tick, replay->pitchValue) / pitchSpeed;
    }
}

void RulesetBMSReplay::update(const lunaticvibes::Time& t)
{
    isSkippingToEnd = t.hres() == LLONG_MAX;
    showJudge = showJudge && !isSkippingToEnd;

    if (!_hasStartTime)
        setStartTime(isSkippingToEnd ? lunaticvibes::Time(0) : t);

    const auto rt = t - _startTime.norm();
    _basic.play_time = rt;

    InputMask prevKeyPressing = keyPressing;
    lunaticvibes::Time prevReplayT{0};
    auto updateInput = [this, &prevKeyPressing, &prevReplayT]() {
        // NOTE: _chart takes rt, but in this case t == rt as _startTime = 0.
        // NOTE: important to call this periodically, or after some time notes will not be judged.
        if (isSkippingToEnd)
            _chart->update(prevReplayT);
        InputMask pressed = keyPressing & ~prevKeyPressing;
        lunaticvibes::InputMaskTimes tt;
        tt.fill(prevReplayT);
        InputMask released = ~keyPressing & prevKeyPressing;
        if (pressed.any())
            RulesetBMS::updatePress(pressed, prevReplayT, tt);
        if (keyPressing.any())
            RulesetBMS::updateHold(keyPressing, prevReplayT);
        if (released.any())
            RulesetBMS::updateRelease(released, prevReplayT);
        // NOTE: important to call this often or unpressed notes will be judged incorrectly.
        RulesetBMS::update(prevReplayT);
        prevKeyPressing = keyPressing;
    };
    while (itReplayCommand != replay->commands.end())
    {
        const lunaticvibes::Time replayT{
            _startTime.norm() + static_cast<long long>(std::round(itReplayCommand->ms * replayTimestampMultiplier)),
            false};
        if (t < replayT)
            break;

        if (prevReplayT != 0 && replayT != prevReplayT)
            updateInput();

        auto cmd = itReplayCommand->type;
        if (_side == PlaySide::AUTO_2P)
            cmd = ReplayChart::Commands::leftSideCmdToRightSide(cmd);

        if (auto it = _inputDownMap->find(cmd); it != _inputDownMap->end())
        {
            keyPressing[it->second] = true;
        }
        if (auto it = _inputUpMap->find(cmd); it != _inputUpMap->end())
        {
            keyPressing[it->second] = false;
        }
        else
            switch (cmd)
            {
            case ReplayChart::Commands::Type::S1A_PLUS: playerScratchAccumulator[PLAYER_SLOT_PLAYER] = 0.0015; break;
            case ReplayChart::Commands::Type::S1A_MINUS: playerScratchAccumulator[PLAYER_SLOT_PLAYER] = -0.0015; break;
            case ReplayChart::Commands::Type::S1A_STOP: playerScratchAccumulator[PLAYER_SLOT_PLAYER] = 0; break;
            case ReplayChart::Commands::Type::S2A_PLUS: playerScratchAccumulator[PLAYER_SLOT_TARGET] = 0.0015; break;
            case ReplayChart::Commands::Type::S2A_MINUS: playerScratchAccumulator[PLAYER_SLOT_TARGET] = -0.0015; break;
            case ReplayChart::Commands::Type::S2A_STOP: playerScratchAccumulator[PLAYER_SLOT_TARGET] = 0; break;
            default: break;
            }

        itReplayCommand++;
        prevReplayT = replayT;
    }
    prevReplayT = t;
    updateInput();
}

void RulesetBMSReplay::fail()
{
    _startTime = lunaticvibes::Time(0);
    _hasStartTime = true;
    RulesetBMS::fail();
}

void RulesetBMSReplay::updateGlobals()
{
    if (!isSkippingToEnd)
        RulesetBMS::updateGlobals();
}
