#include <common/in_test_mode.h>
#include <common/sysutil.h>

#include <benchmark/benchmark.h>

namespace lunaticvibes
{
bool in_test_mode()
{
    return false;
};
} // namespace lunaticvibes

int main(int argc, char** argv)
{
    SetThreadAsMainThread();
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
