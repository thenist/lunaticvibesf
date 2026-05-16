#pragma once

#include <string_view>

namespace lunaticvibes
{

void InstallCrashHandler();
void CrashBreadcrumb(std::string_view message);

} // namespace lunaticvibes
