#include "scene.h"

#include <cstddef>
#include <format>
#include <functional>
#include <memory>

#include <common/assert.h>
#include <common/beat.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <config/config_mgr.h>
#include <game/graphics/graphics.h>
#include <game/runtime/generic_info.h>
#include <game/runtime/i18n.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>
#include <game/skin/skin_lr2_debug.h>
#include <game/skin/skin_mgr.h>
#include <game/sound/sound_mgr.h>
#include <game/sound/sound_sample.h>

#include <imgui.h>

// prototype
SceneBase::SceneBase(const std::shared_ptr<SkinMgr>& skinMgr, SkinType skinType, unsigned rate, bool backgroundInput)
    : _type(SceneType::NOT_INIT), _input(ConfigMgr::get('P', cfg::P_INPUT_POLLING_RATE, 1000), backgroundInput),
      _rate(rate)
{
    // Disable skin caching for now. dst options are changing all the time
    const bool simple_skin = gInCustomize && skinType != SkinType::THEME_SELECT;
    if (skinMgr)
    {
        skinMgr->reload(skinType, gInCustomize && simple_skin);
        pSkin = skinMgr->get(skinType);
    }
    else
    {
        LOG_VERBOSE << "[SceneBase] No skinMgr";
    }

    int notificationPosY = 480;
    int notificationWidth = 640;

    if (pSkin &&
        ((!gInCustomize && skinType != SkinType::THEME_SELECT) || (gInCustomize && skinType == SkinType::THEME_SELECT)))
    {
        const int x = pSkin->info.resolution.first;
        const int y = pSkin->info.resolution.second;
        graphics_resize_canvas(x, y);
        notificationPosY = y;
        notificationWidth = x;
    }

    if (!simple_skin)
    {
        int faceIndex;
        const Path fontPath = getSysMonoFontPath(nullptr, &faceIndex, i18n::getCurrentLanguage());
        const int notificationHeight = 20;
        const int textHeight = 24;
        _fNotifications = std::make_shared<TTFFont>(fontPath, int(textHeight * 1.5), faceIndex);
        _texNotificationsBG = std::make_shared<TextureFull>(0x000000ff);
        for (size_t i = 0; i < _sNotifications.size(); ++i)
        {
            SpriteText::SpriteTextBuilder textBuilder;
            textBuilder.font = _fNotifications;
            textBuilder.textInd = IndexText(size_t(IndexText::_OVERLAY_NOTIFICATION_0) + i);
            textBuilder.align = TextAlign::TEXT_ALIGN_LEFT;
            textBuilder.ptsize = textHeight;
            _sNotifications[i] = textBuilder.build();
            _sNotifications[i]->setMotionLoopTo(0);

            SpriteStatic::SpriteStaticBuilder bgBuilder;
            bgBuilder.texture = _texNotificationsBG;
            _sNotificationsBG[i] = bgBuilder.build();
            _sNotificationsBG[i]->setMotionLoopTo(0);

            notificationPosY -= notificationHeight;
            MotionKeyFrame f;
            f.time = 0;
            f.param.rect = Rect(0, notificationPosY, notificationWidth, notificationHeight);
            f.param.accel = MotionKeyFrameParams::CONSTANT;
            f.param.blend = BlendMode::ALPHA;
            f.param.filter = true;
            f.param.angle = 0;
            f.param.center = Point(0, 0);
            f.param.color = 0xffffff80;
            _sNotificationsBG[i]->appendMotionKeyFrame(f);

            f.param.rect.y += (notificationHeight - textHeight) / 2;
            f.param.rect.h = textHeight;
            f.param.color = 0xffffffff;
            _sNotifications[i]->appendMotionKeyFrame(f);
        }
    }

    _input.register_p("DEBUG_TOGGLE", std::bind_front(&SceneBase::DebugToggle, this));

    _input.register_p("GLOBALFUNC", std::bind_front(&SceneBase::GlobalFuncKeys, this));

    _input.register_p("SKIN_MOUSE_CLICK", std::bind_front(&SceneBase::MouseClick, this));
    _input.register_h("SKIN_MOUSE_DRAG", std::bind_front(&SceneBase::MouseDrag, this));
    _input.register_r("SKIN_MOUSE_RELEASE", std::bind_front(&SceneBase::MouseRelease, this));

    if (pSkin && !(gNextScene == SceneType::SELECT && skinType == SkinType::THEME_SELECT))
    {
        State::resetTimer();

        State::set(IndexText::_OVERLAY_TOPLEFT, "");

        // Skin may be cached. Reset mouse status
        pSkin->setHandleMouseEvents(true);
    }
}

SceneBase::~SceneBase()
{
    LVF_ASSERT(!_input.isRunning());
}

void SceneBase::update()
{
    lunaticvibes::Time t;
    gUpdateContext.updateTime = t;

    if (pSkin)
    {
        // update skin
        pSkin->update();
        auto [x, y] = _input.getCursorPos();
        pSkin->update_mouse(x, y);

        checkAndStartTextEdit();

        // update notifications
        {
            std::unique_lock lock(gOverlayContext._mutex);

            // notifications expire check
            while (!gOverlayContext.notifications.empty() &&
                   (t - gOverlayContext.notifications.begin()->first).norm() >
                       static_cast<decltype(std::declval<timeNormRes>().count())>(10 * 1000)) // 10s
            {
                gOverlayContext.notifications.pop_front();
            }

            // update notification texts
            auto itNotifications = gOverlayContext.notifications.rbegin();
            for (size_t i = 0; i < _sNotifications.size(); ++i)
            {
                if (itNotifications != gOverlayContext.notifications.rend())
                {
                    State::set(IndexText(size_t(IndexText::_OVERLAY_NOTIFICATION_0) + i), itNotifications->second);
                    ++itNotifications;
                }
                else
                {
                    State::set(IndexText(size_t(IndexText::_OVERLAY_NOTIFICATION_0) + i), "");
                }
            }
        }

        // update top-left texts
        for (auto& bg : _sNotificationsBG)
        {
            if (bg != nullptr)
                bg->update(t);
        }
        for (auto& notification : _sNotifications)
        {
            if (notification != nullptr)
                notification->update(t);
        }

        // update videos
        TextureVideo::updateAll();
    }

    if ((!gInCustomize || _type == SceneType::CUSTOMIZE) && shouldShowImgui())
    {
        ImGuiNewFrame();
        updateImgui();
        ImGui::Render();
    }
}

void SceneBase::MouseClick(InputMask& m, const lunaticvibes::Time& t)
{
    if (!pSkin)
        return;
    if (m[Input::Pad::M1])
    {
        auto [x, y] = _input.getCursorPos();
        pSkin->update_mouse_click(x, y);
    }
}

void SceneBase::MouseDrag(InputMask& m, const lunaticvibes::Time& t)
{
    if (!pSkin)
        return;
    if (m[Input::Pad::M1])
    {
        auto [x, y] = _input.getCursorPos();
        pSkin->update_mouse_drag(x, y);
    }
}

void SceneBase::MouseRelease(InputMask& m, const lunaticvibes::Time& t)
{
    if (!pSkin)
        return;
    if (m[Input::Pad::M1])
    {
        pSkin->update_mouse_release();
    }
}

bool SceneBase::queuedScreenshot = false;
bool SceneBase::queuedFPS = false;
bool SceneBase::showFPS = false;

void SceneBase::draw() const
{
    if (pSkin)
    {
        pSkin->draw();
    }

    {
        std::shared_lock lock(gOverlayContext._mutex);
        if (!gOverlayContext.notifications.empty())
        {
            // draw notifications at the bottom. One string per line
            for (size_t i = 0; i < gOverlayContext.notifications.size() && i < _sNotifications.size(); ++i)
            {
                if (_sNotificationsBG[i] != nullptr)
                {
                    _sNotificationsBG[i]->draw();
                }
                if (_sNotifications[i] != nullptr)
                {
                    _sNotifications[i]->updateText();
                    _sNotifications[i]->draw();
                }
            }
        }
    }

    if (queuedScreenshot)
    {
        Path p = "screenshot";
        p /= std::format("LV {:04d}-{:02d}-{:02d} {:02d}-{:02d}-{:02d}.png", State::get(IndexNumber::DATE_YEAR),
                         State::get(IndexNumber::DATE_MON), State::get(IndexNumber::DATE_DAY),
                         State::get(IndexNumber::DATE_HOUR), State::get(IndexNumber::DATE_MIN),
                         State::get(IndexNumber::DATE_SEC));

        lunaticvibes::graphics::queue_screenshot(std::move(p));

        SoundMgr::playSysSample(SoundChannelType::KEY_SYS, eSoundSample::SOUND_SCREENSHOT);
        queuedScreenshot = false;
    }

    if (queuedFPS)
    {
        showFPS = !showFPS;
        queuedFPS = false;
    }
}

static bool should_show_text_overlay()
{
    for (size_t i = 0; i < 4; ++i)
        if (!State::get(IndexText(int(IndexText::_OVERLAY_TOPLEFT) + i)).empty())
            return true;
    return false;
}

bool SceneBase::shouldShowImgui() const
{
    return showFPS || should_show_text_overlay() || lunaticvibes::g_enable_imgui_debug_monitor;
}

void SceneBase::updateImgui()
{
    LVF_DEBUG_ASSERT(IsMainThread());

    // TODO: implement showFPS without Imgui, as Imgui takes over 10% FPS.
    if (showFPS || should_show_text_overlay())
    {
        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.f, 0.f, 0.f, 0.4f});
        if (ImGui::Begin("##textoverlay", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (showFPS)
            {
                ImGui::PushID("##fps");
                ImGui::Text("FPS: Render %d | Input %d", State::get(IndexNumber::FPS),
                            State::get(IndexNumber::INPUT_DETECT_FPS));
                ImGui::PopID();
            }

            static const char* overlayTextID[] = {
                "##overlaytext1",
                "##overlaytext2",
                "##overlaytext3",
                "##overlaytext4",
            };
            size_t count = 0;
            for (size_t i = 0; i < 4; ++i)
            {
                auto idx = IndexText(int(IndexText::_OVERLAY_TOPLEFT) + i);
                if (!State::get(idx).empty())
                {
                    ImGui::PushID(overlayTextID[count++]);
                    ImGui::TextUnformatted(State::get(idx).c_str());
                    ImGui::PopID();
                }
            }

            ImGui::End();
        }
        ImGui::PopStyleColor();
    }

    if (lunaticvibes::g_enable_imgui_debug_monitor)
    {
        if (imguiShowMonitorLR2DST)
            imguiMonitorLR2DST();
        if (imguiShowMonitorNumber)
            imguiMonitorNumber();
        if (imguiShowMonitorOption)
            imguiMonitorOption();
        if (imguiShowMonitorSlider)
            imguiMonitorSlider();
        if (imguiShowMonitorSwitch)
            imguiMonitorSwitch();
        if (imguiShowMonitorText)
            imguiMonitorText();
        if (imguiShowMonitorBargraph)
            imguiMonitorBargraph();
        if (imguiShowMonitorTimer)
            imguiMonitorTimer();
    }
}

void SceneBase::DebugToggle(InputMask& p, const lunaticvibes::Time& t)
{
    if (!(!gInCustomize || _type == SceneType::CUSTOMIZE))
        return;
    if (!lunaticvibes::g_enable_imgui_debug_monitor)
        return;

    if (p[Input::F1])
    {
        imguiShowMonitorLR2DST = !imguiShowMonitorLR2DST;
    }
    if (p[Input::F2])
    {
        imguiShowMonitorNumber = !imguiShowMonitorNumber;
    }
    if (p[Input::F3])
    {
        imguiShowMonitorOption = !imguiShowMonitorOption;
    }
    if (p[Input::F4])
    {
        imguiShowMonitorSlider = !imguiShowMonitorSlider;
    }
    if (p[Input::F5])
    {
        imguiShowMonitorSwitch = !imguiShowMonitorSwitch;
    }
    if (p[Input::F6])
    {
        imguiShowMonitorText = !imguiShowMonitorText;
    }
    if (p[Input::F7])
    {
        imguiShowMonitorBargraph = !imguiShowMonitorBargraph;
    }
    if (p[Input::F8])
    {
        imguiShowMonitorTimer = !imguiShowMonitorTimer;
    }
}

bool SceneBase::isInTextEdit() const
{
    return inTextEdit;
}

IndexText SceneBase::textEditType() const
{
    return inTextEdit ? pSkin->textEditType() : IndexText::INVALID;
}

void SceneBase::startTextEdit(bool clear)
{
    if (pSkin)
    {
        pSkin->startTextEdit(clear);
        inTextEdit = true;
    }
}

void SceneBase::stopTextEdit(bool modify)
{
    if (pSkin)
    {
        inTextEdit = false;
        pSkin->stopTextEdit(modify);
    }
}

void SceneBase::GlobalFuncKeys(InputMask& m, const lunaticvibes::Time& t)
{
    if (m[Input::F6])
    {
        queuedScreenshot = true;
    }

    if (m[Input::F7])
    {
        queuedFPS = true;
    }
}
