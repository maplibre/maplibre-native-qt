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
        maplibre_quick_item.mm
        plugin.cpp
    RESOURCES
        plugin.json
)

target_link_libraries(maplibre_quick PRIVATE
    Qt6::Quick
    QMapLibre::Core
    "-framework QuartzCore"
)

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
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MapLibre/Quick
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/MapLibre) 