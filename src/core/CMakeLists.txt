# Core MapLibre Native definitions
set(MLN_WITH_QT ON CACHE BOOL "Build MapLibre Native Core with Qt platform" FORCE)
set(MLN_QT_LIBRARY_ONLY ON CACHE BOOL "Build only MapLibre Native Core Qt bindings libraries" FORCE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(MLN_QT_WITH_INTERNAL_SQLITE ON CACHE BOOL "Build MapLibre Native Core with internal sqlite" FORCE)
endif()
set(MLN_CORE_PATH "${PROJECT_SOURCE_DIR}/vendor/maplibre-native" CACHE STRING "MapLibre Native Core source path" FORCE)

# Build MapLibre Native Core
add_subdirectory(
    ${MLN_CORE_PATH}  # Source directory
    MapLibreCore      # Binary directory
    EXCLUDE_FROM_ALL
)

# Public headers
string(TOLOWER ${MLN_QT_NAME} MLN_QT_NAME_LOWERCASE)
set(MLNQtCore_Headers
    export_core.hpp
    map.hpp
    settings.hpp
    types.hpp
    utils.hpp

    style/style_parameter.hpp
    style/filter_parameter.hpp
    style/layer_parameter.hpp
    style/source_parameter.hpp
)
# Header generation
mln_umbrella_header_preprocess(${MLN_QT_NAME_LOWERCASE} ${MLN_QT_NAME} HeaderOut)
list(APPEND MLNQtCore_Headers_Generated ${HeaderOut})
foreach(Header ${MLNQtCore_Headers})
    mln_header_preprocess(${Header} ${MLN_QT_NAME} HeaderOut)
    list(APPEND MLNQtCore_Headers_Generated ${HeaderOut})
endforeach()
set(MLNQtCore_Headers ${MLN_QT_NAME_LOWERCASE}.hpp ${MLNQtCore_Headers} ${MLNQtCore_Headers_Generated})

# Make a Qt library
if(COMMAND qt_add_library)
    if(MLN_QT_STATIC)
        qt_add_library(MLNQtCore STATIC)
    else()
        qt_add_library(MLNQtCore)
    endif()
else()
    if(MLN_QT_STATIC)
        add_library(MLNQtCore STATIC)
    else()
        add_library(MLNQtCore SHARED)
    endif()
endif()
add_library(${MLN_QT_NAME}::Core ALIAS MLNQtCore)

# Main sources
target_sources(
    MLNQtCore
    PRIVATE
        ${MLNQtCore_Headers}
        conversion_p.hpp
        geojson.cpp geojson_p.hpp
        map_observer.cpp map_observer_p.hpp
        map_renderer.cpp map_renderer_p.hpp
        map.cpp map_p.hpp
        settings.cpp settings_p.hpp
        types.cpp
        utils.cpp

        style/style_parameter.cpp
        style/filter_parameter.cpp
        style/layer_parameter.cpp
        style/source_parameter.cpp

        style/style_change.cpp style/style_change_p.hpp
        style/style_change_utils.cpp style/style_change_utils_p.hpp
        style/image_style_change.cpp style/image_style_change_p.hpp
        style/layer_style_change.cpp style/layer_style_change_p.hpp
        style/source_style_change.cpp style/source_style_change_p.hpp

        $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    MLNQtCore
    PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME ${MLN_QT_NAME}
        EXPORT_NAME Core
        VERSION ${MLN_QT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${MLNQtCore_Headers}"
)

# Qt MOC
if(Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(MLNQtCore "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    MLNQtCore
    PRIVATE
        QT_BUILD_MAPLIBRE_CORE_LIB
)

# Common include directories
target_include_directories(
    MLNQtCore
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/style
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${MLN_CORE_PATH}/src
        ${MLN_CORE_PATH}/platform/qt/src
)

# Common link libraries
target_link_libraries(
    MLNQtCore
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
    PRIVATE
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:mbgl-core>
        $<BUILD_INTERFACE:MLNQtCompilerOptions>
)
if(NOT MLN_QT_WITH_INTERNAL_SQLITE)
    target_link_libraries(
        MLNQtCore
        PUBLIC
            Qt${QT_VERSION_MAJOR}::Sql
    )
endif()
foreach(target ${MLN_QT_VENDOR_LIBRARIES})
    target_link_libraries(
        MLNQtCore
            PRIVATE
                $<BUILD_INTERFACE:${target}>
    )
endforeach()

# Apple specifics
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_target_properties(
        MLNQtCore
        PROPERTIES
            FRAMEWORK ON
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_NAME}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        MLNQtCore
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_NAME}.framework>
    )
endif()

# Development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(MLNQtCore PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# Export and installation
install(
    EXPORT ${MLN_QT_NAME}CoreTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include/mbgl
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    TARGETS MLNQtCore
    EXPORT ${MLN_QT_NAME}CoreTargets
    # Explicit set of DESTINATION is needed for older CMake versions.
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_NAME}"
)
