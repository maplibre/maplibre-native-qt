# Make a Qt library
if(COMMAND qt_add_library)
    qt_add_library(Location)
else()
    add_library(Location SHARED)
endif()
add_library(${MLN_QT_NAME}::Location ALIAS Location)

target_sources(
    Location
    PRIVATE
        qgeomap.cpp qgeomap.hpp qgeomap_p.hpp
        qt_mapping_engine.cpp qt_mapping_engine.hpp
        stylechange.cpp stylechange_p.h
        texture_node.cpp texture_node.hpp
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    Location
    PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME ${MLN_QT_NAME}Location
        VERSION ${MLN_QT_VERSION}
        SOVERSION ${MLN_QT_VERSION_COMPATIBILITY}
)

# Qt MOC
if (Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(Location "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    Location
    PRIVATE
        QT_BUILD_MAPLIBRE_LOCATION_LIB
)

# Common include directories
target_include_directories(
    Location
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
)

# Common link libraries
target_link_libraries(
    Location
    PRIVATE
        Core
        $<$<BOOL:${Qt6_FOUND}>:Qt::OpenGL>
        Qt::Network
        Qt::LocationPrivate
)

# Apple specifics
if (APPLE)
    set_target_properties(
        Location
        PROPERTIES
            FRAMEWORK ON
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_NAME}Location
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        Location
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_NAME}Location.framework>
    )
endif()

# Export and installation
install(
    EXPORT ${MLN_QT_NAME}LocationTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    TARGETS Location
    EXPORT ${MLN_QT_NAME}LocationTargets
    # Explicit set of DESTINATION is needed for older CMake versions.
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_NAME}Location"
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/macros.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}LocationMacros.cmake" COPYONLY)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}LocationMacros.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

# plugins
add_subdirectory(plugins)
