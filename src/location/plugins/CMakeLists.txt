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
set_property(TARGET ${MLN_QT_GEOSERVICES_PLUGIN} PROPERTY AUTOMOC ON)
set_property(TARGET ${MLN_QT_GEOSERVICES_PLUGIN} PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/geoservices")

# QtLocation plugin include directories
target_include_directories(
    ${MLN_QT_GEOSERVICES_PLUGIN}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
)

# Qt Location plugin linked libraries
target_link_libraries(${MLN_QT_GEOSERVICES_PLUGIN}
    PRIVATE
    ${MLN_QT_LOCATION_LIB}
    Qt::Location
    Qt::LocationPrivate)

# QtLocation plugin installation
install(TARGETS ${MLN_QT_GEOSERVICES_PLUGIN}
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
    set_property(TARGET ${MLN_QT_QML_PLUGIN} PROPERTY AUTOMOC ON)
    set_property(TARGET ${MLN_QT_QML_PLUGIN} PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/legacy/qmldir.in" "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/qmldir" @ONLY)
endif()

# QtLocation QML extension plugin include directories
target_include_directories(
    ${MLN_QT_QML_PLUGIN}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
)

# QtLocation QML extension plugin linked libraries
target_link_libraries(${MLN_QT_QML_PLUGIN}
    PRIVATE
    ${MLN_QT_LOCATION_LIB}
    Qt::LocationPrivate
)

# QtLocation QML extension plugin installation
install(TARGETS ${MLN_QT_QML_PLUGIN}
        LIBRARY DESTINATION "qml/QtLocation/MapLibre"
)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/qmldir"
    DESTINATION "qml/QtLocation/MapLibre"
    COMPONENT development)

if (COMMAND qt_add_qml_module)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/QtLocation/MapLibre/${MLN_QT_QML_PLUGIN}.qmltypes"
    DESTINATION "qml/QtLocation/MapLibre"
    COMPONENT development)
endif()
