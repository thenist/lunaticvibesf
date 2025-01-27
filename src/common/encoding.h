#pragma once

#include <string>

#include <common/types.h>

enum class eFileEncoding : uint8_t
{
    LATIN1,
    SHIFT_JIS,
    EUC_KR,
    UTF8,
    UTF32,
};
[[nodiscard]] eFileEncoding getFileEncoding(const Path& path);
[[nodiscard]] eFileEncoding getFileEncoding(std::istream& is);
[[nodiscard]] const char* getFileEncodingName(eFileEncoding enc);

[[nodiscard]] std::string to_utf8(const std::string& str, eFileEncoding fromEncoding);
[[nodiscard]] std::string from_utf8(const std::string& input, eFileEncoding toEncoding);

namespace lunaticvibes
{
void to_utf8(const std::string& str, eFileEncoding fromEncoding, std::string& out);
void utf8_to_utf32(const std::string& str, std::u32string& out);
} // namespace lunaticvibes

[[nodiscard]] inline std::u32string utf8_to_utf32(const std::string& str)
{
    std::u32string buf;
    lunaticvibes::utf8_to_utf32(str, buf);
    return buf;
}
