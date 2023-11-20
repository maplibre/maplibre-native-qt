# Public headers
string(TOLOWER ${MLN_QT_NAME}Widgets MLN_QT_WIDGETS_LOWERCASE)
set(Widgets_Headers
    export_widgets.hpp
    gl_widget.hpp
)
# Header generation
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_NAME}Widgets/${MLN_QT_NAME}Widgets" "#include \"${MLN_QT_WIDGETS_LOWERCASE}.hpp\"")
list(APPEND Widgets_Headers_Generated "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_NAME}Widgets/${MLN_QT_NAME}Widgets")
foreach(Header ${Widgets_Headers})
    mln_header_preprocess(${Header} "${MLN_QT_NAME}Widgets" HeaderOut)
    list(APPEND Widgets_Headers_Generated ${HeaderOut})
endforeach()
set(Widgets_Headers ${MLN_QT_WIDGETS_LOWERCASE}.hpp ${Widgets_Headers} ${Widgets_Headers_Generated})

# Make a Qt library
if(COMMAND qt_add_library)
    qt_add_library(Widgets)
else()
    add_library(Widgets SHARED)
endif()
add_library(${MLN_QT_NAME}::Widgets ALIAS Core)

target_sources(
    Widgets
    PRIVATE
        ${Widgets_Headers}
        gl_widget.cpp
        gl_widget_p.hpp
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    Widgets
    PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME ${MLN_QT_NAME}Widgets
        VERSION ${MLN_QT_VERSION}
        SOVERSION ${MLN_QT_VERSION_COMPATIBILITY}
        PUBLIC_HEADER "${Widgets_Headers}"
)

# Qt MOC
if (Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(Widgets "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    Widgets
    PRIVATE
        QT_BUILD_MAPLIBRE_WIDGETS_LIB
)

# Common include directories
target_include_directories(
    Widgets
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
)

# Common link libraries
target_link_libraries(
    Widgets
    PUBLIC
        Core
    PRIVATE
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:CompilerOptions>
)
if (Qt6_FOUND)
    target_link_libraries(
        Widgets
        PUBLIC
            Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    )
else()
    target_link_libraries(
        Widgets
        PUBLIC
            Qt${QT_VERSION_MAJOR}::OpenGL
    )
endif()

# Apple specifics
if (APPLE)
    set_target_properties(
        Widgets
        PROPERTIES
            FRAMEWORK ON
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_NAME}Widgets
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        Widgets
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_NAME}Widgets.framework>
    )
endif()

# Export and installation
install(
    EXPORT ${MLN_QT_NAME}WidgetsTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    TARGETS Widgets
    EXPORT ${MLN_QT_NAME}WidgetsTargets
    # Explicit set of DESTINATION is needed for older CMake versions.
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_NAME}Widgets"
)
