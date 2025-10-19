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

if(MLN_WITH_METAL)
    set(MLN_QT_TEST_RENDERER metal)
elseif(MLN_WITH_VULKAN)
    set(MLN_QT_TEST_RENDERER vulkan)
elseif(MLN_WITH_OPENGL)
    set(MLN_QT_TEST_RENDERER opengl)
endif()

if(MLN_QT_WITH_QUICK_PLUGIN)
    add_subdirectory(quick)
endif()
if(MLN_QT_WITH_LOCATION)
    add_subdirectory(qml)
endif()
if(MLN_QT_WITH_WIDGETS)
    add_subdirectory(widgets)
endif()
