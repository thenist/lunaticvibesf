#include "skin_lr2_debug.h"

#include <common/assert.h>
#include <common/sysutil.h>
#include <game/runtime/state.h>
#include <game/skin/skin_lr2_bargraph.h>
#include <game/skin/skin_lr2_dst.h>
#include <game/skin/skin_lr2_number.h>

#include <imgui.h>

void imguiMonitorLR2DST()
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!imguiShowMonitorLR2DST)
        return;

    if (ImGui::Begin("LR2 dst_option (F1)", &imguiShowMonitorLR2DST, ImGuiWindowFlags_NoCollapse))
    {
        char titleBuf[32] = {0};
        for (int i = 0; i <= 1099; i += 100)
        {
            sprintf(titleBuf, "%d - %d", i, i + 99);
            if (ImGui::CollapsingHeader(titleBuf))
            {
                if (ImGui::BeginTable(titleBuf, 10, ImGuiTableFlags_SizingFixedSame))
                {
                    ImGui::TableSetupScrollFreeze(0, 0);
                    for (int j = 0; j <= 99; j += 10)
                    {
                        ImGui::TableNextRow();
                        for (int k = 0; k <= 9; ++k)
                        {
                            ImGui::TableSetColumnIndex(k);
                            ImGui::Text("% 4d%c", i + j + k, getDstOpt(i + j + k) ? '+' : ' ');
                        }
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }
}

void imguiMonitorNumber()
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!imguiShowMonitorNumber)
        return;

    if (ImGui::Begin("Numbers (F2)", &imguiShowMonitorNumber, ImGuiWindowFlags_NoCollapse))
    {
        char titleBuf[32] = {0};
        for (int i = 0; i <= 699; i += 20)
        {
            sprintf(titleBuf, "%d - %d", i, i + 20);
            if (ImGui::CollapsingHeader(titleBuf))
            {
                for (int j = 0; j <= 20; j++)
                {
                    ImGui::Text("% 4d: %d", i + j, lunaticvibes::get_number(static_cast<IndexNumber>(i + j)));
                }
            }
        }
        if (ImGui::CollapsingHeader("etc."))
        {
            IndexNumber etcNumbers[] = {
                IndexNumber::RANDOM,
                IndexNumber::INPUT_DETECT_FPS,
                IndexNumber::NEW_ENTRY_SECONDS,
            };
            for (auto& e : etcNumbers)
                ImGui::Text("% 4d: %d", static_cast<int>(e), lunaticvibes::get_number(e));
        }
        ImGui::End();
    }
}

void imguiMonitorOption()
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!imguiShowMonitorOption)
        return;

    if (ImGui::Begin("Options (F3)", &imguiShowMonitorOption, ImGuiWindowFlags_NoCollapse))
    {
        char titleBuf[32] = {0};
        for (int i = 0; i <= 149; i += 20)
        {
            sprintf(titleBuf, "%d - %d", i, i + 20);
            if (ImGui::CollapsingHeader(titleBuf))
            {
                for (int j = 0; j <= 20; j++)
                {
                    ImGui::Text("% 4d: %d", i + j, State::get(static_cast<IndexOption>(i + j)));
                }
            }
        }
        ImGui::End();
    }
}

void imguiMonitorSlider()
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!imguiShowMonitorSlider)
        return;

    if (ImGui::Begin("Sliders (F4)", &imguiShowMonitorSlider, ImGuiWindowFlags_NoCollapse))
    {
        char titleBuf[32] = {0};
        for (int i = 0; i <= 99; i += 20)
        {
            sprintf(titleBuf, "%d - %d", i, i + 20);
            if (ImGui::CollapsingHeader(titleBuf))
            {
                for (int j = 0; j <= 20; j++)
                {
                    ImGui::Text("% 4d: %lf", i + j, State::get(static_cast<IndexSlider>(i + j)));
                }
            }
        }
        ImGui::End();
    }
}

void imguiMonitorSwitch()
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!imguiShowMonitorSwitch)
        return;

    if (ImGui::Begin("Switches (F5)", &imguiShowMonitorSwitch, ImGuiWindowFlags_NoCollapse))
    {
        char titleBuf[32] = {0};
        for (int i = 0; i <= 149; i += 100)
        {
            sprintf(titleBuf, "%d - %d", i, i + 99);
            if (ImGui::CollapsingHeader(titleBuf))
            {
                if (ImGui::BeginTable(titleBuf, 10, ImGuiTableFlags_SizingFixedSame))
                {
                    ImGui::TableSetupScrollFreeze(0, 0);
                    for (int j = 0; j <= 99; j += 10)
                    {
                        ImGui::TableNextRow();
                        for (int k = 0; k <= 9; ++k)
                        {
                            ImGui::TableSetColumnIndex(k);
                            ImGui::Text("% 4d%c", i + j + k,
                                        State::get(static_cast<IndexSwitch>(i + j + k)) ? '+' : ' ');
                        }
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }
}

void imguiMonitorText()
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!imguiShowMonitorText)
        return;

    if (ImGui::Begin("Text (F6)", &imguiShowMonitorText, ImGuiWindowFlags_NoCollapse))
    {
        char titleBuf[32] = {0};
        for (int i = 0; i <= static_cast<int>(IndexText::TEXT_COUNT); i += 20)
        {
            sprintf(titleBuf, "%d - %d", i, i + 20);
            if (ImGui::CollapsingHeader(titleBuf))
            {
                for (int j = 0; j <= 20; j++)
                {
                    ImGui::Text("% 4d: %s", i + j, State::get(static_cast<IndexText>(i + j)).c_str());
                }
            }
        }
        ImGui::End();
    }
}

void imguiMonitorBargraph()
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!imguiShowMonitorBargraph)
        return;

    if (ImGui::Begin("Bar graphs (F7)", &imguiShowMonitorBargraph, ImGuiWindowFlags_NoCollapse))
    {
        char titleBuf[32] = {0};
        for (int i = 0; i <= 99; i += 20)
        {
            sprintf(titleBuf, "%d - %d", i, i + 20);
            if (ImGui::CollapsingHeader(titleBuf))
            {
                for (int j = 0; j <= 20; j++)
                {
                    ImGui::Text("% 4d: %lf", i + j, lunaticvibes::get_bargraph(static_cast<IndexBargraph>(i + j)));
                }
            }
        }
        ImGui::End();
    }
}

void imguiMonitorTimer()
{
    LVF_DEBUG_ASSERT(IsMainThread());
    if (!imguiShowMonitorTimer)
        return;

    if (ImGui::Begin("Timers (F8)", &imguiShowMonitorTimer, ImGuiWindowFlags_NoCollapse))
    {
        char titleBuf[32] = {0};
        for (int i = 0; i <= 299; i += 20)
        {
            sprintf(titleBuf, "%d - %d", i, i + 20);
            if (ImGui::CollapsingHeader(titleBuf))
            {
                for (int j = 0; j <= 20; j++)
                {
                    long long t = State::get(static_cast<IndexTimer>(i + j));
                    if (t == TIMER_NEVER)
                        ImGui::Text("% 4d: -", i + j);
                    else
                        ImGui::Text("% 4d: %lld", i + j, t);
                }
            }
        }
        ImGui::End();
    }
}
