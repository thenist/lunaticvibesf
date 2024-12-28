find_package(CURL REQUIRED)
find_package(plog CONFIG REQUIRED)
find_package(re2 CONFIG REQUIRED)
find_package(taocpp-json CONFIG REQUIRED)

add_library(
    common
    STATIC
    assert.cpp
    asynclooper.cpp
    encoding.cpp
    fraction.cpp
    hash.cpp
    log.cpp
    str_utils.cpp
    thread_pool.cpp
    types.cpp
    utils.cpp
    keymap.cpp
    sysutil.cpp
    sysutil_win.cpp
    sysutil_linux.cpp
    chartformat/chartformat.cpp
    chartformat/chartformat_bms.cpp
    entry/entry_folder.cpp
    entry/entry_random_song.cpp
    entry/entry_song.cpp
    difficultytable/difficultytable.cpp
    difficultytable/table_bms.cpp
    coursefile/lr2crs.cpp
)

target_include_directories(common PRIVATE ${PROJECT_INCLUDE_DIR})

set_target_properties(common PROPERTIES CXX_STANDARD 20)

target_link_libraries(
    common
    PUBLIC CURL::libcurl
    PUBLIC plog::plog
    PUBLIC re2::re2
    PUBLIC taocpp::json
    PRIVATE tinyfiledialogs
)

if(LINUX)
    target_compile_options(
        common
        # Set by SDL2 pkg-config. Needed to fix -Winvalid-pch `not used because `_REENTRANT' is defined`.
        PUBLIC -D_REENTRANT
    )
    find_package(OpenSSL REQUIRED)
    target_link_libraries(common PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

target_precompile_headers(common PUBLIC pch.h)
