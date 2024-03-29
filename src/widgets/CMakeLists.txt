# Public headers
string(TOLOWER ${MLN_QT_NAME}Widgets MLN_QT_WIDGETS_LOWERCASE)
set(Widgets_Headers
    export_widgets.hpp
    gl_widget.hpp
)
# Header generation
mln_umbrella_header_preprocess(${MLN_QT_WIDGETS_LOWERCASE} ${MLN_QT_NAME}Widgets HeaderOut)
list(APPEND Widgets_Headers_Generated ${HeaderOut})
foreach(Header ${Widgets_Headers})
    mln_header_preprocess(${Header} "${MLN_QT_NAME}Widgets" HeaderOut)
    list(APPEND Widgets_Headers_Generated ${HeaderOut})
endforeach()
set(Widgets_Headers ${MLN_QT_WIDGETS_LOWERCASE}.hpp ${Widgets_Headers} ${Widgets_Headers_Generated})

# Make a Qt library
if(COMMAND qt_add_library)
    if(MLN_QT_STATIC)
        qt_add_library(Widgets STATIC)
    else()
        qt_add_library(Widgets)
    endif()
else()
    if(MLN_QT_STATIC)
        add_library(Widgets STATIC)
    else()
        add_library(Widgets SHARED)
    endif()
endif()
add_library(${MLN_QT_NAME}::Widgets ALIAS Core)

target_sources(
    Widgets
    PRIVATE
        ${Widgets_Headers}
        gl_widget.cpp
        gl_widget_p.hpp

        $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    Widgets
    PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME ${MLN_QT_NAME}Widgets
        VERSION ${MLN_QT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${Widgets_Headers}"
)

# Qt MOC
if(Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
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
if(Qt6_FOUND)
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
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
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

# Development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(Widgets PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
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
