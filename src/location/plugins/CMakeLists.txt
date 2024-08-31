# QtLocation plugin
if(COMMAND qt_add_plugin)
    if(MLN_QT_STATIC)
        qt_add_plugin(
            ${MLN_QT_GEOSERVICES_PLUGIN}
            STATIC
            CLASS_NAME QGeoServiceProviderFactoryMapLibre
            OUTPUT_TARGETS GeoServicesPluginOutputTargets
            PLUGIN_TYPE geoservices
        )
    else()
        qt_add_plugin(
            ${MLN_QT_GEOSERVICES_PLUGIN}
            CLASS_NAME QGeoServiceProviderFactoryMapLibre
            OUTPUT_TARGETS GeoServicesPluginOutputTargets
            PLUGIN_TYPE geoservices
        )
    endif()
else()
    if(MLN_QT_STATIC)
        add_library(${MLN_QT_GEOSERVICES_PLUGIN} STATIC)
    else()
        add_library(${MLN_QT_GEOSERVICES_PLUGIN} SHARED)
    endif()
    target_compile_definitions(${MLN_QT_GEOSERVICES_PLUGIN} PRIVATE QT_PLUGIN)
endif()

target_sources(
    ${MLN_QT_GEOSERVICES_PLUGIN}
    PRIVATE
        qgeoserviceproviderplugin.cpp qgeoserviceproviderplugin.hpp

        $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

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
        MLNQtLocation
        Qt${QT_VERSION_MAJOR}::Location
        Qt${QT_VERSION_MAJOR}::LocationPrivate
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:MLNQtCompilerOptions>
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
    declarative_filter_parameter.cpp declarative_filter_parameter.hpp
    declarative_image_parameter.cpp declarative_image_parameter.hpp
    declarative_layer_parameter.cpp declarative_layer_parameter.hpp
    declarative_source_parameter.cpp declarative_source_parameter.hpp

    $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

if(COMMAND qt_add_qml_module)
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
else()
    if(MLN_QT_STATIC)
        add_library(
            ${MLN_QT_QML_PLUGIN}
            STATIC
            ${Plugin_Sources}
            legacy/qml_module.cpp
            legacy/qml_registration.cpp
        )
    else()
        add_library(
            ${MLN_QT_QML_PLUGIN}
            SHARED
            ${Plugin_Sources}
            legacy/qml_module.cpp
            legacy/qml_registration.cpp
        )
    endif()
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
            LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/MapLibre>
            RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}/MapLibre>
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/legacy/qmldir.in"
        "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/qmldir"
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
        MLNQtLocation
        Qt${QT_VERSION_MAJOR}::LocationPrivate
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:MLNQtCompilerOptions>
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
    ARCHIVE DESTINATION "qml/MapLibre"
    LIBRARY DESTINATION "qml/MapLibre"
    OBJECTS DESTINATION "qml/MapLibre"
    RUNTIME DESTINATION "qml/MapLibre"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/qmldir"
    DESTINATION "qml/MapLibre"
)

if(COMMAND qt_add_qml_module)
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/MapLibre/${MLN_QT_QML_PLUGIN}.qmltypes"
        DESTINATION "qml/MapLibre"
    )
endif()
