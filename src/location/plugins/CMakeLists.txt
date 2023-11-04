# QtLocation plugin
if(COMMAND qt_add_plugin)
    qt_add_plugin(${MLN_QT_GEOSERVICES_PLUGIN}
        CLASS_NAME QGeoServiceProviderFactoryMapLibre
        PLUGIN_TYPE geoservices
        qgeoserviceproviderplugin.cpp qgeoserviceproviderplugin.hpp
    )
else()
    add_library(${MLN_QT_GEOSERVICES_PLUGIN} SHARED
        qgeoserviceproviderplugin.cpp qgeoserviceproviderplugin.hpp)
    target_compile_definitions(${MLN_QT_GEOSERVICES_PLUGIN} PRIVATE QT_PLUGIN)
endif()

# QtLocation plugin extra target properties
set_target_properties(
    ${MLN_QT_GEOSERVICES_PLUGIN}
    PROPERTIES
        AUTOMOC ON
        EXPORT_NAME PluginGeoServices
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/geoservices"
)

# QtLocation plugin include directories
target_include_directories(
    ${MLN_QT_GEOSERVICES_PLUGIN}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
)

# QtLocation plugin linked libraries
target_link_libraries(
    ${MLN_QT_GEOSERVICES_PLUGIN}
    PRIVATE
        Location
        Qt${QT_VERSION_MAJOR}::Location
        Qt${QT_VERSION_MAJOR}::LocationPrivate
        $<BUILD_INTERFACE:mbgl-compiler-options>
)

# QtLocation plugin installation
install(
    EXPORT ${MLN_QT_NAME}LocationPluginGeoServicesTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)
install(
    TARGETS ${MLN_QT_GEOSERVICES_PLUGIN}
    EXPORT ${MLN_QT_NAME}LocationPluginGeoServicesTargets
    LIBRARY DESTINATION "plugins/geoservices"
)

# QtLocation QML extension plugin
if (COMMAND qt_add_qml_module)
    qt_add_qml_module(${MLN_QT_QML_PLUGIN}
        URI QtLocation.MapLibre
        VERSION ${PROJECT_VERSION}
        PLUGIN_TARGET ${MLN_QT_QML_PLUGIN}
        NO_PLUGIN_OPTIONAL
        CLASS_NAME QtLocationMapLibreQmlModule
        RESOURCE_PREFIX "/"
        OUTPUT_DIRECTORY "QtLocation/MapLibre"
        SOURCES
            qml_types.hpp
    )
else()
    add_library(${MLN_QT_QML_PLUGIN} SHARED qml_types.hpp legacy/qml_module.cpp legacy/qml_registration.cpp)
    target_compile_definitions(${MLN_QT_QML_PLUGIN} PRIVATE QT_PLUGIN)
    target_include_directories(${MLN_QT_QML_PLUGIN} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(
        ${MLN_QT_QML_PLUGIN}
        PROPERTIES
            AUTOMOC ON
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre"
    )
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/legacy/qmldir.in" "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/qmldir" @ONLY)
endif()
set_property(TARGET ${MLN_QT_QML_PLUGIN} PROPERTY EXPORT_NAME PluginQml)

# QtLocation QML extension plugin include directories
target_include_directories(
    ${MLN_QT_QML_PLUGIN}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
)

# QtLocation QML extension plugin linked libraries
target_link_libraries(
    ${MLN_QT_QML_PLUGIN}
    PRIVATE
        Location
        Qt${QT_VERSION_MAJOR}::LocationPrivate
        $<BUILD_INTERFACE:mbgl-compiler-options>
)

# QtLocation QML extension plugin installation
install(
    EXPORT ${MLN_QT_NAME}LocationPluginQmlTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)
install(
    TARGETS ${MLN_QT_QML_PLUGIN}
    EXPORT ${MLN_QT_NAME}LocationPluginQmlTargets
    LIBRARY DESTINATION "qml/QtLocation/MapLibre"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/qmldir"
    DESTINATION "qml/QtLocation/MapLibre"
)

if (COMMAND qt_add_qml_module)
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/${MLN_QT_QML_PLUGIN}.qmltypes"
        DESTINATION "qml/QtLocation/MapLibre"
)
endif()
