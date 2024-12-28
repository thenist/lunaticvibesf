#include "thread_pool.h"

#include <cstddef>
#include <thread>

using size_t = std::size_t;

std::pair<boost::asio::thread_pool&, std::mutex&> lunaticvibes::get_thread_pool()
{
    static size_t thread_count = 0;
    if (thread_count == 0)
    {
        if (const auto threads = std::thread::hardware_concurrency(); threads >= 4)
            thread_count = threads - 2;
        else
            thread_count = 1;
    }
    static boost::asio::thread_pool thread_pool{thread_count};
    static std::mutex m;
    return {thread_pool, m};
}
