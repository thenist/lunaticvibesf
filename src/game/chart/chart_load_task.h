#pragma once

#include <common/chartformat/chartformat.h>

#include <functional>

namespace lunaticvibes
{

void load_audio(ChartFormatBase& chart, const std::function<bool()>& should_discard);
void load_video(ChartFormatBase& chart, const std::function<bool()>& should_discard);

} // namespace lunaticvibes
