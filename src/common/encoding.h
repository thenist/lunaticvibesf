#pragma once

#include <string>

#include <common/types.h>

namespace lunaticvibes
{
enum class eFileEncoding : uint8_t
{
    LATIN1,
    SHIFT_JIS,
    EUC_KR,
    UTF8,
    UTF32,
};
[[nodiscard]] eFileEncoding getFileEncoding(std::istream& is);
[[nodiscard]] const char* getFileEncodingName(eFileEncoding enc);
void to_utf8(const std::string& str, eFileEncoding fromEncoding, std::string& out);
void utf8_to_utf32(const std::string& str, std::u32string& out);
} // namespace lunaticvibes

using lunaticvibes::eFileEncoding;
using lunaticvibes::getFileEncoding;
using lunaticvibes::getFileEncodingName;
