#ifdef __linux__

#include "sysutil.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <string>
#include <string_view>

#include <boost/format.hpp>

#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <common/assert.h>
#include <common/log.h>
#include <common/types.h>
#include <common/utils.h>

[[nodiscard]] static std::string ellipsize(const std::string_view s, const size_t len)
{
    std::string out;
    out.reserve(len);
    out += s.substr(0, len / 2);
    out += '~';
    out += s.substr(s.size() - len / 2);
    LVF_DEBUG_ASSERT(out.size() <= len);
    return out;
}

void SetThreadName(const char* name)
{
    // > The  name  can  be up to 16 bytes long, including the terminating null byte.
    static constexpr size_t max_name_len{15};
    std::string_view name_view{name};
    if (name_view.size() > max_name_len)
    {
        const std::string name_ = ellipsize(name_view, max_name_len);
        SetThreadName(name_.c_str());
        return;
    }
    LVF_DEBUG_ASSERT(name_view.size() <= max_name_len);
    int ret = prctl(PR_SET_NAME, name);
    if (ret != 0)
    {
        LOG_ERROR << "PR_SET_NAME failed, ret=" << ret;
    }
}

Path GetExecutablePath()
{
    char fullpath[256] = {0};

    char process_path[] = "/proc/self/exe";
    const auto bytes =
        std::min(readlink(process_path, fullpath, sizeof(fullpath)), static_cast<ssize_t>(sizeof(fullpath) - 1));
    if (bytes >= 0)
        fullpath[bytes] = '\0';

    return PathFromUTF8(fullpath).parent_path();
}

long long getFileLastWriteTime(const Path& p)
{
    struct stat sb;
    int ret = stat(p.native().c_str(), &sb);
    if (ret != 0)
    {
        LOG_ERROR << "stat error, ret=" << ret;
        return 0;
    }
    return static_cast<long long>(sb.st_mtim.tv_sec);
}

namespace portable_strerror_r_detail
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// Use the output of XSI-compliant (from POSIX.1-2001) strerror_r().
static char* impl(int strerror_r_ret, char* buffer, const size_t buffer_length)
{
    if (strerror_r_ret != 0)
    {
        // The message format follows what GNU's strerror_r() would
        // return in such case.
        snprintf(buffer, buffer_length, "Unknown error %d", strerror_r_ret);
    }
    return buffer;
}

// Use the output of GNU-specific strerror_r().
static char* impl(char* strerror_r_ret, char* /*buffer*/, const size_t /*buffer_length*/)
{
    return strerror_r_ret;
}

#pragma GCC diagnostic pop
} // namespace portable_strerror_r_detail

// Interface to support both GNU's and XSI's (from POSIX.1-2001) strerror_r().
const char* safe_strerror(const int errnum, char* buffer, const size_t buffer_length)
{
    return portable_strerror_r_detail::impl(strerror_r(errnum, buffer, buffer_length), buffer, buffer_length);
}

bool lunaticvibes::open(const std::string& link)
{
    // SAFETY: quotes are escaped so that `/tmp";echo pwned"` doesn't hack the system.
    const std::string s = (boost::format("xdg-open %s &") % std::quoted(link)).str();
    LOG_INFO << "Spawning " << std::quoted(s);
    int ret = system(s.c_str());
    if (ret != 0)
    {
        LOG_ERROR << "xdg-open failed with ret=" << ret;
        return false;
    }
    return true;
}

const char* lunaticvibes::safe_ctime(const std::time_t* timep, char* buf)
{
    const char* out = ctime_r(timep, buf);
    if (out == nullptr)
        return "";
    return out;
}

const tm* lunaticvibes::safe_gmtime(const std::time_t* timep, tm* result)
{
    return gmtime_r(timep, result);
}

const tm* lunaticvibes::safe_localtime(const std::time_t* timep, tm* result)
{
    return localtime_r(timep, result);
}

#endif // __linux__
