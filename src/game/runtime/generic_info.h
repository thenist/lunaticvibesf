#pragma once

#include <common/asynclooper.h>

#include <atomic>
#include <ctime>

inline std::atomic<unsigned> gFrameCount[2]{0};
constexpr size_t FRAMECOUNT_IDX_FPS = 0;
constexpr size_t FRAMECOUNT_IDX_INPUT = 1;

class GenericInfoUpdater : public AsyncLooper
{
public:
    GenericInfoUpdater(unsigned rate);

private:
    void loop();
};
