add_library(fmod SHARED IMPORTED GLOBAL)

if(WIN32)
    if(VCPKG_TARGET_TRIPLET MATCHES "^arm64-")
        set(FMOD_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/ARM64)
    else()
        set(FMOD_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/x64)
    endif()

    set_target_properties(
        fmod
        PROPERTIES
            IMPORTED_LOCATION_DEBUG ${FMOD_LIB_DIR}/fmodL.dll
            IMPORTED_IMPLIB_DEBUG ${FMOD_LIB_DIR}/fmodL_vc.lib
            IMPORTED_LOCATION ${FMOD_LIB_DIR}/fmod.dll
            IMPORTED_IMPLIB ${FMOD_LIB_DIR}/fmod_vc.lib
    )
elseif(LINUX)
    set_target_properties(
        fmod
        PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/x64/libfmod.so.14
            IMPORTED_LOCATION_DEBUG
                ${CMAKE_CURRENT_LIST_DIR}/lib/x64/libfmodL.so.14
    )
else()
    message(FATAL_ERROR "No bundled FMOD for this platform")
endif()

target_include_directories(fmod INTERFACE ${CMAKE_CURRENT_LIST_DIR}/inc)
