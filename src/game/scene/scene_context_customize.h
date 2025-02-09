#pragma once

#include <common/types.h>

#include <cstddef>
#include <queue>
#include <variant>

using std::size_t;

namespace lunaticvibes
{

// Also used by select.
struct ModeUpdate
{
    SkinType mode;
};

namespace customize_message
{

struct OptionDrag
{
};

struct OptionUpdate
{
    size_t idx;
    int dir;
};

struct SkinDirUpdate
{
    int plus; // -1 or 1
};

} // namespace customize_message

using CustomizeMessage =
    std::variant<customize_message::OptionDrag, customize_message::OptionUpdate, customize_message::SkinDirUpdate>;

} // namespace lunaticvibes

struct CustomizeContextParams
{
    std::queue<lunaticvibes::CustomizeMessage> messages;
    std::optional<lunaticvibes::ModeUpdate> mode_update;
};
