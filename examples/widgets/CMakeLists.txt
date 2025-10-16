# Public headers
string(TOLOWER ${MLN_QT_NAME}Widgets MLN_QT_WIDGETS_LOWERCASE)
set(MLNQtWidgets_Headers
    export_widgets.hpp
    rhi_widget.hpp
)
# Header generation
mln_umbrella_header_preprocess(${MLN_QT_WIDGETS_LOWERCASE} ${MLN_QT_NAME}Widgets HeaderOut)
list(APPEND MLNQtWidgets_Headers_Generated ${HeaderOut})
foreach(Header ${MLNQtWidgets_Headers})
    mln_header_preprocess(${Header} "${MLN_QT_NAME}Widgets" HeaderOut)
    list(APPEND MLNQtWidgets_Headers_Generated ${HeaderOut})
endforeach()
set(MLNQtWidgets_Headers ${MLN_QT_WIDGETS_LOWERCASE}.hpp ${MLNQtWidgets_Headers} ${MLNQtWidgets_Headers_Generated})

# Make a Qt library
if(COMMAND qt_add_library)
    if(MLN_QT_STATIC)
        qt_add_library(MLNQtWidgets STATIC)
    else()
        qt_add_library(MLNQtWidgets)
    endif()
else()
    if(MLN_QT_STATIC)
        add_library(MLNQtWidgets STATIC)
    else()
        add_library(MLNQtWidgets SHARED)
    endif()
endif()
add_library(${MLN_QT_NAME}::Widgets ALIAS MLNQtWidgets)

target_sources(
    MLNQtWidgets
    PRIVATE
        ${MLNQtWidgets_Headers}
        rhi_widget.cpp
        rhi_widget_p.hpp

        $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    MLNQtWidgets
    PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME ${MLN_QT_NAME}Widgets
        EXPORT_NAME Widgets
        VERSION ${MLN_QT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${MLNQtWidgets_Headers}"
)

# Qt MOC
if(Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(MLNQtWidgets "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    MLNQtWidgets
    PRIVATE
        QT_BUILD_MAPLIBRE_WIDGETS_LIB
)

# Common include directories
target_include_directories(
    MLNQtWidgets
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
)

# Add Qt RHI private headers for QRhiWidget integration
if(Qt6_FOUND)
    target_include_directories(
        MLNQtWidgets
        PRIVATE
            ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
            /home/birks/Qt/6.10.0/gcc_arm64/include/QtGui/6.10.0/QtGui
    )
endif()

# Common link libraries
target_link_libraries(
    MLNQtWidgets
    PUBLIC
        MLNQtCore
    PRIVATE
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:MLNQtCompilerOptions>
)
if(Qt6_FOUND)
    target_link_libraries(
        MLNQtWidgets
        PUBLIC
            Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    )
    if(TARGET Qt${QT_VERSION_MAJOR}::GuiPrivate)
        target_link_libraries(
            MLNQtWidgets
            PRIVATE
                Qt${QT_VERSION_MAJOR}::GuiPrivate
        )
    endif()
else()
    target_link_libraries(
        MLNQtWidgets
        PUBLIC
            Qt${QT_VERSION_MAJOR}::OpenGL
    )
endif()

# Apple specifics
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_target_properties(
        MLNQtWidgets
        PROPERTIES
            FRAMEWORK ON
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_NAME}Widgets
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        MLNQtWidgets
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_NAME}Widgets.framework>
    )
endif()

# Development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(MLNQtWidgets PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# Export and installation
install(
    EXPORT ${MLN_QT_NAME}WidgetsTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    TARGETS MLNQtWidgets
    EXPORT ${MLN_QT_NAME}WidgetsTargets
    # Explicit set of DESTINATION is needed for older CMake versions.
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_NAME}Widgets"
)
