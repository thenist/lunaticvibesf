#include "thread_pool.h"

#include <cstddef>
#include <thread>

using size_t = std::size_t;

static size_t wanted_thread_count()
{
    if (const auto threads = std::thread::hardware_concurrency(); threads >= 4)
        return threads - 2;
    return 1;
}

std::pair<boost::asio::thread_pool&, std::mutex&> lunaticvibes::get_thread_pool()
{
    static boost::asio::thread_pool thread_pool{wanted_thread_count()};
    static std::mutex m;
    return {thread_pool, m};
}
