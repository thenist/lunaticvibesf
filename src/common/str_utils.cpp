#include "str_utils.h"

#include <algorithm>
#include <cstring>
#include <span>
#include <string_view>

void lunaticvibes::assign(std::span<char> to, std::string_view from)
{
    size_t arrn = to.size();
    if (arrn == 0)
        arrn = 1; // Saturating subtraction.
    const size_t bytes = std::min(arrn - 1, from.size());
    strncpy(to.data(), from.data(), bytes); // NOLINT(bugprone-suspicious-stringview-data-usage): computed on prev line
    to[bytes] = '\0';
}
