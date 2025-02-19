#pragma once

#include <functional>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace lunaticvibes
{

void assign(std::span<char> to, std::string_view from);

template <class Proj = std::identity>
[[nodiscard]] inline std::string join(char sep, std::ranges::input_range auto v, Proj proj = std::identity{})
{
    using std::begin, std::end;
    auto it = begin(v);
    auto e = end(v);
    if (it == e)
        return {};
    std::stringstream s;
    s << std::invoke(proj, *it++);
    for (; it != e; ++it)
    {
        s << sep;
        s << std::invoke(proj, *it);
    }
    return s.str();
}

inline void replace_all(std::string& str, std::string_view from, std::string_view to)
{
    for (size_t pos = str.find(from); pos != std::string::npos;)
    {
        str.replace(pos, from.length(), to);
        pos = str.find(from, pos + to.length());
    }
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
