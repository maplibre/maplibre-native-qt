# Build the MapLibre Quick plugin providing MapLibreView item
set(Plugin_Sources
    maplibre_quick_item.hpp
    plugin.cpp
)

set(CMAKE_AUTOMOC ON)

if(MLN_QT_STATIC)
    qt_add_qml_module(${MLN_QT_QUICK_MODULE}
        STATIC
        URI MapLibre.Quick
        VERSION ${PROJECT_VERSION}
        NO_PLUGIN_OPTIONAL
        CLASS_NAME MapLibreQuickModule
        RESOURCE_PREFIX "/"
        OUTPUT_DIRECTORY "MapLibre/Quick"
        SOURCES ${Plugin_Sources}
        RESOURCES
            plugin.json
    )
else()
    qt_add_qml_module(${MLN_QT_QUICK_MODULE}
        URI MapLibre.Quick
        VERSION ${PROJECT_VERSION}
        NO_PLUGIN_OPTIONAL
        CLASS_NAME MapLibreQuickModule
        RESOURCE_PREFIX "/"
        OUTPUT_DIRECTORY "MapLibre/Quick"
        SOURCES ${Plugin_Sources}
        RESOURCES
            plugin.json
    )
endif()

set_target_properties(
    ${MLN_QT_QUICK_MODULE}
    PROPERTIES
        AUTOMOC ON
)

# Add backend-specific sources based on the selected backend
if(MLN_WITH_OPENGL)
    target_sources(${MLN_QT_QUICK_MODULE} PRIVATE
        maplibre_quick_item_opengl.hpp
        maplibre_quick_item_opengl.cpp
    )
    target_compile_definitions(${MLN_QT_QUICK_MODULE} PRIVATE MLN_RENDER_BACKEND_OPENGL=1)
elseif(MLN_WITH_METAL AND APPLE)
    target_sources(${MLN_QT_QUICK_MODULE} PRIVATE
        maplibre_quick_item_metal.hpp
        maplibre_quick_item_metal.mm
    )
    target_compile_definitions(${MLN_QT_QUICK_MODULE} PRIVATE MLN_RENDER_BACKEND_METAL=1)
elseif(MLN_WITH_VULKAN)
    target_sources(${MLN_QT_QUICK_MODULE} PRIVATE
        maplibre_quick_item_vulkan.hpp
        maplibre_quick_item_vulkan.cpp
    )
    target_compile_definitions(${MLN_QT_QUICK_MODULE} PRIVATE MLN_RENDER_BACKEND_VULKAN=1)
    target_include_directories(${MLN_QT_QUICK_MODULE} SYSTEM PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/vendor/Vulkan-Headers/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/vendor/VulkanMemoryAllocator/include
    )
    # Add RHI include path for Qt 6.10+
    target_include_directories(${MLN_QT_QUICK_MODULE} PRIVATE
        ${Qt6Gui_INCLUDE_DIRS}/QtGui/${Qt6_VERSION}/QtGui
    )
else()
    # Default to OpenGL backend
    target_sources(${MLN_QT_QUICK_MODULE} PRIVATE
        maplibre_quick_item_opengl.hpp
        maplibre_quick_item_opengl.cpp
    )
    target_compile_definitions(maplibre_quick PRIVATE MLN_RENDER_BACKEND_OPENGL=1)
endif()

target_link_libraries(${MLN_QT_QUICK_MODULE} PRIVATE
    Qt6::Quick
    Qt6::Gui
    QMapLibre::Core
    QMapLibre::Location
)

# Add platform-specific frameworks
if(APPLE)
    target_link_libraries(${MLN_QT_QUICK_MODULE} PRIVATE
        "-framework QuartzCore"
    )
endif()

target_include_directories(${MLN_QT_QUICK_MODULE} PRIVATE
    ${PROJECT_SOURCE_DIR}/src/platform/src
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/include
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/maplibre-native-base/include
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/maplibre-native-base/deps/geometry.hpp/include
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/maplibre-native-base/deps/variant/include
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/metal-cpp
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/src
)

# QtQuick QML extension plugin development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(${MLN_QT_QUICK_MODULE} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# QtLocation QML extension plugin installation
install(
    EXPORT ${MLN_QT_NAME}QuickModuleTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)
install(
    TARGETS ${MLN_QT_QUICK_MODULE} ${QmlPluginOutputTargets}
    EXPORT ${MLN_QT_NAME}QuickModuleTargets
    ARCHIVE DESTINATION "qml/MapLibre/Quick"
    LIBRARY DESTINATION "qml/MapLibre/Quick"
    OBJECTS DESTINATION "qml/MapLibre/Quick"
    RUNTIME DESTINATION "qml/MapLibre/Quick"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/Quick/qmldir"
    DESTINATION "qml/MapLibre/Quick"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/Quick/${MLN_QT_QUICK_MODULE}.qmltypes"
    DESTINATION "qml/MapLibre/Quick"
)
