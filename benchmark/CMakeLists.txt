find_package(benchmark REQUIRED)
add_executable(lvf_benchmark benchmark_main.cpp encoding_benchmark.cpp)
set_target_properties(lvf_benchmark PROPERTIES CXX_STANDARD 23)
target_link_libraries(
    lvf_benchmark
    PRIVATE benchmark::benchmark
    PRIVATE gamelib
)
