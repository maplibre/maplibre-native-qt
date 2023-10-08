# Public headers
string(TOLOWER ${MLN_QT_WIDGETS_LIB} MLN_QT_WIDGETS_LIB_LOWERCASE)
set(Widgets_Headers
    gl_widget.hpp
)
# Header generation
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_WIDGETS_LIB}/${MLN_QT_WIDGETS_LIB}" "#include \"${MLN_QT_WIDGETS_LIB_LOWERCASE}.hpp\"")
list(APPEND Widgets_Headers_Generated "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_WIDGETS_LIB}/${MLN_QT_WIDGETS_LIB}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_WIDGETS_LIB}/GLWidget" "#include \"gl_widget.hpp\"")
list(APPEND Widgets_Headers_Generated "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_WIDGETS_LIB}/GLWidget")
set(Widgets_Headers ${MLN_QT_WIDGETS_LIB_LOWERCASE}.hpp ${Widgets_Headers} ${Widgets_Headers_Generated})

# Make a Qt library
if(COMMAND qt_add_library)
    qt_add_library(${MLN_QT_WIDGETS_LIB})
else()
    add_library(${MLN_QT_WIDGETS_LIB} SHARED)
endif()

target_sources(
    ${MLN_QT_WIDGETS_LIB}
    PRIVATE
    ${Widgets_Headers}
    gl_widget.cpp
    gl_widget_p.hpp
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    ${MLN_QT_WIDGETS_LIB} PROPERTIES
    AUTOMOC ON
    EXPORT_NAME ${MLN_QT_WIDGETS_LIB}
    OUTPUT_NAME ${MLN_QT_WIDGETS_LIB}
    VERSION ${MLN_QT_VERSION}
    SOVERSION ${MLN_QT_VERSION_COMPATIBILITY}
    PUBLIC_HEADER "${Widgets_Headers}"
)

# Qt MOC
if (Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(${MLN_QT_WIDGETS_LIB} "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    ${MLN_QT_WIDGETS_LIB}
    PRIVATE
    QT_BUILD_MAPLIBRE_LIB
)

# Common include directories
target_include_directories(
    ${MLN_QT_WIDGETS_LIB}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
)

# Common link libraries
target_link_libraries(
    ${MLN_QT_WIDGETS_LIB}
    PUBLIC
    ${MLN_QT_CORE_LIB}
)
if (Qt6_FOUND)
    target_link_libraries(
        ${MLN_QT_WIDGETS_LIB}
        PUBLIC
            Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    )
else()
    target_link_libraries(
        ${MLN_QT_WIDGETS_LIB}
        PUBLIC
            Qt${QT_VERSION_MAJOR}::OpenGL
    )
endif()

# Apple specifics
if (APPLE)
    set_target_properties(
        ${MLN_QT_WIDGETS_LIB} PROPERTIES
        FRAMEWORK ON
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_WIDGETS_LIB}
        MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        ${MLN_QT_WIDGETS_LIB}
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_WIDGETS_LIB}.framework>
    )
endif()

# Configuration and installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(CMAKECONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${MLN_QT_WIDGETS_LIB}/)

configure_package_config_file(
    "config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_WIDGETS_LIB}Config.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    PATH_VARS CMAKE_INSTALL_PREFIX CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_WIDGETS_LIB}ConfigVersion.cmake
    VERSION ${MLN_QT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(EXPORT ${MLN_QT_WIDGETS_LIB}Targets
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    COMPONENT development)

export(EXPORT ${MLN_QT_WIDGETS_LIB}Targets)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_WIDGETS_LIB}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_WIDGETS_LIB}ConfigVersion.cmake"
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    COMPONENT development)

install(TARGETS ${MLN_QT_WIDGETS_LIB}
        EXPORT ${MLN_QT_WIDGETS_LIB}Targets
        # Explicit set of DESTINATION is needed for older CMake versions.
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_WIDGETS_LIB}"
)
