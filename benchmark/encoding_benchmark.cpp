#include <common/encoding.h>

#include <benchmark/benchmark.h>
#include <string>

static void EncodingToUtf8(benchmark::State& state)
{
    const auto in = std::string(256, 'a');
    std::string out;
    for (auto _ : state)
    {
        lunaticvibes::to_utf8(in, eFileEncoding::SHIFT_JIS, out);
    }
}

static void EncodingToUtf32(benchmark::State& state)
{
    const auto in = std::string(256, 'a');
    std::u32string out;
    for (auto _ : state)
    {
        lunaticvibes::utf8_to_utf32(in, out);
    }
}

BENCHMARK(EncodingToUtf8);
BENCHMARK(EncodingToUtf32);
