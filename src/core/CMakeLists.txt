# Core MapLibre Native definitions
set(MLN_WITH_QT ON CACHE BOOL "Build MapLibre Native Core with Qt platform" FORCE)
set(MLN_QT_LIBRARY_ONLY ON CACHE BOOL "Build only MapLibre Native Core Qt bindings libraries" FORCE)
set(MLN_CORE_PATH "${CMAKE_SOURCE_DIR}/vendor/maplibre-native" CACHE STRING "MapLibre Native Core source path" FORCE)

# Build MapLibre Native Core
add_subdirectory(
    ${MLN_CORE_PATH}  # Source directory
    MapLibreCore      # Binary directory
    EXCLUDE_FROM_ALL
)

# Public headers
string(TOLOWER ${MLN_QT_NAME} MLN_QT_NAME_LOWERCASE)
set(Core_Headers
    export_core.hpp
    map.hpp
    settings.hpp
    types.hpp
    utils.hpp
)
# Header generation
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_NAME}/${MLN_QT_NAME}" "#include \"${MLN_QT_NAME_LOWERCASE}.hpp\"")
list(APPEND Core_Headers_Generated "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_NAME}/${MLN_QT_NAME}")
foreach(Header ${Core_Headers})
    string(SUBSTRING ${Header} 0 1 HeaderFirstLetter)
    string(TOUPPER ${HeaderFirstLetter} HeaderFirstLetter)
    string(REGEX REPLACE "^.(.*)" "${HeaderFirstLetter}\\1" HeaderOut "${Header}")
    string(REGEX REPLACE "\.hpp" "" HeaderOut "${HeaderOut}")
    string(REPLACE "Export_core" "Export" HeaderOut "${HeaderOut}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_NAME}/${HeaderOut}" "#include \"${Header}\"")
    list(APPEND Core_Headers_Generated "${CMAKE_CURRENT_BINARY_DIR}/include/${MLN_QT_NAME}/${HeaderOut}")
endforeach()
set(Core_Headers ${MLN_QT_NAME_LOWERCASE}.hpp ${Core_Headers} ${Core_Headers_Generated})

# Make a Qt library
if(COMMAND qt_add_library)
    qt_add_library(Core)
else()
    add_library(Core SHARED)
endif()
add_library(${MLN_QT_NAME}::Core ALIAS Core)

# Main sources
target_sources(
    Core
    PRIVATE
        ${Core_Headers}
        conversion_p.hpp
        geojson.cpp
        geojson_p.hpp
        map_observer.cpp
        map_observer_p.hpp
        map_p.hpp
        map_renderer.cpp
        map_renderer_p.hpp
        map.cpp
        renderer_backend.cpp
        renderer_backend_p.hpp
        renderer_observer_p.hpp
        scheduler.cpp
        scheduler_p.hpp
        settings.cpp
        types.cpp
        utils.cpp
)

# Linux/Mac: Set framework, version and headers
set_target_properties(
    Core
    PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME ${MLN_QT_NAME}
        VERSION ${MLN_QT_VERSION}
        SOVERSION ${MLN_QT_VERSION_COMPATIBILITY}
        PUBLIC_HEADER "${Core_Headers}"
)

# Qt MOC
if (Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(Core "moc" ON)
endif()

# Common compile definitions
target_compile_definitions(
    Core
    PRIVATE
        QT_BUILD_MAPLIBRE_CORE_LIB
)
# static?
# target_compile_definitions(
#     Core
#     PUBLIC QT_MAPLIBRE_STATIC
# )

# Common include directories
target_include_directories(
    Core
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${MLN_CORE_PATH}/src
)

# Common link libraries
target_link_libraries(
    Core
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
    PRIVATE
        $<BUILD_INTERFACE:mbgl-compiler-options>
        $<BUILD_INTERFACE:mbgl-core-interface>
        $<BUILD_INTERFACE:mbgl-core>
)
if (NOT MLN_QT_WITH_INTERNAL_SQLITE)
    target_link_libraries(
        Core
        PUBLIC
            Qt${QT_VERSION_MAJOR}::Sql
    )
endif()

# Apple specifics
if (APPLE)
    set_target_properties(
        Core
        PROPERTIES
            FRAMEWORK ON
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.maplibre.${MLN_QT_NAME}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${MLN_QT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MLN_QT_VERSION}
    )
    target_include_directories(
        Core
        INTERFACE
            $<INSTALL_INTERFACE:lib/${MLN_QT_NAME}.framework>
    )
endif()

# Export and installation
install(
    EXPORT ${MLN_QT_NAME}CoreTargets
    NAMESPACE ${MLN_QT_NAMESPACE}
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include/mbgl
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    TARGETS Core
    EXPORT ${MLN_QT_NAME}CoreTargets
    # Explicit set of DESTINATION is needed for older CMake versions.
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${MLN_QT_NAME}"
)
