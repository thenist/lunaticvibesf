#pragma once

namespace lunaticvibes
{

enum class BlendMode
{
    NONE,
    ALPHA,
    ADD,
    MOD,
    SUBTRACT,
    INVERT,
    MULTIPLY_INVERTED_BACKGROUND,
    MULTIPLY_WITH_ALPHA,
};

} // namespace lunaticvibes

using lunaticvibes::BlendMode;
