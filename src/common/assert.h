#pragma once

#include <source_location>

namespace lunaticvibes
{
[[noreturn]] void assert_failed(const char* msg, std::source_location loc = std::source_location::current());
void verify_failed(const char* msg, std::source_location loc = std::source_location::current());
} // namespace lunaticvibes

#define LVF_ASSERT(cond)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(cond)) [[unlikely]]                                                                                      \
            lunaticvibes::assert_failed(#cond);                                                                        \
    } while (false)

#define LVF_VERIFY(cond)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(cond)) [[unlikely]]                                                                                      \
            lunaticvibes::verify_failed(#cond);                                                                        \
    } while (false)

#ifndef NDEBUG
#define LVF_DEBUG_ASSERT(cond) LVF_ASSERT(cond)
#else // NDEBUG
#define LVF_DEBUG_ASSERT(cond)
#endif // NDEBUG
