# Make a Qt library
if(MLN_QT_STATIC)
    qt_add_library(MLNQtLocation STATIC)
else()
    qt_add_library(MLNQtLocation)
endif()
add_library(${MLN_QT_NAME}::Location ALIAS MLNQtLocation)

target_sources(
    MLNQtLocation
    PRIVATE
        qgeomap.cpp qgeomap.hpp qgeomap_p.hpp
        qt_mapping_engine.cpp qt_mapping_engine.hpp
        style_change_utils.cpp style_change_utils_p.hpp

        $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    MLNQtLocation
    PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME ${MLN_QT_NAME}Location
        EXPORT_NAME Location
        VERSION ${MLN_QT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Qt MOC
if(Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(MLNQtLocation "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    MLNQtLocation
    PRIVATE
        QT_BUILD_MAPLIBRE_LOCATION_LIB
)

# Common include directories
target_include_directories(
    MLNQtLocation
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/core/style
        ${CMAKE_SOURCE_DIR}/src/quick
        ${CMAKE_BINARY_DIR}/src/core/include
        $<$<BOOL:${MLN_WITH_VULKAN}>:${MLN_CORE_PATH}/vendor/Vulkan-Headers/include>
        $<$<BOOL:${MLN_WITH_VULKAN}>:${MLN_CORE_PATH}/vendor/VulkanMemoryAllocator/include>
)

# Common link libraries
target_link_libraries(
    MLNQtLocation
    PUBLIC
        MLNQtCore
    PRIVATE
        MLNQtQuickPrivate
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::LocationPrivate
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:MLNQtCompilerOptions>
)

# Apple specifics
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_target_properties(
        MLNQtLocation
        PROPERTIES
            FRAMEWORK ON
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_NAME}Location
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        MLNQtLocation
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_NAME}Location.framework>
    )
endif()

# Development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(MLNQtLocation PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# Export and installation
install(
    EXPORT ${MLN_QT_NAME}LocationTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    TARGETS MLNQtLocation
    EXPORT ${MLN_QT_NAME}LocationTargets
    # Explicit set of DESTINATION is needed for older CMake versions.
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_NAME}Location"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/macros.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}LocationMacros.cmake"
    COPYONLY
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}LocationMacros.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

# plugins
add_subdirectory(plugins)
