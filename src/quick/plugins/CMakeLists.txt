# QtQuick QML extension plugin
set(Plugin_Sources
    map_quick_item.cpp map_quick_item.cpp

    $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

if(MLN_QT_STATIC)
    qt_add_qml_module(
        ${MLN_QT_QML_PLUGIN}
        STATIC
        URI MapLibre
        VERSION ${PROJECT_VERSION}
        PLUGIN_TARGET ${MLN_QT_QML_PLUGIN}
        NO_PLUGIN_OPTIONAL
        NO_GENERATE_QMLDIR
        CLASS_NAME MapLibreQmlModule
        RESOURCE_PREFIX "/"
        OUTPUT_DIRECTORY "MapLibre"
        OUTPUT_TARGETS QmlPluginOutputTargets
        SOURCES ${Plugin_Sources}
    )
else()
    qt_add_qml_module(
        ${MLN_QT_QML_PLUGIN}
        URI MapLibre
        VERSION ${PROJECT_VERSION}
        PLUGIN_TARGET ${MLN_QT_QML_PLUGIN}
        NO_PLUGIN_OPTIONAL
        NO_GENERATE_QMLDIR
        CLASS_NAME MapLibreQmlModule
        RESOURCE_PREFIX "/"
        OUTPUT_DIRECTORY "MapLibre"
        OUTPUT_TARGETS QmlPluginOutputTargets
        SOURCES ${Plugin_Sources}
    )
endif()
set_target_properties(
    ${MLN_QT_QML_PLUGIN}
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/MapLibre>
        RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/MapLibre>
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/qmldir.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/qmldir"
    @ONLY
)
set_property(TARGET ${MLN_QT_QML_PLUGIN} PROPERTY EXPORT_NAME PluginQml)

# QtQuick QML extension plugin include directories
target_include_directories(
    ${MLN_QT_QML_PLUGIN}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/core/style
        ${CMAKE_SOURCE_DIR}/src/quick
        ${CMAKE_BINARY_DIR}/src/core/include
        $<$<BOOL:${MLN_WITH_VULKAN}>:${MLN_CORE_PATH}/vendor/Vulkan-Headers/include>
        $<$<BOOL:${MLN_WITH_VULKAN}>:${MLN_CORE_PATH}/vendor/VulkanMemoryAllocator/include>
)

# QtQuick QML extension plugin linked libraries
target_link_libraries(
    ${MLN_QT_QML_PLUGIN}
    PRIVATE
        MLNQtQuickPrivate
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:MLNQtCompilerOptions>
)

# QtQuick QML extenstion plugin development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(${MLN_QT_QML_PLUGIN} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# QtQuick QML extension plugin installation
install(
    EXPORT ${MLN_QT_NAME}QuickPluginQmlTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)
install(
    TARGETS ${MLN_QT_QML_PLUGIN} ${QmlPluginOutputTargets}
    EXPORT ${MLN_QT_NAME}QuickPluginQmlTargets
    ARCHIVE DESTINATION "${MLN_QML_MODULE_DIR}/MapLibre"
    LIBRARY DESTINATION "${MLN_QML_MODULE_DIR}/MapLibre"
    OBJECTS DESTINATION "${MLN_QML_MODULE_DIR}/MapLibre"
    RUNTIME DESTINATION "${MLN_QML_MODULE_DIR}/MapLibre"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/qmldir"
    DESTINATION "${MLN_QML_MODULE_DIR}/MapLibre"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/${MLN_QT_QML_PLUGIN}.qmltypes"
    DESTINATION "${MLN_QML_MODULE_DIR}/MapLibre"
)
