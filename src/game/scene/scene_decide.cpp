#include "scene_decide.h"

#include <common/log.h>
#include <game/arena/arena_data.h>
#include <game/runtime/index/timer.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>
#include <game/sound/sound_mgr.h>
#include <game/sound/sound_sample.h>

#include <functional>

SceneDecide::SceneDecide(const std::shared_ptr<SkinMgr>& skinMgr) : SceneBase(skinMgr, SkinType::DECIDE, 1000)
{
    _type = SceneType::DECIDE;

    _inputAvailable = INPUT_MASK_FUNC;
    _inputAvailable |= INPUT_MASK_1P | INPUT_MASK_2P;

    _input.register_p("SCENE_PRESS", std::bind_front(&SceneDecide::inputGamePress, this));
    _input.register_h("SCENE_HOLD", std::bind_front(&SceneDecide::inputGameHold, this));
    _input.register_r("SCENE_RELEASE", std::bind_front(&SceneDecide::inputGameRelease, this));

    state = eDecideState::START;

    if (!gInCustomize)
    {
        SoundMgr::stopSysSamples();
        SoundMgr::setSysVolume(1.0);
        SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::BGM_DECIDE);
    }
}

SceneDecide::~SceneDecide()
{
    _input.loopEnd();
}

////////////////////////////////////////////////////////////////////////////////

void SceneDecide::update_fixed(const lunaticvibes::Time& t)
{
    if (gNextScene != SceneType::DECIDE)
        return;

    if (gAppIsExiting)
    {
        gNextScene = SceneType::EXIT_TRANS;
    }

    switch (state)
    {
    case eDecideState::START: updateStart(t); break;
    case eDecideState::SKIP: updateSkip(t); break;
    case eDecideState::CANCEL: updateCancel(t); break;
    }
}

void SceneDecide::updateStart(const lunaticvibes::Time& t)
{
    const auto rt = t - State::get(IndexTimer::SCENE_START);

    if (!gInCustomize && rt.norm() >= pSkin->info.timeDecideExpiry)
    {
        gNextScene = SceneType::PLAY;
    }
}

void SceneDecide::updateSkip(const lunaticvibes::Time& t)
{
    const auto ft = t - State::get(IndexTimer::FADEOUT_BEGIN);

    if (ft.norm() >= pSkin->info.timeOutro)
    {
        gNextScene = SceneType::PLAY;
    }
}

void SceneDecide::updateCancel(const lunaticvibes::Time& t)
{
    const auto ft = t - State::get(IndexTimer::FADEOUT_BEGIN);

    if (ft.norm() >= pSkin->info.timeOutro)
    {
        clearContextPlay();
        gPlayContext.isAuto = false;
        gPlayContext.isReplay = false;
        gNextScene = SceneType::SELECT;
    }
}

////////////////////////////////////////////////////////////////////////////////

// CALLBACK
void SceneDecide::inputGamePress(InputMask& m, const lunaticvibes::Time& t)
{
    const unsigned rt = (t - State::get(IndexTimer::SCENE_START)).norm();
    if (rt < pSkin->info.timeIntro)
        return;

    const auto k = _inputAvailable & m;
    if ((k & INPUT_MASK_DECIDE).any() && rt >= pSkin->info.timeDecideSkip)
    {
        switch (state)
        {
        case eDecideState::START:
            State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
            state = eDecideState::SKIP;
            LOG_DEBUG << "[Decide] State changed to SKIP";
            break;
        case eDecideState::SKIP:
        case eDecideState::CANCEL: break;
        }
    }

    if (!gArenaData.isOnline())
    {
        if (k[Input::ESC])
        {
            switch (state)
            {
            case eDecideState::START:
                State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
                SoundMgr::stopSysSamples();
                state = eDecideState::CANCEL;
                LOG_DEBUG << "[Decide] State changed to CANCEL";
                break;
            case eDecideState::SKIP:
            case eDecideState::CANCEL: break;
            }
        }
    }
}

// CALLBACK
void SceneDecide::inputGameHold(InputMask& m, const lunaticvibes::Time& t)
{
    const unsigned rt = (t - State::get(IndexTimer::SCENE_START)).norm();
    if (rt < pSkin->info.timeIntro)
        return;

    const auto k = _inputAvailable & m;

    if (!gArenaData.isOnline())
    {
        if ((k[Input::K1START] && k[Input::K1SELECT]) || (k[Input::K2START] && k[Input::K2SELECT]))
        {
            switch (state)
            {
            case eDecideState::START:
                State::set(IndexTimer::FADEOUT_BEGIN, t.norm());
                SoundMgr::stopSysSamples();
                state = eDecideState::CANCEL;
                LOG_DEBUG << "[Decide] State changed to CANCEL";
                break;
            case eDecideState::SKIP:
            case eDecideState::CANCEL: break;
            }
        }
    }
}

// CALLBACK
void SceneDecide::inputGameRelease(InputMask& m, const lunaticvibes::Time& t)
{
    const unsigned rt = (t - State::get(IndexTimer::SCENE_START)).norm();
    if (rt < pSkin->info.timeIntro)
        return;
}
