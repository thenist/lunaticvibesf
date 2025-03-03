#pragma once

#include <cstdint>

using std::int16_t;

namespace LR2SkinDef
{

// Negative value means inverse of the option.
using dst_option = int16_t;
inline constexpr dst_option DST_TRUE = 0;
inline constexpr dst_option DST_FALSE = 999;

} // namespace LR2SkinDef

namespace lunaticvibes
{
bool getDstOpt(int d);
void clearCustomDstOpt();
void setCustomDstOpt(unsigned base, size_t offset, bool val);
void updateDstOpt();
} // namespace lunaticvibes

using lunaticvibes::clearCustomDstOpt;
using lunaticvibes::getDstOpt;
using lunaticvibes::setCustomDstOpt;
using lunaticvibes::updateDstOpt;
