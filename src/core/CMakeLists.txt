# Core MapLibre Native definitions
set(MLN_WITH_QT ON CACHE BOOL "Build MapLibre Native Core with Qt platform" FORCE)
set(MLN_QT_LIBRARY_ONLY ON CACHE BOOL "Build only MapLibre Native Core Qt bindings libraries" FORCE)
set(MLN_CORE_PATH "${CMAKE_SOURCE_DIR}/vendor/maplibre-native" CACHE STRING "MapLibre Native Core source path" FORCE)

# Build MapLibre Native Core
add_subdirectory(
    ${MLN_CORE_PATH}  # Source directory
    MapLibreCore      # Binary directory
    EXCLUDE_FROM_ALL
)

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

# Public headers
string(TOLOWER ${MLN_QT_CORE_LIB} MLN_QT_CORE_LIB_LOWERCASE)
set(Core_Headers
    export.hpp
    map.hpp
    settings.hpp
    types.hpp
    utils.hpp
)
# Header generation
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_CORE_LIB}/${MLN_QT_CORE_LIB}" "#include \"${MLN_QT_CORE_LIB_LOWERCASE}.hpp\"")
list(APPEND Core_Headers_Generated "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_CORE_LIB}/${MLN_QT_CORE_LIB}")
foreach(Header ${Core_Headers})
    string(SUBSTRING ${Header} 0 1 HeaderFirstLetter)
    string(TOUPPER ${HeaderFirstLetter} HeaderFirstLetter)
    string(REGEX REPLACE "^.(.*)" "${HeaderFirstLetter}\\1" HeaderOut "${Header}")
    string(REGEX REPLACE "\.hpp" "" HeaderOut "${HeaderOut}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_CORE_LIB}/${HeaderOut}" "#include \"${Header}\"")
    list(APPEND Core_Headers_Generated "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_CORE_LIB}/${HeaderOut}")
endforeach()
set(Core_Headers ${MLN_QT_CORE_LIB_LOWERCASE}.hpp ${Core_Headers} ${Core_Headers_Generated})

# Make a Qt library
if(COMMAND qt_add_library)
    qt_add_library(${MLN_QT_CORE_LIB})
else()
    add_library(${MLN_QT_CORE_LIB} SHARED)
endif()

# Main sources
target_sources(
    ${MLN_QT_CORE_LIB}
    PRIVATE
    ${Core_Headers}
    conversion_p.hpp
    geojson.cpp
    geojson_p.hpp
    map_observer.cpp
    map_observer_p.hpp
    map_p.hpp
    map_renderer.cpp
    map_renderer_p.hpp
    map.cpp
    renderer_backend.cpp
    renderer_backend_p.hpp
    renderer_observer_p.hpp
    scheduler.cpp
    scheduler_p.hpp
    settings.cpp
    types.cpp
    utils.cpp
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    ${MLN_QT_CORE_LIB} PROPERTIES
    AUTOMOC ON
    EXPORT_NAME ${MLN_QT_CORE_LIB}
    OUTPUT_NAME ${MLN_QT_CORE_LIB}
    VERSION ${MLN_QT_VERSION}
    SOVERSION ${MLN_QT_VERSION_COMPATIBILITY}
    PUBLIC_HEADER "${Core_Headers}"
)

# Qt MOC
if (Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(${MLN_QT_CORE_LIB} "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    ${MLN_QT_CORE_LIB}
    PRIVATE
    QT_BUILD_MAPLIBRE_LIB
)
# static?
# target_compile_definitions(
#     ${MLN_QT_CORE_LIB}
#     PUBLIC QT_MAPLIBRE_STATIC
# )

# Common include directories
target_include_directories(
    ${MLN_QT_CORE_LIB}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${MLN_CORE_PATH}/src
)

# Common link libraries
target_link_libraries(
    ${MLN_QT_CORE_LIB}
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
    PRIVATE
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:mbgl-core-interface>
        $<BUILD_INTERFACE:mbgl-core>
)

# Apple specifics
if (APPLE)
    set_target_properties(
        ${MLN_QT_CORE_LIB} PROPERTIES
        FRAMEWORK ON
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_CORE_LIB}
        MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        ${MLN_QT_CORE_LIB}
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_CORE_LIB}.framework>
    )
endif()

# Configuration and installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(CMAKECONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${MLN_QT_CORE_LIB}/)

configure_package_config_file(
    "config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_CORE_LIB}Config.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    PATH_VARS CMAKE_INSTALL_PREFIX CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_CORE_LIB}ConfigVersion.cmake
    VERSION ${MLN_QT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(EXPORT ${MLN_QT_CORE_LIB}Targets
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    COMPONENT development)

export(EXPORT ${MLN_QT_CORE_LIB}Targets)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_CORE_LIB}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_CORE_LIB}ConfigVersion.cmake"
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    COMPONENT development)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include/mbgl
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT development
)

install(TARGETS ${MLN_QT_CORE_LIB}
        EXPORT ${MLN_QT_CORE_LIB}Targets
        # Explicit set of DESTINATION is needed for older CMake versions.
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_CORE_LIB}"
)
