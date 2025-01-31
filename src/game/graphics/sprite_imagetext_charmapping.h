#pragma once

#include <game/graphics/rect.h>

#include <unordered_map>

using size_t = std::size_t;

struct CharMapping
{
    size_t textureIdx;
    Rect textureRect;
};
using CharMappingList = std::unordered_map<char32_t, CharMapping>;
