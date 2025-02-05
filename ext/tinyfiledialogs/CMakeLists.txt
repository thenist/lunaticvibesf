add_library(tinyfiledialogs STATIC src/tinyfiledialogs.cpp)
target_include_directories(
    tinyfiledialogs
    SYSTEM
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
)
