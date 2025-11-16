cmake_minimum_required(VERSION 3.19...3.25)

# Version
file(READ "VERSION.txt" MLN_QT_VERSION)
string(STRIP "${MLN_QT_VERSION}" MLN_QT_VERSION) # get rid of the whitespace
set(MLN_QT_VERSION_COMPATIBILITY 4.0.0)
message(STATUS "Version ${MLN_QT_VERSION}")

# Project definition
project(
    maplibre-native-qt
    VERSION "${MLN_QT_VERSION}"
    DESCRIPTION "MapLibre Native Qt Bindings"
    LANGUAGES CXX C
)

# Common CMake definitions
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Common QMapLibre definitions
set(MLN_QT_NAME QMapLibre)
set(MLN_QT_NAMESPACE ${MLN_QT_NAME}::)
set(MLN_QT_GEOSERVICES_PLUGIN qtgeoservices_maplibre)
set(MLN_QT_QML_PLUGIN declarative_maplibre)
set(MLN_QT_QML_PLUGIN_LOCATION declarative_maplibre_locationplugin)

include(cmake/helpers.cmake)

# Options
option(MLN_QT_WITH_QUICK_PLUGIN "Build QMapLibreQuick plugin" ON)
option(MLN_QT_WITH_LOCATION "Build QMapLibreLocation" ON)
option(MLN_QT_WITH_WIDGETS "Build QMapLibreWidgets" ON)
option(MLN_QT_STATIC "Build QMapLibre staticaly (force static build with Qt6)" OFF)
option(MLN_QT_WITH_COVERAGE "Build QMapLibre with code coverage collection" OFF)
option(MLN_QT_WITH_CLANG_TIDY "Build QMapLibre with clang-tidy checks enabled" OFF)
option(MLN_QT_WITH_LTO "Build QMapLibre with Link-Time Optimization" OFF)
option(MLN_QT_WITH_RENDERER_DEBUGGING "Build QMapLibre with renderer debugging" OFF)

if(MLN_QT_WITH_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_result OUTPUT ipo_output)
    if(ipo_result)
        message(STATUS "Building with LTO/IPO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "LTO/IPO is not supported: ${ipo_output}")
    endif()
endif()

# Try to use ECM (extra-cmake-modules) to query directories used by Qt
find_package(ECM 6.0.0 NO_MODULE)
if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
    include(KDEInstallDirs)
    set(MLN_QT_PLUGIN_DIR ${KDE_INSTALL_QTPLUGINDIR})
    set(MLN_QML_MODULE_DIR ${KDE_INSTALL_QMLDIR})
else()
    message(STATUS "Can not query plugin directories used by Qt installation because extra-cmake-modules is not available. Using defaults.")
    include(GNUInstallDirs)
    set(MLN_QT_PLUGIN_DIR ${KDE_INSTALL_QTPLUGINDIR} "${CMAKE_INSTALL_LIBDIR}/plugins/")
    set(MLN_QML_MODULE_DIR ${KDE_INSTALL_QMLDIR} "${CMAKE_INSTALL_LIBDIR}/qml")
endif()

# Find Qt
if("${QT_VERSION_MAJOR}" STREQUAL "")
    find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
else()
    find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
endif()
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui Network REQUIRED) # main dependencies
if(NOT MLN_QT_WITH_INTERNAL_SQLITE)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Sql REQUIRED)
endif()
if(MLN_QT_WITH_QUICK_PLUGIN)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Quick REQUIRED) # quick
endif()
if(MLN_QT_WITH_LOCATION)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Location REQUIRED) # location
    if(QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 9)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LocationPrivate REQUIRED)
    endif()
endif()
if(MLN_QT_WITH_WIDGETS)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
    if(QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 9)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS GuiPrivate REQUIRED)
    endif()
endif()

if("${QT_VERSION}" STREQUAL "")
    message(FATAL_ERROR "Qt version unknown")
endif()

message(STATUS "Using Qt${QT_VERSION_MAJOR} (${QT_VERSION})")

# clang-tidy
if(MLN_QT_WITH_CLANG_TIDY)
    find_program(CLANG_TIDY_COMMAND
        NAMES clang-tidy
        HINTS ENV CLANG_TIDY_PATH
    )
    if(NOT CLANG_TIDY_COMMAND)
        message(FATAL_ERROR "ENABLE_CLANG_TIDY is ON but clang-tidy is not found!")
    else()
        message(STATUS "Found clang-tidy at ${CLANG_TIDY_COMMAND}")
    endif()
    # TODO: there are options which are only available on GCC (e.g. -Werror=maybe-uninitialized),
    # that's why we need to disable this `unknown-warning-option` here. We could check if current
    # compiler supports particular flag before enabling it.
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_COMMAND};--extra-arg=-Wno-error=unknown-warning-option")
endif()

# Platform-specifics
if(ANDROID)
    message(STATUS "Building for ABI: ${ANDROID_ABI}")
    set(CMAKE_STATIC_LIBRARY_SUFFIX "_${ANDROID_ABI}.a")
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_DEBUG_POSTFIX "_debug")
elseif(MSVC OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_DEBUG_POSTFIX "d")
endif()

# Common configuration
add_library(MLNQtCompilerOptions INTERFACE)
target_compile_options(
    MLNQtCompilerOptions
    INTERFACE
        $<$<BOOL:${MLN_QT_WITH_COVERAGE}>:--coverage>
)
target_link_libraries(
    MLNQtCompilerOptions
    INTERFACE
        $<$<BOOL:${MLN_QT_WITH_COVERAGE}>:--coverage>
)

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(test)

# Coverage
if(MLN_QT_WITH_COVERAGE)
    include(cmake/coverage.cmake)
    setup_target_for_coverage(coverage ${CMAKE_CTEST_COMMAND} "${CMAKE_BINARY_DIR}/coverage" "-V")
endif()

# Packaging
if(MLN_WITH_METAL)
    set(CPACK_RENDERER_NAME Metal)
elseif(MLN_WITH_VULKAN)
    set(CPACK_RENDERER_NAME Vulkan)
elseif(MLN_WITH_OPENGL)
    set(CPACK_RENDERER_NAME OpenGL)
endif()

set(CPACK_GENERATOR TBZ2)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPACK_SYSTEM_NAME "macOS")
else()
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
endif()
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}_v${PROJECT_VERSION}_Qt${QT_VERSION}_${CPACK_SYSTEM_NAME}_${CPACK_RENDERER_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}_v${PROJECT_VERSION}_Source")
set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES
    "/docs/"
    "/geometry-test-data/"
    "\\\\.clang"
    "\\\\.DS_Store"
    "\\\\.git"
    "\\\\.mason"
    "\\\\.pre-commit-config.yaml"
    "codecov.yml"
)

include(CPack)
