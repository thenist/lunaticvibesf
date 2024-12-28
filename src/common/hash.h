#pragma once

#include <cstring>
#include <filesystem>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace lunaticvibes
{
[[nodiscard]] std::string bin2hex(std::span<const unsigned char> bin);
[[nodiscard]] inline std::string bin2hex(std::span<const char> bin)
{
    return bin2hex({reinterpret_cast<const unsigned char*>(bin.data()), bin.size()});
};
constexpr void hex2bin(std::string_view hex, std::span<unsigned char> buf)
{
    auto tolower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
    // LANDMINE. TODO: add size check;
    for (size_t i = 0, j = 0; i < hex.length() - 1; i += 2, j++)
    {
        unsigned char& c = buf[j];
        unsigned char c1 = tolower(hex[i]);
        unsigned char c2 = tolower(hex[i + 1]);
        c += (c1 + ((c1 >= 'a') ? (10 - 'a') : (-'0'))) << 4;
        c += (c2 + ((c2 >= 'a') ? (10 - 'a') : (-'0')));
    }
}
[[nodiscard]] std::string hex2bin(std::string_view hex);

} // namespace lunaticvibes

template <size_t Len> class Hash
{
private:
    unsigned char data[Len] = {0};
    bool set = false;

public:
    constexpr Hash() = default;
    constexpr explicit Hash(const std::string_view hex)
    {
        if (hex.size() != Len * 2)
            throw std::runtime_error("invalid 'hex' length");
        set = true;
        lunaticvibes::hex2bin(hex, data);
    }
    template <size_t HexLen> constexpr explicit Hash(const char (&hex)[HexLen])
    {
        static_assert(HexLen - 1 == Len * 2);
        set = true;
        lunaticvibes::hex2bin(hex, data);
    }

    static constexpr size_t length() { return Len; }
    // TODO: remove this and use std::optional<Hash> where needed.
    [[nodiscard]] constexpr bool empty() const { return !set; }
    [[nodiscard]] std::string hexdigest() const { return lunaticvibes::bin2hex({data, Len}); }
    [[nodiscard]] constexpr const unsigned char* hex() const { return data; }
    void reset()
    {
        set = false;
        memset(data, 0, Len);
    }

    auto operator<=>(const Hash<Len>&) const = default;

    friend struct std::hash<Hash<Len>>;
};

template <size_t Len> struct std::hash<Hash<Len>>
{
    size_t operator()(const Hash<Len>& obj) const
    {
        return std::hash<std::string_view>()({reinterpret_cast<const char*>(obj.data), Len});
    }
};

using HashMD5 = Hash<16>;

HashMD5 md5(std::string_view str);
HashMD5 md5file(const std::filesystem::path& filePath);
