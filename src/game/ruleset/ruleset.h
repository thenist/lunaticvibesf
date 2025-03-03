#pragma once

#include <common/beat.h>
#include <common/chartformat/chartformat.h>
#include <common/types.h>
#include <game/chart/chart.h>
#include <game/input/input_callback.h>

#include <utility>

using size_t = std::size_t; // TODO: move to common/types.h

enum class RulesetType : uint8_t
{
    SOUND_ONLY,
    BMS,
};

class RulesetBase
{
public:
    struct BasicData
    {
        lunaticvibes::Time play_time;
        double health = 1.0;

        double acc;       // 0.0 - 100.0
        double total_acc; // 0.0 - 100.0

        unsigned combo;
        unsigned maxCombo;
        unsigned firstMaxCombo; // max combo until first break.
        unsigned comboDisplay;  // for course, last combo coming from previous stage.
        unsigned maxComboDisplay;

        unsigned judge[32]; // judge count. reserved for implementation
    };

protected:
    std::shared_ptr<ChartFormatBase> _format;
    std::shared_ptr<ChartObjectBase> _chart;
    BasicData _basic{};

    bool _isCleared = false;
    bool _isFailed = false;
    bool _isAutoplay = false;

    bool _hasStartTime = false;
    lunaticvibes::Time _startTime;

    unsigned notesReached = 0; // total notes reached. +1 when timestamp reached
    unsigned notesExpired =
        0; // total notes expired. +1 when timestamp+POOR reached; +1 for LN when tail timestamp (no +POOR) is reached

public:
    RulesetBase() : _basic{} {}
    RulesetBase(std::shared_ptr<ChartFormatBase> format, std::shared_ptr<ChartObjectBase> chart)
        : _format(std::move(format)), _chart(std::move(chart)), _basic{}
    {
    }
    virtual ~RulesetBase() = default;

public:
    virtual void updatePress(InputMask& pg, const lunaticvibes::Time& t, const lunaticvibes::InputMaskTimes& tt) = 0;
    virtual void updateHold(InputMask& hg, const lunaticvibes::Time& t) = 0;
    virtual void updateRelease(InputMask& rg, const lunaticvibes::Time& t) = 0;
    virtual void updateAxis(double s1, double s2, const lunaticvibes::Time& t) = 0;
    virtual void update(const lunaticvibes::Time& t) = 0;

public:
    [[nodiscard]] BasicData getData() const { return _basic; }
    [[nodiscard]] virtual double getClearHealth() const = 0;
    [[nodiscard]] virtual bool failWhenNoHealth() const = 0;

    [[nodiscard]] virtual bool isNoScore() const { return false; } // for quick esc
    [[nodiscard]] virtual bool isFinished() const { return notesExpired >= getMaxCombo(); }
    [[nodiscard]] virtual bool isCleared() const { return _isCleared; }
    [[nodiscard]] virtual bool isFailed() const { return _isFailed; }

    virtual void save_graph_point(size_t idx) {};
    virtual std::span<const double> get_acc_graph() { return {}; };
    virtual std::span<const double> get_gauge_graph() { return {}; };

    [[nodiscard]] virtual unsigned getCurrentMaxScore() const = 0;
    [[nodiscard]] virtual unsigned getMaxScore() const = 0;

    // Some games count one LN as two notes. Leave it virtual
    [[nodiscard]] virtual unsigned getNoteCount() const = 0;

    // A LN may have more than one combo (head+tail=2 / rolling LN). Leave it virtual
    [[nodiscard]] virtual unsigned getMaxCombo() const = 0;

    virtual void fail() { _isFailed = true; }

    virtual void updateGlobals() = 0;

    [[nodiscard]] virtual double get_hit_mean() const = 0;
    [[nodiscard]] virtual double get_hit_std_dev() const = 0;

    void setComboDisplay(unsigned combo)
    {
        _basic.comboDisplay = combo;
        _basic.maxComboDisplay = std::max(_basic.maxComboDisplay, _basic.combo + combo);
    }
    void setMaxComboDisplay(unsigned combo) { _basic.maxComboDisplay = combo; }
    void setStartTime(const lunaticvibes::Time& t)
    {
        _hasStartTime = true;
        _startTime = t;
    }
};
