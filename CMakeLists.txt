cmake_minimum_required(VERSION 3.19...3.21)

# Version
file(READ "VERSION.txt" MLN_QT_VERSION)
string(REGEX REPLACE "\n" "" MLN_QT_VERSION "${MLN_QT_VERSION}") # get rid of the newline at the end
set(MLN_QT_VERSION_COMPATIBILITY 3.0.0)
message(STATUS "Version ${MLN_QT_VERSION}")

# Project definition
project(MapLibreNativeQt
    VERSION "${MLN_QT_VERSION}"
    DESCRIPTION "MapLibre Native Qt Bindings"
    LANGUAGES CXX
)

# Common CMake definitions
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Common QMapLibre definitions
set(MLN_QT_CORE_LIB QMapLibre)

# Find Qt
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui Network REQUIRED)  # main dependencies
if (Qt6_FOUND)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS OpenGL OpenGLWidgets REQUIRED)
endif()
message(STATUS "Using Qt${QT_VERSION_MAJOR}")

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(test)
