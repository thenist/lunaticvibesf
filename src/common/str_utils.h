#pragma once

#include <functional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lunaticvibes
{

void assign(std::span<char> to, std::string_view from);

template <class Proj = std::identity>
[[nodiscard]] inline std::string join(char sep, std::ranges::input_range auto v, Proj proj = std::identity{})
{
    return std::views::transform(v, proj) | std::views::join_with(sep) | std::ranges::to<std::string>();
}

template <class T> void split(const std::string_view raw, const char delim, std::vector<T>& out)
{
    out.clear();
    size_t start = 0;
    for (size_t end = raw.find(delim); end != std::string_view::npos;)
    {
        out.emplace_back(raw.substr(start, end - start));
        start = end + 1;
        end = raw.find(delim, start);
    }
    out.emplace_back(raw.substr(start));
}

} // namespace lunaticvibes
