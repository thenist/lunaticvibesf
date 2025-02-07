#pragma once

#include <cmath>
#include <cstddef>

using size_t = std::size_t;

namespace lunaticvibes::math
{

// Standard deviation using Welford's method
struct StdDev
{
    size_t count{};
    double mean{};
    double m2{};

    void insert(const double x)
    {
        ++count;
        const double delta = x - mean;
        mean += delta / count;
        const double delta2 = x - mean;
        m2 += delta * delta2;
    }

    [[nodiscard]] double variance() const { return (count < 2) ? 0. : m2 / (count - 1); }
    [[nodiscard]] double std_dev() const { return std::sqrt(variance()); }
};

} // namespace lunaticvibes::math
