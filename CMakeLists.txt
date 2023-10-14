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
set(MLN_QT_NAME QMapLibre)
set(MLN_QT_NAMESPACE ${MLN_QT_NAME}::)
set(MLN_QT_GEOSERVICES_PLUGIN qtgeoservices_maplibre)
set(MLN_QT_QML_PLUGIN declarative_locationplugin_maplibre)

# Options
set(MLN_QT_WITH_LOCATION ON CACHE BOOL "Build QMapLibreLocation")
set(MLN_QT_WITH_WIDGETS ON CACHE BOOL "Build QMapLibreWidgets")

# Find Qt
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui Network REQUIRED)  # main dependencies
if (NOT MLN_QT_WITH_INTERNAL_SQLITE)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Sql REQUIRED)
endif()
if (MLN_QT_WITH_LOCATION)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Location REQUIRED)     # location
endif()
if (MLN_QT_WITH_WIDGETS)
    if (Qt6_FOUND)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS OpenGLWidgets REQUIRED)
    else()
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS OpenGL REQUIRED)
    endif()
endif()
message(STATUS "Using Qt${QT_VERSION_MAJOR}")

# Debugging & ccache on Windows
if (MSVC)
    foreach(config DEBUG RELWITHDEBINFO)
        foreach(lang C CXX)
            set(flags_var "CMAKE_${lang}_FLAGS_${config}")
            string(REPLACE "/Zi" "/Z7" ${flags_var} "${${flags_var}}")
            set(${flags_var} "${${flags_var}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endif()

# Platform-specifics
if(ANDROID)
    message(STATUS "Building for ABI: ${ANDROID_ABI}")
    set(CMAKE_STATIC_LIBRARY_SUFFIX "_${ANDROID_ABI}.a")
elseif(CMAKE_SYSTEM_NAME STREQUAL iOS)
    set(CMAKE_DEBUG_POSTFIX "_debug")
elseif(MSVC OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_DEBUG_POSTFIX "d")
endif()

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(test)
