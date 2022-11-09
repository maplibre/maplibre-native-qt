cmake_minimum_required(VERSION 3.16)

file(READ "VERSION.txt" MBGL_QML_VERSION)
string(REGEX REPLACE "\n" "" MBGL_QML_VERSION "${MBGL_QML_VERSION}") # get

project(MapLibreGLQmlPlugin
    VERSION "${MBGL_QML_VERSION}"
    DESCRIPTION "MapLibre GL Qt Location QML Plugin"
    LANGUAGES CXX
)

# Make sure we use the fixed BASE argument of qt_add_resource.
set(QT_USE_FIXED_QT_ADD_RESOURCE_BASE TRUE)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core) # special case
# WebAssembly not supported
if(WASM)
    message(NOTICE "Skipping the build as the condition \"NOT WASM\" is not met.")
    return()
endif()

qt_prepare_standalone_project()

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Location Positioning Network OpenGL) # special case

# Core QMapLibreGL definitions
set(MBGL_WITH_QT ON CACHE BOOL "Build MapLibre GL Qt version" FORCE)
set(MBGL_QT_LIBRARY_ONLY ON CACHE BOOL "Build only the library part of QMapLibreGL" FORCE)
set(MBGL_QT_INSIDE_PLUGIN ON CACHE BOOL "Build QMapLibreGL as OBJECT library, so it can be bundled into separate single plugin lib." FORCE)
set(MBGL_ENGINE_PATH "dependencies/maplibre-gl-native" CACHE STRING "QMapLibreGL source path" FORCE)

add_subdirectory(
    ${MBGL_ENGINE_PATH} # Source directory
    QMapLibreGL           # Binary directory
    EXCLUDE_FROM_ALL
)

qt_internal_add_plugin(QGeoServiceProviderFactoryMapLibreGLPlugin
    OUTPUT_NAME qtgeoservices_maplibregl
    CLASS_NAME QGeoServiceProviderFactoryMapLibreGL
    PLUGIN_TYPE geoservices
    SOURCES
        src/qgeoserviceproviderpluginmaplibregl.cpp src/qgeoserviceproviderpluginmaplibregl.h
        src/qgeomappingmanagerenginemaplibregl.cpp src/qgeomappingmanagerenginemaplibregl.h
        src/qgeomapmaplibregl.cpp src/qgeomapmaplibregl.h src/qgeomapmaplibregl_p.h
        src/qmaplibreglstylechange.cpp src/qmaplibreglstylechange_p.h
        src/qsgmaplibreglnode.cpp src/qsgmaplibreglnode.h
    INCLUDE_DIRECTORIES
        ${MBGL_ENGINE_PATH}/platform/qt/include
    DEFINES
        QT_MAPLIBREGL_STATIC
    LIBRARIES
        Qt::OpenGL
        Qt::Network
        Qt::LocationPrivate
        "$<BUILD_INTERFACE:qmaplibregl>"
        "$<BUILD_INTERFACE:mbgl-core>"
        "$<BUILD_INTERFACE:mbgl-vendor-parsedate>"
        "$<BUILD_INTERFACE:mbgl-vendor-nunicode>"
        "$<BUILD_INTERFACE:mbgl-vendor-csscolorparser>"
)

set_property(TARGET QGeoServiceProviderFactoryMapLibreGLPlugin PROPERTY INTERFACE_QT_MAJOR_VERSION 6)

qt_add_resources(plugin_resource_files resources/maplibregl.qrc)

qt_internal_extend_target(QGeoServiceProviderFactoryMapLibreGLPlugin
    SOURCES
        ${plugin_resource_files}
)
