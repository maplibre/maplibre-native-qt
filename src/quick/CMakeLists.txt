# Make a Qt library
if(MLN_QT_STATIC)
    qt_add_library(MLNQtQuickPrivate STATIC)
else()
    qt_add_library(MLNQtQuickPrivate)
endif()
add_library(${MLN_QT_NAME}::QuickPrivate ALIAS MLNQtQuickPrivate)

target_sources(
    MLNQtQuickPrivate
    PRIVATE
        texture_node_base.cpp texture_node_base_p.hpp

        $<$<BOOL:${MLN_WITH_OPENGL}>:texture_node_opengl.cpp>
        $<$<BOOL:${MLN_WITH_OPENGL}>:texture_node_opengl_p.hpp>
        $<$<BOOL:${MLN_WITH_VULKAN}>:texture_node_vulkan.cpp>
        $<$<BOOL:${MLN_WITH_VULKAN}>:texture_node_vulkan_p.hpp>
        $<$<BOOL:${MLN_WITH_METAL}>:texture_node_metal.mm>
        $<$<BOOL:${MLN_WITH_METAL}>:texture_node_metal_p.hpp>

        $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/version_info.rc>
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    MLNQtQuickPrivate
    PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME ${MLN_QT_NAME}QuickPrivate
        EXPORT_NAME QuickPrivate
        VERSION ${MLN_QT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Qt MOC
if(Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(MLNQtQuickPrivate "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    MLNQtQuickPrivate
    PRIVATE
        QT_BUILD_MAPLIBRE_QUICKPRIVATE_LIB
)

# Common include directories
target_include_directories(
    MLNQtQuickPrivate
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
        ${MLN_CORE_PATH}/include
        ${MLN_CORE_PATH}/src
        ${MLN_CORE_PATH}/vendor/maplibre-native-base/include
        ${MLN_CORE_PATH}/vendor/maplibre-native-base/deps/geometry.hpp/include
        ${MLN_CORE_PATH}/vendor/maplibre-native-base/deps/variant/include
        $<$<BOOL:${MLN_WITH_VULKAN}>:${MLN_CORE_PATH}/vendor/Vulkan-Headers/include>
        $<$<BOOL:${MLN_WITH_VULKAN}>:${MLN_CORE_PATH}/vendor/VulkanMemoryAllocator/include>
)

# Common link libraries
target_link_libraries(
    MLNQtQuickPrivate
    PUBLIC
        MLNQtCore
        Qt${QT_VERSION_MAJOR}::Quick
    PRIVATE
        $<$<BOOL:${MLN_WITH_OPENGL}>:Qt${QT_VERSION_MAJOR}::OpenGL>
        Qt${QT_VERSION_MAJOR}::Network
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:MLNQtCompilerOptions>
)

# Add QuartzCore framework for Metal support on Apple platforms
if(APPLE AND MLN_WITH_METAL)
    target_link_libraries(
        MLNQtQuickPrivate
        PRIVATE
            "-framework QuartzCore"
    )
endif()

# Apple specifics
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_target_properties(
        MLNQtQuickPrivate
        PROPERTIES
            FRAMEWORK ON
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_NAME}QuickPrivate
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        MLNQtQuickPrivate
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_NAME}QuickPrivate.framework>
    )
endif()

# Development specifics
if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(MLNQtQuickPrivate PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# Export and installation
install(
    EXPORT ${MLN_QT_NAME}QuickPrivateTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    TARGETS MLNQtQuickPrivate
    EXPORT ${MLN_QT_NAME}QuickPrivateTargets
    # Explicit set of DESTINATION is needed for older CMake versions.
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_NAME}QuickPrivate"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/macros.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}QuickMacros.cmake"
    COPYONLY
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}QuickMacros.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

# plugins
add_subdirectory(plugins)
