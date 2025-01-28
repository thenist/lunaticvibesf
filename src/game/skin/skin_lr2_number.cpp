#include "skin_lr2_number.h"

#include <common/chartformat/chartformat.h>
#include <common/chartformat/chartformat_bms.h>
#include <common/entry/entry_song.h>
#include <game/ruleset/ruleset_bms.h>
#include <game/runtime/index/number.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>

#include <cmath>
#include <memory>

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
    case IndexNumber::INFO_BMS_TOTAL:
        if (auto chart = get_chart_for_display(gSelectContext, gChartContext); chart != nullptr)
            return getEffectiveChartTotal(*chart, convertGaugeType(State::get(IndexOption::PLAY_GAUGE_TYPE_1P)));
        return 0;
    default: break;
    }
    return State::get(ind);
}
