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
        texture_node_base.cpp texture_node_base.hpp

        $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

# Add backend-specific implementations
if(MLN_WITH_OPENGL)
    target_sources(MLNQtLocation PRIVATE
        texture_node_opengl.cpp texture_node_opengl.hpp
        qgeomap_opengl.cpp qgeomap_opengl.hpp)
endif()

if(MLN_WITH_VULKAN)
    target_sources(MLNQtLocation PRIVATE
        texture_node_vulkan.cpp texture_node_vulkan.hpp
        qgeomap_vulkan.cpp qgeomap_vulkan.hpp)
endif()

if(MLN_WITH_METAL AND APPLE)
    target_sources(MLNQtLocation PRIVATE
        texture_node_metal.mm texture_node_metal.hpp
        qgeomap_metal.mm qgeomap_metal.hpp)
endif()


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
        $<$<BOOL:${MLN_WITH_OPENGL}>:MLN_RENDER_BACKEND_OPENGL>
        $<$<BOOL:${MLN_WITH_METAL}>:MLN_RENDER_BACKEND_METAL>
        $<$<BOOL:${MLN_WITH_VULKAN}>:MLN_RENDER_BACKEND_VULKAN>
)

# Common include directories
target_include_directories(
    MLNQtLocation
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/core/style
        ${CMAKE_BINARY_DIR}/src/core/include
)

# Common link libraries
target_link_libraries(
    MLNQtLocation
    PUBLIC
        MLNQtCore
    PRIVATE
        $<$<BOOL:${Qt6_FOUND}>:Qt${QT_VERSION_MAJOR}::OpenGL>
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::LocationPrivate
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:MLNQtCompilerOptions>
)

# Add QuartzCore framework for Metal support on Apple platforms
if(MLN_WITH_METAL AND APPLE)
    target_link_libraries(
        MLNQtLocation
        PRIVATE
            "-framework QuartzCore"
    )
endif()

# Add Vulkan-specific includes
if(MLN_WITH_VULKAN)
    target_include_directories(
        MLNQtLocation
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/platform/src
            ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include
            ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/vendor
            ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/vendor/maplibre-native-base/include
            ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/vendor/maplibre-native-base/deps/geometry.hpp/include
            ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/vendor/maplibre-native-base/deps/variant/include
            ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/src
            ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/vendor/VulkanMemoryAllocator/include
            ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/vendor/Vulkan-Headers/include
    )
endif()

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
