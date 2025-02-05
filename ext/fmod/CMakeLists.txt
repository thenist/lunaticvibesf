add_library(fmod SHARED IMPORTED GLOBAL)

if(WIN32)
    set_target_properties(
        fmod
        PROPERTIES
            IMPORTED_LOCATION_DEBUG ${CMAKE_CURRENT_LIST_DIR}/lib/x64/fmodL.dll
            IMPORTED_IMPLIB_DEBUG ${CMAKE_CURRENT_LIST_DIR}/lib/x64/fmodL_vc.lib
            IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/x64/fmod.dll
            IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/lib/x64/fmod_vc.lib
    )
elseif(LINUX)
    set_target_properties(
        fmod
        PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/x64/libfmod.so.13
            IMPORTED_LOCATION_DEBUG
                ${CMAKE_CURRENT_LIST_DIR}/lib/x64/libfmodL.so.13
    )
else()
    message(FATAL_ERROR "No bundled FMOD for this platform")
endif()

target_include_directories(fmod INTERFACE ${CMAKE_CURRENT_LIST_DIR}/inc)
