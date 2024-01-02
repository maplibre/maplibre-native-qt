# QtLocation plugin
if(COMMAND qt_add_plugin)
    qt_add_plugin(
        ${MLN_QT_GEOSERVICES_PLUGIN}
        CLASS_NAME QGeoServiceProviderFactoryMapLibre
        OUTPUT_TARGETS GeoServicesPluginOutputTargets
        PLUGIN_TYPE geoservices
        qgeoserviceproviderplugin.cpp qgeoserviceproviderplugin.hpp
    )
else()
    add_library(
        ${MLN_QT_GEOSERVICES_PLUGIN}
        SHARED
        qgeoserviceproviderplugin.cpp qgeoserviceproviderplugin.hpp
    )
    target_compile_definitions(${MLN_QT_GEOSERVICES_PLUGIN} PRIVATE QT_PLUGIN)
endif()

# QtLocation plugin extra target properties
set_target_properties(
    ${MLN_QT_GEOSERVICES_PLUGIN}
    PROPERTIES
        AUTOMOC ON
        EXPORT_NAME PluginGeoServices
        LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/geoservices>
        RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/geoservices>
)

# QtLocation plugin include directories
target_include_directories(
    ${MLN_QT_GEOSERVICES_PLUGIN}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/core/style
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
        $<BUILD_INTERFACE:CompilerOptions>
)

# QtLocation plugin development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(${MLN_QT_GEOSERVICES_PLUGIN} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# QtLocation plugin installation
install(
    EXPORT ${MLN_QT_NAME}LocationPluginGeoServicesTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)
install(
    TARGETS ${MLN_QT_GEOSERVICES_PLUGIN} ${GeoServicesPluginOutputTargets}
    EXPORT ${MLN_QT_NAME}LocationPluginGeoServicesTargets
    ARCHIVE DESTINATION "plugins/geoservices"
    LIBRARY DESTINATION "plugins/geoservices"
    OBJECTS DESTINATION "plugins/geoservices"
    RUNTIME DESTINATION "plugins/geoservices"
)

# QtLocation QML extension plugin
set(Plugin_Sources
    qml_types.hpp
    declarative_style.cpp declarative_style.hpp
    declarative_style_parameter.hpp
    declarative_layer_parameter.cpp declarative_layer_parameter.hpp
    declarative_source_parameter.cpp declarative_source_parameter.hpp
)

if(COMMAND qt_add_qml_module)
    qt_add_qml_module(
        ${MLN_QT_QML_PLUGIN}
        URI QtLocation.MapLibre
        VERSION ${PROJECT_VERSION}
        PLUGIN_TARGET ${MLN_QT_QML_PLUGIN}
        NO_PLUGIN_OPTIONAL
        NO_GENERATE_QMLDIR
        CLASS_NAME QtLocationMapLibreQmlModule
        RESOURCE_PREFIX "/"
        OUTPUT_DIRECTORY "QtLocation/MapLibre"
        OUTPUT_TARGETS QmlPluginOutputTargets
        SOURCES ${Plugin_Sources}
    )
    set_target_properties(
        ${MLN_QT_QML_PLUGIN}
        PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre>
            RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre>
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/qmldir.in"
        "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/qmldir"
        @ONLY
    )
else()
    add_library(
        ${MLN_QT_QML_PLUGIN}
        SHARED
        ${Plugin_Sources}
        legacy/qml_module.cpp
        legacy/qml_registration.cpp
    )
    target_compile_definitions(
        ${MLN_QT_QML_PLUGIN}
        PRIVATE
            QT_PLUGIN
    )
    target_include_directories(
        ${MLN_QT_QML_PLUGIN}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_target_properties(
        ${MLN_QT_QML_PLUGIN}
        PROPERTIES
            AUTOMOC ON
            LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre>
            RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre>
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/legacy/qmldir.in"
        "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/qmldir"
        @ONLY
    )
endif()
set_property(TARGET ${MLN_QT_QML_PLUGIN} PROPERTY EXPORT_NAME PluginQml)

# QtLocation QML extension plugin include directories
target_include_directories(
    ${MLN_QT_QML_PLUGIN}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/core/style
        ${CMAKE_BINARY_DIR}/src/core/include
)

# QtLocation QML extension plugin linked libraries
target_link_libraries(
    ${MLN_QT_QML_PLUGIN}
    PRIVATE
        Location
        Qt${QT_VERSION_MAJOR}::LocationPrivate
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:CompilerOptions>
)

# QtLocation QML extenstion plugin development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(${MLN_QT_QML_PLUGIN} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# QtLocation QML extension plugin installation
install(
    EXPORT ${MLN_QT_NAME}LocationPluginQmlTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)
install(
    TARGETS ${MLN_QT_QML_PLUGIN} ${QmlPluginOutputTargets}
    EXPORT ${MLN_QT_NAME}LocationPluginQmlTargets
    ARCHIVE DESTINATION "qml/QtLocation/MapLibre"
    LIBRARY DESTINATION "qml/QtLocation/MapLibre"
    OBJECTS DESTINATION "qml/QtLocation/MapLibre"
    RUNTIME DESTINATION "qml/QtLocation/MapLibre"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/qmldir"
    DESTINATION "qml/QtLocation/MapLibre"
)

if(COMMAND qt_add_qml_module)
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/${MLN_QT_QML_PLUGIN}.qmltypes"
        DESTINATION "qml/QtLocation/MapLibre"
    )
endif()
