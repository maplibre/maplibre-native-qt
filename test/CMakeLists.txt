if(
    CMAKE_SYSTEM_NAME STREQUAL "Android"
    OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten"
    OR CMAKE_SYSTEM_NAME STREQUAL "iOS"
)
    return()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_VERSION VERSION_LESS "3.22.0")
    message(STATUS "Tests on Windows require CMake 3.22 or later and will be disabled")
    return()
endif()

# add_subdirectory(core)
# add_subdirectory(quick)
if(MLN_QT_WITH_LOCATION)
    add_subdirectory(qml)
endif()
# if(MLN_QT_WITH_WIDGETS)
#     add_subdirectory(widgets)
# endif()
