#include "scene_customize.h"

#include <common/assert.h>
#include <common/hash.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/types.h>
#include <common/u8.h>
#include <common/utils.h>
#include <config/config_mgr.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>
#include <game/skin/skin_lr2.h>
#include <game/skin/skin_mgr.h>
#include <game/sound/sound_mgr.h>
#include <game/sound/soundset_lr2.h>

#include <algorithm>
#include <fstream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace r = std::ranges;

template <class... Ts> struct overloaded : Ts...
{
    using Ts::operator()...;
};

static void loadPreviewChartIfNeeded(SkinType mode)
{
    auto previewChartKeyForMode = [](SkinType mode) -> const char* {
        switch (mode)
        {
        case SkinType::PLAY5:
        case SkinType::PLAY5_2: return cfg::P_PREVIEW_CHART_5K;
        case SkinType::PLAY7:
        case SkinType::PLAY7_2: return cfg::P_PREVIEW_CHART_7K;
        case SkinType::PLAY9:
        case SkinType::PLAY9_2: return cfg::P_PREVIEW_CHART_9K;
        case SkinType::PLAY10: return cfg::P_PREVIEW_CHART_10K;
        case SkinType::PLAY14: return cfg::P_PREVIEW_CHART_14K;
        case SkinType::EXIT:
        case SkinType::TITLE:
        case SkinType::MUSIC_SELECT:
        case SkinType::DECIDE:
        case SkinType::THEME_SELECT:
        case SkinType::SOUNDSET:
        case SkinType::KEY_CONFIG:
        case SkinType::RESULT:
        case SkinType::COURSE_RESULT:
        case SkinType::RETRY_TRANS:
        case SkinType::COURSE_TRANS:
        case SkinType::EXIT_TRANS:
        case SkinType::PRE_SELECT:
        case SkinType::TMPL:
        case SkinType::TEST:
        case SkinType::MODE_COUNT: return nullptr;
        }
        lunaticvibes::assert_failed("previewChartKeyForMode");
    };

    const char* key = previewChartKeyForMode(mode);
    if (key == nullptr)
        return;

    gChartContext.chart.reset();
    gChartContext.path.clear();

    auto path = ConfigMgr::Profile()->get(key, "");
    if (path.empty())
    {
        LOG_ERROR << "[Customize] No preview chart specified";
        return;
    }

    std::shared_ptr<ChartFormatBase> chart = ChartFormatBase::createFromFile(path, std::time(nullptr));
    if (chart == nullptr || !chart->isLoaded())
    {
        LOG_ERROR << "[Customize] Failed to load preview chart: " << path;
        return;
    }

    LOG_DEBUG << "[Customize] Loaded preview chart: " << path;

    gChartContext.started = false;
    gPlayContext.isBattle = mode == SkinType::PLAY5_2 || mode == SkinType::PLAY7_2 || mode == SkinType::PLAY9_2;
    prepareChartForPlay(std::move(chart), Option::e_battle_type::BATTLE_LOCAL);
}

[[nodiscard]] static std::vector<Path> recursiveFindFiles(const Path& p, std::string_view extension)
{
    std::vector<Path> out;
    for (const auto& entry : fs::recursive_directory_iterator{p})
        if (lunaticvibes::iequals(lunaticvibes::s(entry.path().extension().u8string()), extension))
            out.push_back(entry.path());
    return out;
}

SceneCustomize::SceneCustomize(std::shared_ptr<SkinMgr> skinMgr_, std::optional<SkinType> mode)
    : SceneBase(skinMgr_, SkinType::THEME_SELECT, 240), _skinMgr(std::move(skinMgr_)),
      _state(lunaticvibes::CustomizeState::Start)
{
    _type = SceneType::CUSTOMIZE;

    if (gInCustomize)
    {
        (void)mode; // unused.

        // Initial entry is PLAY7.
        selectedMode = SkinType::PLAY7;

        gNextScene = SceneType::PLAY;
        gPlayContext.mode = selectedMode;
        gPlayContext.isAuto = true;
        gCustomizeSceneChanged = true;

        State::set(IndexSwitch::SKINSELECT_7KEYS, true);
        State::set(IndexSwitch::SKINSELECT_5KEYS, false);
        State::set(IndexSwitch::SKINSELECT_14KEYS, false);
        State::set(IndexSwitch::SKINSELECT_10KEYS, false);
        State::set(IndexSwitch::SKINSELECT_9KEYS, false);
        State::set(IndexSwitch::SKINSELECT_SELECT, false);
        State::set(IndexSwitch::SKINSELECT_DECIDE, false);
        State::set(IndexSwitch::SKINSELECT_RESULT, false);
        State::set(IndexSwitch::SKINSELECT_KEYCONFIG, false);
        State::set(IndexSwitch::SKINSELECT_SKINSELECT, false);
        State::set(IndexSwitch::SKINSELECT_SOUNDSET, false);
        State::set(IndexSwitch::SKINSELECT_THEME, false);
        State::set(IndexSwitch::SKINSELECT_7KEYS_BATTLE, false);
        State::set(IndexSwitch::SKINSELECT_5KEYS_BATTLE, false);
        State::set(IndexSwitch::SKINSELECT_9KEYS_BATTLE, false);
        State::set(IndexSwitch::SKINSELECT_COURSE_RESULT, false);
    }
    else
    {
        selectedMode = mode.value_or(SkinType{} /*maybe invalid*/);
    }
    load(selectedMode);

    auto skinFileList =
        recursiveFindFiles(convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."), "LR2files/Theme/"), ".lr2skin");
    r::sort(skinFileList);
    auto dummySharedData = std::make_shared<lunaticvibes::SkinLr2SharedData>();
    for (auto& p : skinFileList)
    {
        SkinLR2 s(dummySharedData, p, 2);
        LOG_DEBUG << "[Customize] Adding skin: mode='" << s.info.mode << "' p='" << p << "'";
        skinList[s.info.mode].push_back(fs::absolute(p));

        switch (s.info.mode)
        {
        case SkinType::PLAY7: skinList[SkinType::PLAY5].push_back(fs::absolute(p)); break;
        case SkinType::PLAY7_2: skinList[SkinType::PLAY5_2].push_back(fs::absolute(p)); break;
        case SkinType::PLAY14: skinList[SkinType::PLAY10].push_back(fs::absolute(p)); break;
        default: break;
        }
    }

    auto soundsetFileList =
        recursiveFindFiles(convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."), "LR2files/Sound/"), ".lr2ss");
    r::sort(soundsetFileList);
    for (auto& p : soundsetFileList)
    {
        soundsetList.push_back(fs::absolute(p));
    }

    SoundMgr::setSysVolume(1.0);

    _input.register_p("SCENE_PRESS_CUSTOMIZE", std::bind_front(&SceneCustomize::inputGamePress, this));

    LOG_DEBUG << "[Customize] Start";

    State::set(IndexTimer::_SCENE_CUSTOMIZE_START, lunaticvibes::Time().norm());
}

SceneCustomize::~SceneCustomize()
{
    // Reset option entry names to avoid showing garbage in select skin customize mode.
    // Don't do it for the virtual scene as it is destructed after normal SceneCustomize is created.
    if (!_is_virtual)
    {
        optionsKeyList.clear();
        optionsMap.clear();
        updateTexts();
        State::set(IndexText::SKIN_NAME, "");
        State::set(IndexText::SKIN_MAKER_NAME, "");
    }

    // NOTE: do *not* call this here, as it may unintentionally load another skin which may wait for a callback from
    // main thread, causing a dead lock.
    // Not saving here is okay, since we are also saving on each option change.
    // save(selectedMode);

    _input.loopEnd();
}

void SceneCustomize::update_fixed(const lunaticvibes::Time& t)
{
    if (!gInCustomize && gNextScene != SceneType::SELECT)
        return;

    if (gAppIsExiting)
    {
        pSkin->setHandleMouseEvents(false);
        // NOTE: do *not* get() or unload() skins here, as we will be waiting on main thread, while it will waiting on
        // async task ending in ~SceneCustomize, resulting in a dead lock.
        gNextScene = SceneType::EXIT_TRANS;
        gExitingCustomize = true;
    }

    switch (_state)
    {
    case lunaticvibes::CustomizeState::Start: updateStart(t); break;
    case lunaticvibes::CustomizeState::Main: updateMain(t); break;
    case lunaticvibes::CustomizeState::Fadeout: updateFadeout(t); break;
    }
}

void SceneCustomize::updateStart(const lunaticvibes::Time& t)
{
    lunaticvibes::Time rt = t - State::get(IndexTimer::_SCENE_CUSTOMIZE_START);
    if (rt.norm() > pSkin->info.timeIntro)
    {
        _state = lunaticvibes::CustomizeState::Main;

        if (gInCustomize)
            _handleInput = true;

        LOG_DEBUG << "[Customize] State changed to Main";
    }
}

static void reload_preview(SkinType selectedMode)
{
    if (gInCustomize)
    {
        switch (selectedMode)
        {
        case SkinType::PLAY5:
        case SkinType::PLAY5_2:
        case SkinType::PLAY7:
        case SkinType::PLAY7_2:
        case SkinType::PLAY9:
        case SkinType::PLAY9_2:
        case SkinType::PLAY10:
        case SkinType::PLAY14:
            gPlayContext.mode = selectedMode;
            gPlayContext.isAuto = true;
            break;
        default: break;
        }
        gNextScene = getSceneFromSkinType(selectedMode);
        gCustomizeSceneChanged = true;
    }
}

[[nodiscard]] static const char* configOptionNameForSkinType(const SkinType mode)
{
    switch (mode)
    {
    case SkinType::MUSIC_SELECT: return cfg::S_PATH_MUSIC_SELECT;
    case SkinType::DECIDE: return cfg::S_PATH_DECIDE;
    case SkinType::RESULT: return cfg::S_PATH_RESULT;
    // NOTE: LR2 has a bug that it can't save course result skin.
    case SkinType::COURSE_RESULT: return cfg::S_PATH_COURSE_RESULT;
    case SkinType::KEY_CONFIG: return cfg::S_PATH_KEYCONFIG;
    case SkinType::THEME_SELECT: return cfg::S_PATH_CUSTOMIZE;
    case SkinType::PLAY5: return cfg::S_PATH_PLAY_5;
    case SkinType::PLAY5_2: return cfg::S_PATH_PLAY_5_BATTLE;
    case SkinType::PLAY7: return cfg::S_PATH_PLAY_7;
    case SkinType::PLAY7_2: return cfg::S_PATH_PLAY_7_BATTLE;
    case SkinType::PLAY9: return cfg::S_PATH_PLAY_9;
    case SkinType::PLAY10: return cfg::S_PATH_PLAY_10;
    case SkinType::PLAY14: return cfg::S_PATH_PLAY_14;
    case SkinType::EXIT:
    case SkinType::TITLE:
    case SkinType::SOUNDSET:
    case SkinType::PLAY9_2:
    case SkinType::RETRY_TRANS:
    case SkinType::COURSE_TRANS:
    case SkinType::EXIT_TRANS:
    case SkinType::PRE_SELECT:
    case SkinType::TMPL:
    case SkinType::TEST:
    case SkinType::MODE_COUNT: break;
    }
    return nullptr;
};

[[nodiscard]] static int wrappingAdd(int lhs, int rhs, int min, int max)
{
    lhs += rhs;
    if (lhs > max)
    {
        lhs = min;
    }
    if (lhs < min)
    {
        lhs = max;
    }
    return lhs;
}

void SceneCustomize::updateMain(const lunaticvibes::Time& t)
{
    // For the virtual customize scene gInCustomize is always false, so this means we are in a virtual scene and are
    // switching to a real customize scene.
    // Let's allow the new customize scene to actually load stuff. If we load skins here in such case, main thread may
    // be waiting for ~SceneCustomize, but we here will be waiting for the main thread to load textures, causing a dead
    // lock.
    if (gInCustomize && _is_virtual)
        return;

    auto process_mode_change = [&, this](SkinType mode) {
        if (mode == selectedMode)
            return;

        LOG_INFO << "[Customize] Mode has changed => " << static_cast<int>(mode);

        const SkinType modeOld = selectedMode;
        selectedMode = mode;
        save(modeOld);

        SoundMgr::stopNoteSamples();
        SoundMgr::stopSysSamples();
        pSkin->setHandleMouseEvents(false);
        if (selectedMode == SkinType::SOUNDSET)
        {
            load(SkinType::SOUNDSET);
            loadLR2Sound();
            SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::BGM_SELECT);
        }
        else
        {
            if (!gInCustomize || selectedMode != SkinType::MUSIC_SELECT)
            {
                _skinMgr->unload(selectedMode);
            }
            load(selectedMode);
        }
        pSkin->setHandleMouseEvents(true);
        reload_preview(selectedMode);
    };

    auto process_skin_dir_change = [&, this](int plus) {
        LOG_INFO << "[Customize] Skin has changed " << (plus > 0 ? "+" : "-");

        if (selectedMode == SkinType::SOUNDSET)
        {
            if (soundsetList.size() > 1)
            {
                int selectedIdx;
                for (selectedIdx = 0; selectedIdx < (int)soundsetList.size(); selectedIdx++)
                {
                    Path path = convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."),
                                               ConfigMgr::get('S', cfg::S_PATH_SOUNDSET, cfg::S_DEFAULT_PATH_SOUNDSET));
                    if (fs::exists(soundsetList[selectedIdx]) && fs::exists(path) &&
                        fs::equivalent(soundsetList[selectedIdx], path))
                        break;
                }
                selectedIdx = wrappingAdd(selectedIdx, plus, 0, static_cast<int>(soundsetList.size() - 1));
                const auto& p =
                    fs::relative(soundsetList[selectedIdx], PathFromUTF8(ConfigMgr::get('E', cfg::E_LR2PATH, ".")));

                ConfigMgr::set('S', cfg::S_PATH_SOUNDSET, lunaticvibes::u8str(p));

                pSkin->setHandleMouseEvents(false);
                SoundMgr::stopSysSamples();
                load(SkinType::SOUNDSET);
                loadLR2Sound();
                SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::BGM_SELECT);
                pSkin->setHandleMouseEvents(true);
            }
            else
            {
                LOG_DEBUG << "[Customize] soundsetList.size() <= 1";
            }
        }
        else if (!gInCustomize && selectedMode == SkinType::MUSIC_SELECT)
        {
            // Hold up! You want to change select skin inside select skin?
        }
        else
        {
            if (skinList[selectedMode].size() > 1)
            {
                int selectedIdx;
                const auto currentSkin = _skinMgr->get(selectedMode);
                for (selectedIdx = 0; selectedIdx < (int)skinList[selectedMode].size(); selectedIdx++)
                {
                    const Path& p1 = skinList[selectedMode][selectedIdx];
                    const Path& p2 = currentSkin ? Path(currentSkin->getFilePath()) : Path();
                    if (fs::exists(p1) && fs::exists(p2) && fs::equivalent(p1, p2))
                        break;
                }
                selectedIdx = wrappingAdd(selectedIdx, plus, 0, static_cast<int>(skinList[selectedMode].size()) - 1);

                const auto p = fs::relative(skinList[selectedMode][selectedIdx],
                                            PathFromUTF8(ConfigMgr::get('E', cfg::E_LR2PATH, ".")));
                if (const char* key = configOptionNameForSkinType(selectedMode); key != nullptr)
                {
                    ConfigMgr::set('S', key, lunaticvibes::u8str(p));
                }
                else
                {
                    LOG_ERROR << "[Customize] config_path_for_skin_type(" << selectedMode << ") == nullptr";
                }

                pSkin->setHandleMouseEvents(false);
                _skinMgr->unload(selectedMode);
                load(selectedMode);
                pSkin->setHandleMouseEvents(true);

                reload_preview(selectedMode);
            }
            else
            {
                LOG_DEBUG << "[Customize] skinList[selectedMode].size() <= 1";
            }
        }
        plus = 0;
    };

    auto process_option_update = [&, this](size_t idx, int dir) {
        size_t idxOption = topOptionIndex + idx;
        if (idxOption < optionsKeyList.size())
        {
            Option& op = optionsMap[optionsKeyList[idxOption]];
            const size_t idxEntry =
                wrappingAdd(op.selectedEntry, dir, 0, op.entries.empty() ? 0 : (op.entries.size() - 1));

            if (idxEntry != op.selectedEntry)
            {
                setOption(idxOption, idxEntry);
            }

            if (selectedMode == SkinType::SOUNDSET)
            {
                SoundMgr::stopSysSamples();
                load(SkinType::SOUNDSET);
                loadLR2Sound();
                SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::BGM_SELECT);
            }

            reload_preview(selectedMode);
        }
    };

    auto process_option_dragging = [&, this] {
        topOptionIndex = State::get(IndexSlider::SKIN_CONFIG_OPTIONS) * optionsMap.size();
        updateTexts();
    };

    if (gCustomizeContext.mode_update.has_value())
    {
        process_mode_change(gCustomizeContext.mode_update->mode);
        gCustomizeContext.mode_update.reset();
    }

    while (!gCustomizeContext.messages.empty())
    {
        static_assert(
            std::is_trivially_copyable_v<std::remove_reference_t<decltype(gCustomizeContext.messages.front())>>);
        auto msg = gCustomizeContext.messages.front();
        gCustomizeContext.messages.pop();
        auto visitor = overloaded{
            [&process_option_dragging](const lunaticvibes::customize_message::OptionDrag msg) {
                process_option_dragging();
            },
            [&process_option_update](const lunaticvibes::customize_message::OptionUpdate msg) {
                process_option_update(msg.idx, msg.dir);
            },
            [&process_skin_dir_change](const lunaticvibes::customize_message::SkinDirUpdate msg) {
                process_skin_dir_change(msg.plus);
            },
        };
        std::visit(visitor, msg);
    }

    if (exiting)
    {
        State::set(IndexTimer::_SCENE_CUSTOMIZE_FADEOUT, t.norm());
        SoundMgr::setSysVolume(0.0, 1000);
        _state = lunaticvibes::CustomizeState::Fadeout;
        _handleInput = false;
        LOG_DEBUG << "[Customize] State changed to Fadeout";
    }
}

void SceneCustomize::updateFadeout(const lunaticvibes::Time& t)
{
    lunaticvibes::Time rt = t - State::get(IndexTimer::_SCENE_CUSTOMIZE_FADEOUT);

    if (!gExitingCustomize && rt.norm() > pSkin->info.timeOutro)
    {
        LOG_DEBUG << "[Customize] -> Select";
        pSkin->setHandleMouseEvents(false);
        _skinMgr->unload(selectedMode);
        gNextScene = SceneType::SELECT;
        gExitingCustomize = true;
    }
}

////////////////////////////////////////////////////////////////////////////////

StringPath SceneCustomize::getConfigFileName(StringPathView skinPath)
{
    Path p(skinPath);
    std::string md5 = ::md5(lunaticvibes::s(p.u8string())).hexdigest();
    md5 += ".yaml";
    return Path(md5).native();
}

void SceneCustomize::setOption(size_t idxOption, size_t idxEntry)
{
    switch (pSkin->version())
    {
    case SkinVersion::LR2beta3: {
        LVF_ASSERT(idxOption < optionsKeyList.size());
        Option& op = optionsMap[optionsKeyList[idxOption]];
        if (op.id != 0)
        {
            for (size_t i = 0; i < op.entries.size(); ++i)
                setCustomDstOpt(op.id, i, false);
            setCustomDstOpt(op.id, idxEntry, true);
        }
        else
        {
            // save to file when exit
        }
        op.selectedEntry = idxEntry;
        save(selectedMode);
        updateTexts();
        break;
    }

    default: break;
    }

    if (selectedMode == SkinType::SOUNDSET)
    {
        Path path = convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."),
                                   ConfigMgr::get('S', cfg::S_PATH_SOUNDSET, cfg::S_DEFAULT_PATH_SOUNDSET));

        SoundSetLR2 ss(path);
        Path bgmOld = ss.getPathBGMSelect();
        save(SkinType::SOUNDSET);
        SoundSetLR2 ss2(path);
        Path bgmNew = ss2.getPathBGMSelect();
        if (bgmOld != bgmNew)
        {
            SoundMgr::stopSysSamples();
            loadLR2Sound();
            SoundMgr::playSysSample(SoundChannelType::BGM_SYS, eSoundSample::BGM_SELECT);
        }
    }
}

void SceneCustomize::load(SkinType mode)
{
    LOG_DEBUG << "[Customize] Load";
    StringPath configFilePath;
    if (mode == SkinType::SOUNDSET)
    {
        optionsMap.clear();
        optionsKeyList.clear();

        Path path = convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."),
                                   ConfigMgr::get('S', cfg::S_PATH_SOUNDSET, cfg::S_DEFAULT_PATH_SOUNDSET));

        SoundSetLR2 ss(path);

        State::set(IndexText::SKIN_NAME, ss.name);
        State::set(IndexText::SKIN_MAKER_NAME, ss.maker);

        // load names
        size_t count = ss.getCustomizeOptionCount();
        for (size_t i = 0; i < count; ++i)
        {
            SkinBase::CustomizeOption opSkin = ss.getCustomizeOptionInfo(i);
            Option op;
            op.displayName = opSkin.displayName;
            op.selectedEntry = opSkin.defaultEntry;
            op.id = opSkin.id;
            op.entries = opSkin.entries;
            optionsMap[opSkin.internalName] = op;
            optionsKeyList.push_back(opSkin.internalName);
        }

        *pSkin->getTextureCustomizeThumbnail() = Texture{Image{ss.getThumbnailPath()}};

        configFilePath = ss.getFilePath();
    }
    else
    {
        std::shared_ptr<SkinBase> ps = _skinMgr->get(mode);
        if (!ps)
        {
            _skinMgr->reload(mode, true);
            ps = _skinMgr->get(mode);
        }
        optionsMap.clear();
        optionsKeyList.clear();

        if (ps != nullptr)
        {
            State::set(IndexText::SKIN_NAME, ps->getName());
            State::set(IndexText::SKIN_MAKER_NAME, ps->getMaker());

            // load names
            size_t count = ps->getCustomizeOptionCount();
            for (size_t i = 0; i < count; ++i)
            {
                SkinBase::CustomizeOption opSkin = ps->getCustomizeOptionInfo(i);
                Option op;
                op.displayName = opSkin.displayName;
                op.selectedEntry = opSkin.defaultEntry;
                op.id = opSkin.id;
                op.entries = opSkin.entries;
                optionsMap[opSkin.internalName] = op;
                optionsKeyList.push_back(opSkin.internalName);
            }

            configFilePath = ps->getFilePath();

            pSkin->setThumbnailTextureSize(ps->info.resolution.first, ps->info.resolution.second);

            loadPreviewChartIfNeeded(mode);
        }
        else
        {
            LOG_DEBUG << "[Customize] ps==nullptr";
            State::set(IndexText::SKIN_NAME, "");
            State::set(IndexText::SKIN_MAKER_NAME, "");
        }
    }

    if (!configFilePath.empty())
    {
        // load config from file
        Path pCustomize = ConfigMgr::Profile()->getPath() / "customize" / getConfigFileName(configFilePath);
        try
        {
            for (const auto& node : YAML::LoadFile(lunaticvibes::u8str(pCustomize)))
            {
                if (auto itOp = optionsMap.find(node.first.as<std::string>()); itOp != optionsMap.end())
                {
                    Option& op = itOp->second;
                    auto selectedEntryName = node.second.as<std::string>();
                    if (const auto itEntry = r::find(op.entries, selectedEntryName); itEntry != op.entries.end())
                    {
                        op.selectedEntry = std::distance(op.entries.begin(), itEntry);
                    }
                }
            }
        }
        catch (YAML::BadFile&)
        {
            LOG_WARNING << "[Customize] Bad file: " << pCustomize;
        }
    }

    topOptionIndex = 0;
    if (optionsMap.size() > 2)
        State::set(IndexSlider::SKIN_CONFIG_OPTIONS, double(topOptionIndex) / (optionsMap.size() - 1));
    updateTexts();
}

void SceneCustomize::save(SkinType mode) const
{
    Path pCustomize;
    if (mode == SkinType::SOUNDSET)
    {
        Path path = convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."),
                                   ConfigMgr::get('S', cfg::S_PATH_SOUNDSET, cfg::S_DEFAULT_PATH_SOUNDSET));

        SoundSetLR2 ss(path);

        pCustomize = ConfigMgr::Profile()->getPath() / "customize";
        fs::create_directories(pCustomize);
        pCustomize /= getConfigFileName(ss.getFilePath());
    }
    else
    {
        std::shared_ptr<SkinBase> ps = _skinMgr->get(mode);
        if (ps != nullptr)
        {
            pCustomize = ConfigMgr::Profile()->getPath() / "customize";
            fs::create_directories(pCustomize);
            pCustomize /= getConfigFileName(ps->getFilePath());
        }
    }

    if (!pCustomize.empty())
    {
        YAML::Node yaml;
        for (const auto& itOp : optionsMap)
        {
            auto& [tag, op] = itOp;
            if (!op.entries.empty())
                yaml[tag] = op.entries[op.selectedEntry];
        }

        std::ofstream fout(pCustomize, std::ios_base::trunc);
        fout << yaml;
        fout.close();
    }
}

void SceneCustomize::updateTexts() const
{
    for (size_t i = 0; i < 10; ++i)
    {
        auto optionNameId = IndexText(size_t(IndexText::スキンカスタマイズカテゴリ名1個目) + i);
        auto entryNameId = IndexText(size_t(IndexText::スキンカスタマイズ項目名1個目) + i);
        size_t idx = topOptionIndex + i;
        if (idx < optionsKeyList.size())
        {
            const Option& op = optionsMap.at(optionsKeyList[idx]);
            State::set(optionNameId, op.displayName);
            State::set(entryNameId, !op.entries.empty() ? op.entries[op.selectedEntry] : "");
        }
        else
        {
            State::set(optionNameId, "");
            State::set(entryNameId, "");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

// CALLBACK
void SceneCustomize::inputGamePress(InputMask& m, const lunaticvibes::Time& t)
{
    if (!_handleInput)
        return;

    if (m[Input::Pad::ESC])
        exiting = true;
    if (m[Input::Pad::M2])
        exiting = true;

    if (m[Input::Pad::MWHEELUP] && topOptionIndex > 0)
    {
        topOptionIndex--;
        if (optionsMap.size() > 2)
            State::set(IndexSlider::SKIN_CONFIG_OPTIONS, double(topOptionIndex) / (optionsMap.size() - 1));
        updateTexts();
    }

    if (m[Input::Pad::MWHEELDOWN] && topOptionIndex + 1 < optionsMap.size())
    {
        topOptionIndex++;
        if (optionsMap.size() > 2)
            State::set(IndexSlider::SKIN_CONFIG_OPTIONS, double(topOptionIndex) / (optionsMap.size() - 1));
        updateTexts();
    }
}

////////////////////////////////////////////////////////////////////////////////

void SceneCustomize::draw() const
{
    // screenshot
    std::shared_ptr<Texture> pTex = pSkin->getTextureCustomizeThumbnail();
    graphics_copy_screen_texture(*pTex);

    // draw own things
    SceneBase::draw();
}
