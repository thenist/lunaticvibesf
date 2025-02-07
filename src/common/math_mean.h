#pragma once

#include <cstddef>
#include <cstdint>

using int64_t = std::int64_t;
using size_t = std::size_t;

namespace lunaticvibes::math
{

struct Mean
{
    int64_t accumulator{0};
    size_t samples{0};

    // WARNING: better off using small samples not to overflow the accumulator.
    void insert(int64_t sample)
    {
        accumulator += sample;
        ++samples;
    }

    [[nodiscard]] double mean() const { return samples == 0 ? 0 : static_cast<double>(accumulator) / samples; }
};

} // namespace lunaticvibes::math
