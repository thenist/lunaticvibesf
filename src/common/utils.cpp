#include "utils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <re2/re2.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincrypt.h>
#else
#include <dirent.h>
#include <openssl/evp.h>
#endif

#include <common/assert.h>
#include <common/hash.h>
#include <common/log.h>
#include <common/str_utils.h>
#include <common/sysutil.h>
#include <common/types.h>
#include <common/u8.h>

namespace r = std::ranges;

std::vector<Path> findFiles(Path p, bool recursive)
{
    LOG_VERBOSE << "[Utils] findFiles: '" << p << "' (" << (recursive ? "" : "non-") << "recursive)";
    auto pstr = p.make_preferred().native();
    size_t offset = pstr.find('*');

    std::vector<Path> res;
    if (offset == pstr.npos)
    {
        if (!pstr.empty())
            res.push_back(std::move(p));
        return res;
    }

    StringPath folder = pstr.substr(0, std::min(offset, pstr.find_last_of(Path::preferred_separator, offset)));
    if (fs::is_directory(folder))
    {
        pstr =
            pstr.substr(pstr[folder.length() - 1] == Path::preferred_separator ? folder.length() : folder.length() + 1);

        static const std::pair<RE2, re2::StringPiece> path_replace_pattern[]{
            {R"(\\)", R"(\\\\)"}, {R"(\.)", R"(\\.)"},     {R"(\^)", R"(\\^)"},      {R"(\$)", R"(\\$)"},
            {R"(\|)", R"(\\|)"},  {R"(\()", R"(\\()"},     {R"(\))", R"(\\))"},      {R"(\{)", R"(\\{)"},
            {R"(\{)", R"(\\{)"},  {R"(\[)", R"(\\[)"},     {R"(\])", R"(\\])"},      {R"(\+)", R"(\\+)"},
            {R"(\/)", R"(\\/)"},  {R"(\?)", R"([^\\\\])"}, {R"(\*)", R"([^\\\\]*)"},
        };

        std::string str = lunaticvibes::u8str(pstr);
        for (const auto& [in, out] : path_replace_pattern)
        {
            RE2::GlobalReplace(&str, in, out);
        }

        // Case-insensitive file extensions for LR2 paths.
        if (size_t idx = str.find_last_of('.'); idx != str.npos)
            str.replace(idx, 1, ".(?i)");

        LOG_VERBOSE << "[Utils] findFiles: .. will match '" << str << "' on contents of '" << folder << "'";
        auto pathRegex = RE2(str);
        Path pathFolder(folder);
        if (recursive)
        {
            for (auto& f : fs::recursive_directory_iterator(pathFolder))
            {
                if (f.path().filename().u8string().substr(0, 2) != u8"._" &&
                    RE2::FullMatch(lunaticvibes::u8str(f.path().filename()), pathRegex))
                    res.push_back(f.path());
            }
        }
        else
        {
            for (auto& f : fs::directory_iterator(pathFolder))
            {
                auto relativeFilePath = fs::relative(f, pathFolder);
                if (relativeFilePath.u8string().substr(0, 2) != u8"._" &&
                    RE2::FullMatch(lunaticvibes::u8str(relativeFilePath), pathRegex))
                {
                    res.push_back(f.path());
                }
            }
        }
    }

    return res;
}

bool isParentPath(const Path& parent_, const Path& dir_)
try
{
    const Path parent = fs::absolute(parent_);
    const Path dir = fs::absolute(dir_);

    if (parent.root_directory() != dir.root_directory())
        return false;

    return r::mismatch(dir, parent).in2 == parent.end();
}
catch (const std::filesystem::filesystem_error& e)
{
    LOG_DEBUG << "[Utils] is_parent_path() filesystem_error: " << e.what();
    return false;
}

int toInt(std::string_view str, int defVal) noexcept
{
    int val = 0;
    if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), val); ec == std::errc())
        return val;
    return defVal;
}

double toDouble(std::string_view str, double defVal) noexcept
{
    double val = 0;
    if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), val); ec == std::errc())
        return val;
    return defVal;
}

bool lunaticvibes::iequals(std::string_view lhs, std::string_view rhs) noexcept
{
    return r::equal(lhs, rhs, [](unsigned char l, unsigned char r) { return std::tolower(l) == std::tolower(r); });
}

#ifdef _WIN32

HashMD5 md5(std::string_view s)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);

    CryptHashData(hHash, (const BYTE*)s.data(), s.size(), 0);

    constexpr size_t MD5_LEN = 16;
    BYTE rgbHash[MD5_LEN];
    DWORD cbHash = MD5_LEN;
    CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return HashMD5{lunaticvibes::bin2hex({rgbHash, MD5_LEN})};
}

HashMD5 md5file(const Path& filePath)
{
    if (!fs::exists(filePath) || !fs::is_regular_file(filePath))
    {
        return {};
    }

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);

    std::ifstream ifs(filePath, std::ios::in | std::ios::binary);
    while (!ifs.eof())
    {
        constexpr size_t BUFSIZE = 1024;
        char rgbFile[BUFSIZE];
        DWORD cbRead = 0;
        ifs.read(rgbFile, BUFSIZE);
        cbRead = ifs.gcount();
        if (cbRead == 0)
            break;
        CryptHashData(hHash, (const BYTE*)rgbFile, cbRead, 0);
    }

    constexpr size_t MD5_LEN = 16;
    BYTE rgbHash[MD5_LEN];
    DWORD cbHash = MD5_LEN;
    CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return HashMD5{lunaticvibes::bin2hex({rgbHash, MD5_LEN})};
}

#else

static HashMD5 format_hash(std::span<unsigned char> digest)
{
    std::string ret;
    for (unsigned char i : digest)
    {
        unsigned char high = i >> 4 & 0xF;
        unsigned char low = i & 0xF;
        ret += static_cast<char>(high <= 9 ? ('0' + high) : ('A' - 10 + high));
        ret += static_cast<char>(low <= 9 ? ('0' + low) : ('A' - 10 + low));
    }
    return HashMD5{ret};
}

template <typename DigestUpdater> static HashMD5 md5_impl(DigestUpdater updater)
{
    struct MdCtxDeleter
    {
        void operator()(EVP_MD_CTX* ctx) { EVP_MD_CTX_free(ctx); }
    };
    auto ctx = std::unique_ptr<EVP_MD_CTX, MdCtxDeleter>(EVP_MD_CTX_new());
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;

    if (!EVP_DigestInit_ex2(ctx.get(), EVP_md5(), nullptr))
    {
        lunaticvibes::verify_failed("EVP_DigestInit_ex2()");
        return {};
    };
    if (!updater(ctx.get()))
    {
        lunaticvibes::verify_failed("Digest updating");
        return {};
    }
    if (!EVP_DigestFinal_ex(ctx.get(), digest, &digest_len))
    {
        lunaticvibes::verify_failed("EVP_DigestFinal_ex");
        return {};
    };

    return format_hash({digest, digest_len});
}

HashMD5 md5(std::string_view s)
{
    return md5_impl([&](EVP_MD_CTX* context) { return EVP_DigestUpdate(context, s.data(), s.size()) != 0; });
}

HashMD5 md5file(const Path& filePath)
{
    if (!fs::exists(filePath) || !fs::is_regular_file(filePath))
    {
        LOG_WARNING << "[Utils] Path is not a file or does not exist";
        return {};
    }
    std::ifstream ifs{filePath, std::ios::binary};
    return md5_impl([&](EVP_MD_CTX* context) {
        std::array<char, 1024> buf;
        while (!ifs.eof())
        {
            ifs.read(buf.data(), buf.size());
            if (!EVP_DigestUpdate(context, buf.data(), ifs.gcount()))
                return false;
        }
        return true;
    });
}

#endif

std::string toLower(std::string_view s)
{
    std::string ret(s);
    for (auto& c : ret)
        if (c >= 'A' && c <= 'Z')
            c = c - 'A' + 'a';
    return ret;
}

std::string toUpper(std::string_view s)
{
    std::string ret(s);
    for (auto& c : ret)
        if (c >= 'a' && c <= 'z')
            c = c - 'a' + 'A';
    return ret;
}

#ifndef _WIN32

struct DirDeleter
{
    void operator()(DIR* dir)
    {
        int ret = closedir(dir);
        if (ret == -1)
        {
            const int error = errno;
            LOG_ERROR << "closedir() error: " << safe_strerror(error) << " (" << error << ")";
        }
    }
};
using DirPtr = std::unique_ptr<DIR, DirDeleter>;

Path lunaticvibes::resolve_windows_path(std::string input)
{
    r::replace(input, '\\', '/');

    if (input == "/" || input == "." || input.empty())
        return PathFromUTF8(input);

    static constexpr std::string_view CURRENT_PATH_RELATIVE_PREFIX = "./";
    const char first_character = input[0];
    // Used to determine if this is a relative path without a leading
    // `./`. If so, we prepend it to the string temporarily, so that we
    // can use `filesystem::directory_iterator`. After we are done, we
    // will remove this prepended prefix.
    const bool has_path_prefix = (first_character == '.') || (first_character == '/');

    std::string out;
    out.reserve(input.length() + CURRENT_PATH_RELATIVE_PREFIX.length());

    static thread_local std::vector<std::string> segments;
    lunaticvibes::split(input, '/', segments);

    size_t segments_traversed = 0;
    for (const auto& segment : segments)
    {
        if (segment.empty())
        {
            segments_traversed += 1;
            continue;
        }

        std::string_view prefix;
        if (segment == ".")
            prefix = CURRENT_PATH_RELATIVE_PREFIX;
        else if (segment == "..")
            prefix = "../";
        if (!prefix.empty())
        {
            if (!out.empty() && out.back() != '/')
                out += '/';
            out += prefix;
            segments_traversed += 1;
            continue;
        }

        const bool is_empty = out.empty();
        if (is_empty && !has_path_prefix)
            out = CURRENT_PATH_RELATIVE_PREFIX;
        else if (is_empty || out.back() != '/')
            out += '/';

        bool found_entry = false;

        // PERF: avoid std::directory_iterator as it allocates a lot,
        // and this function scans a ton of files.
        DirPtr dir{opendir(out.c_str())};
        do
        {
            // NOTE: it also returns '.' and '..'. Let's pretend it doesn't.
            // It's not mandated to be thread-safe by POSIX but glibc says "readdir..modern implementations..calls
            // are..thread-safe".
            dirent* read_dir = readdir(dir.get()); // NOLINT(concurrency-mt-unsafe)
            if (read_dir == nullptr)
                break;
            if (lunaticvibes::iequals(read_dir->d_name, segment))
            {
                found_entry = true;
                out += read_dir->d_name;
                break;
            }
        } while (true);

        segments_traversed += 1;
        if (!found_entry)
        {
            out += segment;
            break;
        }
    }

    for (; segments_traversed < segments.size(); ++segments_traversed)
    {
        out += '/';
        out += segments[segments_traversed];
    }

    if (!has_path_prefix)
        out.erase(0, CURRENT_PATH_RELATIVE_PREFIX.length());

    return PathFromUTF8(out);
}

#endif // _WIN32

Path convertLR2Path(const std::string& lr2path, const Path& relative_path)
{
    return convertLR2Path(lr2path, lunaticvibes::s(relative_path.u8string()));
}

Path convertLR2Path(const std::string& lr2path, const std::string& relative_path_utf8)
{
    return convertLR2Path(lr2path, std::string_view(relative_path_utf8));
}

Path convertLR2Path(const std::string& lr2path, const char* relative_path_utf8)
{
    return convertLR2Path(lr2path, std::string_view(relative_path_utf8));
}

Path convertLR2Path(const std::string& lr2path, std::string_view relative_path_utf8)
{
    std::string_view raw = lunaticvibes::trim(relative_path_utf8, "\"");
    if (raw.empty())
        return {};
    if (auto p = PathFromUTF8(raw); p.is_absolute())
        return p;

    std::string_view prefix = raw.substr(0, 2);
    if (prefix == "./" || prefix == ".\\")
        raw = raw.substr(2);
    else if (prefix[0] == '/' || prefix[0] == '\\')
        raw = raw.substr(1);

    std::string path;
    if (lunaticvibes::iequals(raw.substr(0, 8), "LR2files"))
    {
        path = lr2path;
        path += Path::preferred_separator;
    }
    path += raw;

#ifndef _WIN32
    return lunaticvibes::resolve_windows_path(std::move(path));
#else
    return PathFromUTF8(path);
#endif // _WIN32
}

Path PathFromUTF8(std::string_view s)
{
    // Casting char to char8_t should be UB (unlike the other way around), but
    // `_GLIBCXX20_DEPRECATED_SUGGEST("path((const char8_t*)&*source)")`.
    return {std::u8string_view{reinterpret_cast<const char8_t*>(s.data()), s.size()}};
}

void preciseSleep(long long sleep_ns)
{
    if (sleep_ns <= 0)
        return;

    using namespace std::chrono;
    using namespace std::chrono_literals;

#ifdef _WIN32
    static constexpr auto nsInMs = std::chrono::duration_cast<nanoseconds>(milliseconds{1}).count();

    static HANDLE timer = CreateWaitableTimer(nullptr, FALSE, nullptr);
    while (sleep_ns > nsInMs)
    {
        LARGE_INTEGER due{};
        due.QuadPart = -int64_t((sleep_ns - sleep_ns % nsInMs) / 100); // wrap to 1ms

        const auto start = high_resolution_clock::now();
        SetWaitableTimerEx(timer, &due, 0, nullptr, nullptr, nullptr, 0);
        WaitForSingleObjectEx(timer, INFINITE, TRUE);
        const auto end = high_resolution_clock::now();

        const auto observed = duration_cast<nanoseconds>(end - start).count();
        sleep_ns -= observed;
    }

    // spin lock
    const auto end = high_resolution_clock::now() + nanoseconds{sleep_ns};
    while (high_resolution_clock::now() < end)
        std::this_thread::yield();
#else
    // Spin locking time tests for 1000hz desired update rate on my Linux machine (chown2):
    //  <1000us (1ms) - 100% core utilization, 999-1000hz update rate.
    //  <100us        - 4.5-6% core utilization, 998-999hz update rate.
    //  <10us         - same as below.
    //  <1us          - same as below.
    //  None          - 0.6% core utilization, 946-947hz update rate.
    // By these results spin lock for under 100us would be perfect, however when I add it, the game start dropping FPS
    // when pressing keys.
    // TODO: increase sleep precision.
    const auto duration = std::chrono::nanoseconds(sleep_ns);
    std::this_thread::sleep_for(duration);
#endif
}

double normalizeLinearGrowth(double prev, double curr)
{
    if (prev == -1.0)
        return 0.0;
    if (curr == -1.0)
        return 0.0;

    LVF_DEBUG_ASSERT(prev >= 0.0 && prev <= 1.0);
    LVF_DEBUG_ASSERT(curr >= 0.0 && curr <= 1.0);

    double delta = curr - prev;
    if (prev > 0.8 && curr < 0.2)
        delta += 1.0;
    else if (prev < 0.2 && curr > 0.8)
        delta -= 1.0;

    LVF_DEBUG_ASSERT(delta >= -1.0 && delta <= 1.0);
    return delta;
}

// Try to ignore UTF-8.
// E.g. isspace(), tolower(), etc can only handle ASCII characters.
static unsigned char limit_to_ascii(int c)
{
    if (c < 0 || c > 127)
        return 0u;
    return static_cast<unsigned char>(c);
}

void lunaticvibes::trim_in_place(std::string& s)
{
    static auto not_space = [](int c) { return !std::isspace(limit_to_ascii(c)); };
    s.erase(s.begin(), r::find_if(s, not_space));
    s.erase(r::find_if(r::reverse_view{s}, not_space).base(), s.end());
}
