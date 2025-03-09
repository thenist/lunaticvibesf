#include "skin_lr2_slider_callbacks.h"

#include <common/assert.h>
#include <config/cfg_input.h>
#include <config/config_mgr.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>
#include <game/scene/scene_context_customize.h>
#include <game/sound/sound_mgr.h>

namespace lr2skin::slider
{

#pragma region slider type callbacks

static void select_pos(double p)
{
    if (gSelectContext.entries.empty())
        return;
    // TODO: allow smooth dragging. It currently snaps to closest possible value.
    auto idx_new = static_cast<size_t>(std::floor(p * gSelectContext.entries.size()));
    if (idx_new != gSelectContext.selectedEntryIndex)
    {
        State::set(IndexSlider::SELECT_LIST, static_cast<double>(idx_new) / gSelectContext.entries.size());
        gSelectContext.draggingListSlider = true;
    }
}

static void customize_scrollbar(double p)
{
    State::set(IndexSlider::SKIN_CONFIG_OPTIONS, p);
    gCustomizeContext.messages.emplace(lunaticvibes::customize_message::OptionDrag{});
}

static void ir_ranking_scrollbar(double p)
{
    // no ir support right now
}

static void eq(int idx, double p)
{
    int val = static_cast<int>(p * 24) - 12;
    p = (val + 12) / 24.0;
    State::set(IndexSlider(idx + static_cast<int>(IndexSlider::EQ0)), p);
    State::set(IndexNumber(idx + static_cast<int>(IndexNumber::EQ0)), val);

    if (State::get(IndexSwitch::SOUND_EQ))
    {
        SoundMgr::setEQ(static_cast<EQFreq>(idx), val);
    }
}

static void master_volume(double p)
{
    State::set(IndexSlider::VOLUME_MASTER, p);
    State::set(IndexNumber::VOLUME_MASTER, static_cast<int>(std::round(p * 100)));

    if (State::get(IndexSwitch::SOUND_VOLUME))
        SoundMgr::setVolume(SampleChannel::MASTER, static_cast<float>(p));
}

static void key_volume(double p)
{
    State::set(IndexSlider::VOLUME_KEY, p);
    State::set(IndexNumber::VOLUME_KEY, static_cast<int>(std::round(p * 100)));

    if (State::get(IndexSwitch::SOUND_VOLUME))
        SoundMgr::setVolume(SampleChannel::KEY, static_cast<float>(p));
}

static void bgm_volume(double p)
{
    State::set(IndexSlider::VOLUME_BGM, p);
    State::set(IndexNumber::VOLUME_BGM, static_cast<int>(std::round(p * 100)));

    if (State::get(IndexSwitch::SOUND_VOLUME))
        SoundMgr::setVolume(SampleChannel::BGM, static_cast<float>(p));
}

static SampleChannel get_sample_channel_for_target(unsigned int idx)
{
    switch (idx)
    {
    case 0: return SampleChannel::MASTER; break;
    case 1: return SampleChannel::KEY; break;
    case 2: return SampleChannel::BGM; break;
    }
    lunaticvibes::assert_failed("get_sample_channel_for_target");
};

static DSPType get_dsp_type_from_sound_fx(unsigned int idx)
{
    switch (idx)
    {
    case 0: return DSPType::OFF;
    case 1: return DSPType::REVERB;
    case 2: return DSPType::DELAY;
    case 3: return DSPType::LOWPASS;
    case 4: return DSPType::HIGHPASS;
    case 5: return DSPType::FLANGER;
    case 6: return DSPType::CHORUS;
    case 7: return DSPType::DISTORTION;
    }
    lunaticvibes::assert_failed("get_dsp_type_from_sound_fx");
};

static void fx0(int idx, double p)
{
    State::set(idx == 0 ? IndexSlider::FX0_P1 : IndexSlider::FX0_P2, p);
    State::set(idx == 0 ? IndexNumber::FX0_P1 : IndexNumber::FX0_P2, static_cast<int>(std::round(p * 100)));

    float p1 = (idx == 0) ? static_cast<float>(p) : State::get(IndexSlider::FX0_P1);
    float p2 = (idx != 0) ? static_cast<float>(p) : State::get(IndexSlider::FX0_P2);

    if (State::get(IndexSwitch::SOUND_FX0))
    {
        const SampleChannel ch{get_sample_channel_for_target(State::get(IndexOption::SOUND_TARGET_FX0))};
        SoundMgr::setDSP(get_dsp_type_from_sound_fx(State::get(IndexOption::SOUND_FX0)), 0, ch, p1, p2);
    }
}

static void fx1(int idx, double p)
{
    State::set(idx == 0 ? IndexSlider::FX1_P1 : IndexSlider::FX1_P2, p);
    State::set(idx == 0 ? IndexNumber::FX1_P1 : IndexNumber::FX1_P2, static_cast<int>(std::round(p * 100)));

    float p1 = (idx == 0) ? static_cast<float>(p) : State::get(IndexSlider::FX1_P1);
    float p2 = (idx != 0) ? static_cast<float>(p) : State::get(IndexSlider::FX1_P2);

    if (State::get(IndexSwitch::SOUND_FX1))
    {
        const SampleChannel ch{get_sample_channel_for_target(State::get(IndexOption::SOUND_TARGET_FX1))};
        SoundMgr::setDSP(get_dsp_type_from_sound_fx(State::get(IndexOption::SOUND_FX1)), 0, ch, p1, p2);
    }
}

static void fx2(int idx, double p)
{
    State::set(idx == 0 ? IndexSlider::FX2_P1 : IndexSlider::FX2_P2, p);
    State::set(idx == 0 ? IndexNumber::FX2_P1 : IndexNumber::FX2_P2, static_cast<int>(std::round(p * 100)));

    float p1 = (idx == 0) ? static_cast<float>(p) : State::get(IndexSlider::FX2_P1);
    float p2 = (idx != 0) ? static_cast<float>(p) : State::get(IndexSlider::FX2_P2);

    if (State::get(IndexSwitch::SOUND_FX2))
    {
        const SampleChannel ch{get_sample_channel_for_target(State::get(IndexOption::SOUND_TARGET_FX2))};
        SoundMgr::setDSP(get_dsp_type_from_sound_fx(State::get(IndexOption::SOUND_FX2)), 0, ch, p1, p2);
    }
}

void updatePitchState(int val)
{
    if (State::get(IndexSwitch::SOUND_PITCH))
    {
        static const double tick = std::pow(2, 1.0 / 12);
        double f = std::pow(tick, val);
        switch (State::get(IndexOption::SOUND_PITCH_TYPE))
        {
        case 0: // FREQUENCY
            SoundMgr::setFreqFactor(f);
            gSelectContext.pitchSpeed = f;
            break;
        case 1: // PITCH
            SoundMgr::setFreqFactor(1.0);
            SoundMgr::setPitch(f);
            gSelectContext.pitchSpeed = 1.0;
            break;
        case 2: // SPEED (freq up, pitch down)
            SoundMgr::setFreqFactor(1.0);
            SoundMgr::setSpeed(f);
            gSelectContext.pitchSpeed = f;
            break;
        default: break;
        }
    }
}

void pitch(double p)
{
    int val = static_cast<int>(p * 24) - 12;
    double ps = (val + 12) / 24.0;
    State::set(IndexSlider::PITCH, ps);
    State::set(IndexNumber::PITCH, val);

    updatePitchState(val);

    const auto [score, lamp] = getSaveScoreType();
    State::set(IndexSwitch::CHART_CAN_SAVE_SCORE, score);
    State::set(IndexOption::CHART_SAVE_LAMP_TYPE, lamp);
}

static void deadzone(int type, double p)
{
    using namespace cfg;
    int keys = 7;
    switch (State::get(IndexOption::KEY_CONFIG_MODE))
    {
    case Option::KEYCFG_5: keys = 5; break;
    case Option::KEYCFG_7: keys = 7; break;
    case Option::KEYCFG_9: keys = 9; break;
    default: return;
    }
    switch (type)
    {
    case 31:
        State::set(IndexSlider::DEADZONE_K11, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K11, p);
        break;
    case 32:
        State::set(IndexSlider::DEADZONE_K12, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K12, p);
        break;
    case 33:
        State::set(IndexSlider::DEADZONE_K13, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K13, p);
        break;
    case 34:
        State::set(IndexSlider::DEADZONE_K14, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K14, p);
        break;
    case 35:
        State::set(IndexSlider::DEADZONE_K15, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K15, p);
        break;
    case 36:
        State::set(IndexSlider::DEADZONE_K16, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K16, p);
        break;
    case 37:
        State::set(IndexSlider::DEADZONE_K17, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K17, p);
        break;
    case 38:
        State::set(IndexSlider::DEADZONE_K18, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K18, p);
        break;
    case 39:
        State::set(IndexSlider::DEADZONE_K19, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K19, p);
        break;
    case 40:
        State::set(IndexSlider::DEADZONE_K1Start, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K1Start, p);
        break;
    case 41:
        State::set(IndexSlider::DEADZONE_K1Select, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K1Select, p);
        break;
    case 42:
        State::set(IndexSlider::DEADZONE_S1L, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_S1L, p);
        break;
    case 43:
        State::set(IndexSlider::DEADZONE_S1R, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_S1R, p);
        break;
    case 44:
        State::set(IndexSlider::SPEED_S1A, p);
        ConfigMgr::Input(keys)->set(I_INPUT_SPEED_S1A, p);
        break;
    case 51:
        State::set(IndexSlider::DEADZONE_K21, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K21, p);
        break;
    case 52:
        State::set(IndexSlider::DEADZONE_K22, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K22, p);
        break;
    case 53:
        State::set(IndexSlider::DEADZONE_K23, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K23, p);
        break;
    case 54:
        State::set(IndexSlider::DEADZONE_K24, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K24, p);
        break;
    case 55:
        State::set(IndexSlider::DEADZONE_K25, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K25, p);
        break;
    case 56:
        State::set(IndexSlider::DEADZONE_K26, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K26, p);
        break;
    case 57:
        State::set(IndexSlider::DEADZONE_K27, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K27, p);
        break;
    case 58:
        State::set(IndexSlider::DEADZONE_K28, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K28, p);
        break;
    case 59:
        State::set(IndexSlider::DEADZONE_K29, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K29, p);
        break;
    case 60:
        State::set(IndexSlider::DEADZONE_K2Start, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K2Start, p);
        break;
    case 61:
        State::set(IndexSlider::DEADZONE_K2Select, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_K2Select, p);
        break;
    case 62:
        State::set(IndexSlider::DEADZONE_S2L, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_S2L, p);
        break;
    case 63:
        State::set(IndexSlider::DEADZONE_S2R, p);
        ConfigMgr::Input(keys)->set(I_INPUT_DEADZONE_S2R, p);
        break;
    case 64:
        State::set(IndexSlider::SPEED_S2A, p);
        ConfigMgr::Input(keys)->set(I_INPUT_SPEED_S2A, p);
        break;
    }
    InputMgr::updateDeadzones(keys);
}

#pragma endregion

std::function<void(double)> getSliderCallback(int type)
{
    using namespace lr2skin::slider;
    switch (type)
    {
    case 1: return std::bind_front(select_pos);

    case 7: return std::bind_front(customize_scrollbar);

    case 8: return std::bind_front(ir_ranking_scrollbar);

    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16: return std::bind_front(eq, type - 10);

    case 17: return std::bind_front(master_volume);
    case 18: return std::bind_front(key_volume);
    case 19: return std::bind_front(bgm_volume);

    case 20: return std::bind_front(fx0, 0);
    case 21: return std::bind_front(fx0, 1);
    case 22: return std::bind_front(fx1, 0);
    case 23: return std::bind_front(fx1, 1);
    case 24: return std::bind_front(fx2, 0);
    case 25: return std::bind_front(fx2, 1);
    case 26: return std::bind_front(pitch);

    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
    case 63:
    case 64: return std::bind_front(deadzone, type);

    default: return [](double) {};
    }
}

} // namespace lr2skin::slider
