# Build the MapLibre Quick plugin providing MapLibreView item

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)
set(CMAKE_CXX_STANDARD 20)

qt_standard_project_setup()

qt_add_qml_module(maplibre_quick
    URI MapLibre.Quick
    VERSION 1.0
    CLASS_NAME MapLibreQuickPlugin
    INSTALL_DIRECTORY MapLibre/Quick
    SOURCES
        maplibre_quick_item.hpp
        maplibre_quick_item_base.hpp
        maplibre_quick_item_base.cpp
        plugin.cpp
    RESOURCES
        plugin.json
)

# Add backend-specific sources based on the selected backend
if(MLN_WITH_OPENGL)
    target_sources(maplibre_quick PRIVATE
        maplibre_quick_item_opengl.hpp
        maplibre_quick_item_opengl.cpp
    )
    target_compile_definitions(maplibre_quick PRIVATE MLN_RENDER_BACKEND_OPENGL=1)
elseif(MLN_WITH_METAL AND APPLE)
    target_sources(maplibre_quick PRIVATE
        maplibre_quick_item_metal.hpp
        maplibre_quick_item_metal.mm
    )
    target_compile_definitions(maplibre_quick PRIVATE MLN_RENDER_BACKEND_METAL=1)
elseif(MLN_WITH_VULKAN)
    target_sources(maplibre_quick PRIVATE
        maplibre_quick_item_vulkan.hpp
        maplibre_quick_item_vulkan.cpp
    )
    target_compile_definitions(maplibre_quick PRIVATE MLN_RENDER_BACKEND_VULKAN=1)
else()
    # Default to OpenGL backend
    target_sources(maplibre_quick PRIVATE
        maplibre_quick_item_opengl.hpp
        maplibre_quick_item_opengl.cpp
    )
    target_compile_definitions(maplibre_quick PRIVATE MLN_RENDER_BACKEND_OPENGL=1)
endif()

target_link_libraries(maplibre_quick PRIVATE
    Qt6::Quick
    QMapLibre::Core
)

# Add platform-specific frameworks
if(APPLE)
    target_link_libraries(maplibre_quick PRIVATE
        "-framework QuartzCore"
    )
endif()

target_include_directories(maplibre_quick PRIVATE
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/qt/src
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/include
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/mapbox-base/include
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/mapbox-base/deps/geometry.hpp/include
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/mapbox-base/deps/variant/include
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/metal-cpp
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/src
)

# Install the whole generated QML module folder (library + qmldir)
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/Quick")
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MapLibre/Quick
            DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/MapLibre)
endif()
