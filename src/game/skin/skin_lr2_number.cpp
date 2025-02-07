#include "skin_lr2_number.h"

#include <common/chartformat/chartformat.h>
#include <common/chartformat/chartformat_bms.h>
#include <common/entry/entry_song.h>
#include <game/ruleset/ruleset_bms.h>
#include <game/runtime/index/number.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>

#include <cmath>
#include <cstdlib>
#include <memory>
#include <utility>

static std::shared_ptr<ChartFormatBase> get_current_select_chart(SelectContextParams& ctx)
{
    std::shared_lock u(ctx._mutex);
    const EntryList& e = ctx.entries;
    if (e.empty())
        return {};
    auto idx = ctx.selectedEntryIndex;
    if (e[idx].first->type() == eEntryType::CHART || e[idx].first->type() == eEntryType::RIVAL_CHART)
        return std::reinterpret_pointer_cast<EntryChart>(e[idx].first)->_file;
    return {};
}

static std::shared_ptr<ChartFormatBase> get_chart_for_display(SelectContextParams& select_ctx,
                                                              ChartContextParams& chart_ctx)
{
    std::shared_ptr<ChartFormatBase> chart;
    if (chart_ctx.started)
        chart = chart_ctx.chart;
    if (chart == nullptr)
        chart = get_current_select_chart(select_ctx);
    return chart;
}

static std::shared_ptr<ChartObjectBase> get_chart_obj_for_display(PlayContextParams& play_ctx)
{
    std::shared_lock l(play_ctx._mutex);
    return play_ctx.chartObj[PLAYER_SLOT_PLAYER];
}

// \return Seconds
static unsigned get_current_chart_length(SelectContextParams& select_ctx, ChartContextParams& chart_ctx,
                                         PlayContextParams& play_ctx)
{
    // ChartFormatBase doesn't always have populated totalLength.
    if (auto obj = get_chart_obj_for_display(play_ctx); obj != nullptr)
        return obj->getTotalLength().norm() / 1000;
    if (auto chart = get_chart_for_display(select_ctx, chart_ctx); chart != nullptr)
        return chart->totalLength;
    return 0;
}

[[nodiscard]] static int decimal_part(double n)
{
    return std::abs(static_cast<int>((n - static_cast<int>(n)) * 100));
}

[[nodiscard]] static int ratio_whole(int x, int y)
{
    return y == 0 ? x : x / y;
}

[[nodiscard]] static int ratio_decimal(int x, int y)
{
    return y == 0 ? x : decimal_part(static_cast<double>(x) / y);
}

int lunaticvibes::get_number(IndexNumber ind)
{
    // Sources:
    // LR2
    // LR2OOL - https://github.com/tenaibms/LR2OOL/blob/c5505559ed5cc86e1355fee3f6f9e967efaab08f/src/hooks/srcnumber.cpp
    // beatoraja
    switch (std::to_underlying(ind))
    {
    case 100:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); ruleset)
            return static_cast<int>(std::round(ruleset->getMoneyScoreAnimation().animate({})));
        return 0;
    case 107:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); ruleset)
            return static_cast<int>(ruleset->getHealthAnimation().animate({}) * 100);
        break; // might still also be in State::get

    case 120:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_TARGET]); ruleset)
            return static_cast<int>(std::round(ruleset->getMoneyScoreAnimation().animate({})));
        return 0;
    case 127:
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_TARGET]); ruleset)
            return static_cast<int>(ruleset->getHealthAnimation().animate({}) * 100);
        break; // might still also be in State::get

    case 295: return 1112333; // TODO // LR2OOL: current random

    case 296: // LR2OOL
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return static_cast<int>(ruleset->get_hit_mean());
        return 0;
    case 297: // LR2OOL
        if (auto ruleset = gPlayContext.ruleset[PLAYER_SLOT_PLAYER]; ruleset)
            return decimal_part(ruleset->get_hit_mean());
        return 0;

    case 298:           // TODO // LR2OOL: whole part of stddev
    case 299: return 0; // TODO // LR2OOL: decimal part of stddev

    case 301: // LR2HelperG
        if (auto chart = get_chart_for_display(gSelectContext, gChartContext); chart != nullptr)
            return getEffectiveChartTotal(*chart, convertGaugeType(State::get(IndexOption::PLAY_GAUGE_TYPE_1P)));
        return 0;

    case 400: // LR2OOL
        return ratio_whole(State::get(IndexNumber::PLAY_1P_PERFECT), State::get(IndexNumber::PLAY_1P_GREAT));
    case 401: // LR2OOL
        return ratio_decimal(State::get(IndexNumber::PLAY_1P_PERFECT), State::get(IndexNumber::PLAY_1P_GREAT));
    case 402: // LR2OOL
        return ratio_whole(State::get(IndexNumber::PLAY_1P_GREAT), State::get(IndexNumber::PLAY_1P_GOOD));
    case 403: // LR2OOL
        return ratio_decimal(State::get(IndexNumber::PLAY_1P_GREAT), State::get(IndexNumber::PLAY_1P_GOOD));

    case 407: // beatoraja
        if (auto ruleset = std::dynamic_pointer_cast<RulesetBMS>(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]); ruleset)
            return decimal_part(ruleset->getHealthAnimation().animate({}) * 100.);
        return 0;
    case 1163: return get_current_chart_length(gSelectContext, gChartContext, gPlayContext) / 60; // beatoraja
    case 1164: return get_current_chart_length(gSelectContext, gChartContext, gPlayContext) % 60; // beatoraja

    default: break;
    }
    return State::get(ind);
}
