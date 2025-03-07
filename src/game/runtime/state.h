#pragma once

#include "common/types.h"
#include "index/number.h"
#include "index/option.h"
#include "index/slider.h"
#include "index/switch.h"
#include "index/text.h"
#include "index/timer.h"

#include <array>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <utility>

// Global state value manager
class State
{
private:
    static State& inst();

protected:
    template <class Key, class Value, size_t _size> class StateContainer
    {
    public:
        StateContainer() : _data{{}}, _dataDefault{{}} { static_assert(_size > 0); }
        StateContainer(Value defVal) : StateContainer()
        {
            _data.fill(defVal);
            _dataDefault.fill(defVal);
        }

    private:
        std::array<Value, _size> _data;
        std::array<Value, _size> _dataDefault;
        mutable std::shared_mutex _mutex;

    public:
        void get(Key n, Value& out) const
        {
            auto idx = (size_t)n;
            if (idx < _size)
            {
                std::shared_lock l{_mutex};
                out = _data.data()[idx];
            }
            else
            {
                out = {};
            }
        }

        Value get(Key n) const
        {
            Value out;
            get(n, out);
            return out;
        }

        bool set(Key n, const Value& value)
        {
            auto idx = static_cast<size_t>(n);
            if (idx < _size)
            {
                std::unique_lock l{_mutex};
                _data[idx] = value;
                return true;
            }
            return false;
        }

        // PERF: avoid forcing copies for strings.
        bool set(Key n, std::string_view value)
        {
            auto idx = (size_t)n;
            if (idx < _size)
            {
                std::unique_lock l{_mutex};
                _data[idx] = value;
                return true;
            }
            return false;
        }

        bool setDefault(Key n, Value value)
        {
            auto idx = (size_t)n;
            if (idx < _size)
            {
                _dataDefault[idx] = std::move(value);
                return true;
            }
            return false;
        }

        void reset() { _data = _dataDefault; }
    };
    StateContainer<IndexNumber, int, static_cast<size_t>(IndexNumber::NUMBER_COUNT)> gNumbers;
    StateContainer<IndexOption, unsigned, static_cast<size_t>(IndexOption::OPTION_COUNT)> gOptions;
    StateContainer<IndexSlider, Ratio, static_cast<size_t>(IndexSlider::SLIDER_COUNT)> gSliders;
    StateContainer<IndexSwitch, bool, static_cast<size_t>(IndexSwitch::SWITCH_COUNT)> gSwitches;
    StateContainer<IndexText, std::string, static_cast<size_t>(IndexText::TEXT_COUNT)> gTexts;
    StateContainer<IndexTimer, long long, static_cast<size_t>(IndexTimer::TIMER_COUNT)> gTimers{TIMER_NEVER};

private:
    State();

public:
    static bool set(IndexNumber ind, int val);
    static int get(IndexNumber ind);

    static bool set(IndexOption ind, unsigned val);
    static unsigned get(IndexOption ind);

    static bool set(IndexSlider ind, Ratio val);
    static double get(IndexSlider ind);

    static bool set(IndexSwitch ind, bool val);
    static bool get(IndexSwitch ind);

    static bool set(IndexText ind, std::string_view val);
    static std::string get(IndexText ind);
    static void get(IndexText ind, std::string& out);

    static bool set(IndexTimer ind, long long val);
    static long long get(IndexTimer ind);
    static void resetTimer();
};
