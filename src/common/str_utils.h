#pragma once

#include <functional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>

namespace lunaticvibes
{

void assign(std::span<char> to, std::string_view from);

template <class Proj = std::identity>
[[nodiscard]] inline std::string join(char sep, std::ranges::input_range auto v, Proj proj = std::identity{})
{
    return std::views::transform(v, proj) | std::views::join_with(sep) | std::ranges::to<std::string>();
}

} // namespace lunaticvibes
