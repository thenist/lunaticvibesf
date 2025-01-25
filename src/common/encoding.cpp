#include "encoding.h"

#include <fstream>
#include <istream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "common/assert.h"
#include "common/log.h"
#include "common/sysutil.h"

static bool is_ascii(const std::string_view str)
{
    for (unsigned char c : str)
    {
        if (c > 0x7f)
            return false;
    }
    return true;
}

static bool is_shiftjis(const std::string_view str)
{
    for (auto it = str.begin(); it != str.end(); ++it)
    {
        uint8_t c = *it;

        // ascii
        if (c <= 0x7f)
            continue;

        // hankaku gana
        if ((c >= 0xa1 && c <= 0xdf))
            continue;

        // JIS X 0208
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xef))
        {
            if (++it == str.end())
                return false;
            uint8_t cc = *it;
            if ((cc >= 0x40 && cc <= 0x7e) || (cc >= 0x80 && cc <= 0xfc))
                continue;
        }

        // user defined
        else if (c >= 0xf0 && c <= 0xfc)
        {
            if (++it == str.end())
                return false;
            uint8_t cc = *it;
            if ((cc >= 0x40 && cc <= 0x7e) || (cc >= 0x80 && cc <= 0xfc))
                continue;
        }

        else
            return false;
    }

    return true;
}

static bool is_euckr(const std::string_view str)
{
    for (auto it = str.begin(); it != str.end(); ++it)
    {
        uint8_t c = *it;

        // ascii
        if (c <= 0x7f)
            continue;

        // euc-jp
        if (c == 0x8e || c == 0x8f)
            return false;

        // shared range
        else if (c >= 0xa1 && c <= 0xfe)
        {
            if (++it == str.end())
                return false;
            uint8_t cc = *it;
            if (cc >= 0xa1 && cc <= 0xfe)
                continue;
        }

        else
            return false;
    }

    return true;
}

static bool is_utf8(const std::string_view str)
{
    for (auto it = str.begin(); it != str.end(); ++it)
    {
        uint8_t c = *it;
        int bytes = 0;

        // invalid
        if ((c & 0b1100'0000) == 0b1000'0000 || (c & 0b1111'1110) == 0b1111'1110)
            return false;

        // 1 byte (ascii)
        else if ((c & 0b1000'0000) == 0)
            continue;

        // 2~6 bytes
        else if ((c & 0b1110'0000) == 0b1100'0000)
            bytes = 2;
        else if ((c & 0b1111'0000) == 0b1110'0000)
            bytes = 3;
        else if ((c & 0b1111'1000) == 0b1111'0000)
            bytes = 4;
        else if ((c & 0b1111'1100) == 0b1111'1000)
            bytes = 5;
        else if ((c & 0b1111'1110) == 0b1111'1100)
            bytes = 6;
        else
            return false;

        while (--bytes)
        {
            if (++it == str.end())
                return false;
            uint8_t cc = *it;
            if ((cc & 0b1100'0000) != 0b10000000)
                return false;
        }
    }

    return true;
}

eFileEncoding getFileEncoding(const Path& path)
{
    std::ifstream fs(path);
    if (fs.fail())
    {
        return eFileEncoding::LATIN1;
    }
    return getFileEncoding(fs);
}

eFileEncoding getFileEncoding(std::istream& is)
{
    std::streampos oldPos = is.tellg();

    is.clear();
    is.seekg(0);

    eFileEncoding enc = eFileEncoding::LATIN1;
    for (std::string buf; std::getline(is, buf);)
    {
        if (is_ascii(buf))
            continue;

        if (is_utf8(buf))
        {
            enc = eFileEncoding::UTF8;
            break;
        }
        if (is_euckr(buf))
        {
            enc = eFileEncoding::EUC_KR;
            break;
        }
        if (is_shiftjis(buf))
        {
            enc = eFileEncoding::SHIFT_JIS;
            break;
        }
    }

    is.clear();
    is.seekg(oldPos);

    if (enc == eFileEncoding::EUC_KR)
    {
        LOG_WARNING << "beep, boop, detected EUC-KR encoding (rare occurrence)";
    }

    return enc;
}

const char* getFileEncodingName(eFileEncoding enc)
{
    switch (enc)
    {
    case eFileEncoding::EUC_KR: return "EUC-KR";
    case eFileEncoding::LATIN1: return "Latin 1";
    case eFileEncoding::SHIFT_JIS: return "Shift JIS";
    case eFileEncoding::UTF8: return "UTF-8";
    default: return "Unknown";
    }
}

std::string to_utf8(const std::string& input, eFileEncoding fromEncoding)
{
    std::string out;
    lunaticvibes::to_utf8(input, fromEncoding, out);
    return out;
}

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static int to_win_codepage(eFileEncoding enc)
{
    switch (enc)
    {
    case eFileEncoding::SHIFT_JIS: return 932;
    case eFileEncoding::EUC_KR: return 949;
    case eFileEncoding::LATIN1: return CP_ACP;
    case eFileEncoding::UTF8: return CP_UTF8;
    }
    lunaticvibes::assert_failed("to_win_codepage");
};

static void convert(const std::string& input, eFileEncoding fromEncoding, eFileEncoding toEncoding, std::string& out)
{
    const int from = to_win_codepage(fromEncoding);
    const int to = to_win_codepage(toEncoding);
    if (from == to)
    {
        out = input;
        return;
    }

    DWORD wide_buf_size = MultiByteToWideChar(from, 0, input.c_str(), -1, nullptr, 0);
    wchar_t* wstr = new wchar_t[wide_buf_size];
    MultiByteToWideChar(from, 0, input.c_str(), -1, wstr, wide_buf_size);

    DWORD narrow_buf_size = WideCharToMultiByte(to, 0, wstr, -1, 0, 0, 0, FALSE);
    char* lstr = new char[narrow_buf_size];
    WideCharToMultiByte(to, 0, wstr, -1, lstr, narrow_buf_size, 0, FALSE);

    out = lstr;

    delete[] wstr;
    delete[] lstr;
}

void lunaticvibes::to_utf8(const std::string& input, eFileEncoding fromEncoding, std::string& out)
{
    convert(input, fromEncoding, eFileEncoding::UTF8, out);
}

std::string from_utf8(const std::string& input, eFileEncoding toEncoding)
{
    std::string out;
    convert(input, eFileEncoding::UTF8, toEncoding, out);
    return out;
}

#else

#include <cerrno>
#include <cstring>
#include <memory>
#include <type_traits>

#include <iconv.h>

static const char* get_iconv_encoding_name(eFileEncoding encoding)
{
    switch (encoding)
    {
    case eFileEncoding::LATIN1: return "ISO-8859-1";
    case eFileEncoding::SHIFT_JIS: return "CP932";
    case eFileEncoding::EUC_KR: return "CP949";
    case eFileEncoding::UTF8: return "UTF-8";
    }
    lunaticvibes::assert_failed("Incorrect eFileEncoding");
}

struct IcdDeleter
{
    void operator()(iconv_t icd)
    {
        int ret = iconv_close(icd);
        if (ret == -1)
        {
            const int error = errno;
            LOG_ERROR << "iconv_close() error: " << safe_strerror(error) << " (" << error << ")";
        }
    }
};
using IcdPtr = std::unique_ptr<std::remove_pointer_t<iconv_t>, IcdDeleter>;

static void convert(std::string_view input, eFileEncoding from, eFileEncoding to, std::string& out)
{
    static thread_local std::map<std::pair<eFileEncoding, eFileEncoding>, IcdPtr> icds;

    auto icd_it = icds.find({from, to});
    if (icd_it == icds.end())
    {
        const auto* source_encoding_name = get_iconv_encoding_name(from);
        const auto* target_encoding_name = get_iconv_encoding_name(to);
        auto icd = IcdPtr(iconv_open(target_encoding_name, source_encoding_name));
        if (reinterpret_cast<size_t>(icd.get()) == static_cast<size_t>(-1))
        {
            const int error = errno;
            LOG_ERROR << "iconv_open() error: " << safe_strerror(error) << " (" << error << ")";
            out = "(conversion descriptor opening error)";
            return;
        }
        icd_it = icds.insert_or_assign({from, to}, std::move(icd)).first;
        LVF_DEBUG_ASSERT(icd_it != icds.end());
    }
    auto icd = icd_it->second.get();

    // PERF: this buffer is MASSIVE. Don't initialize it or memset will dominate runtime.
    char out_buf[1024L * 32L];

    // BRUH-cast.
    char* buf_ptr = const_cast<char*>(input.data());
    size_t buf_len = input.length();
    char* out_ptr = static_cast<char*>(out_buf);
    size_t out_len = sizeof(out_buf);
    const size_t initial_out_len = out_len;

    size_t iconv_ret = iconv(icd, &buf_ptr, &buf_len, &out_ptr, &out_len);
    if (iconv_ret == static_cast<size_t>(-1))
    {
        const int error = errno;
        LOG_ERROR << "iconv() error: " << safe_strerror(error) << " (" << error << ")";
        out = "(conversion error)";
        return;
    }
    const size_t bytes_written = initial_out_len - out_len;

    out = std::string_view{static_cast<char*>(out_buf), bytes_written};

    // "In each series of calls to iconv(), the last should be one with inbuf or *inbuf equal to NULL, in order to flush
    // out any partially converted input".
    iconv_ret = iconv(icd, nullptr, nullptr, nullptr, nullptr);
    if (iconv_ret == static_cast<size_t>(-1))
    {
        const int error = errno;
        LOG_ERROR << "iconv() error: " << safe_strerror(error) << " (" << error << ")";
        out = "(conversion error)";
        return;
    }
}

void lunaticvibes::to_utf8(const std::string& input, eFileEncoding fromEncoding, std::string& buf)
{
    convert(input, fromEncoding, eFileEncoding::UTF8, buf);
}

std::string from_utf8(const std::string& input, eFileEncoding toEncoding)
{
    std::string buf;
    convert(input, eFileEncoding::UTF8, toEncoding, buf);
    return buf;
}

#endif // _WIN32

void lunaticvibes::utf8_to_utf32(const std::string& str, std::u32string& out)
{
    static const auto locale = std::locale("");
    static const auto& facet_u32_u8 = std::use_facet<std::codecvt<char32_t, char, std::mbstate_t>>(locale);
    out.resize(str.size() * facet_u32_u8.max_length(), '\0');

    std::mbstate_t s;
    const char* from_next = &str[0];
    char32_t* to_next = &out[0];

    std::codecvt_base::result res;
    do
    {
        res = facet_u32_u8.in(s, from_next, &str[str.size()], from_next, to_next, &out[out.size()], to_next);

        // skip unconvertiable chars (which is impossible though)
        if (res == std::codecvt_base::error)
            from_next++;

    } while (res == std::codecvt_base::error);

    out.resize(to_next - &out[0]);
}
