# Packaging and export
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(CMAKECONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${MLN_QT_NAME}/)

# Components
add_subdirectory(core)

set(MLN_QT_SUPPORTED_COMPONENTS "Core")

if(MLN_QT_WITH_LOCATION)
    add_subdirectory(location)
    list(APPEND MLN_QT_SUPPORTED_COMPONENTS "Location")
    message(STATUS "Building location module")
endif()

if(MLN_QT_WITH_WIDGETS AND MLN_WITH_OPENGL)
    add_subdirectory(widgets)
    list(APPEND MLN_QT_SUPPORTED_COMPONENTS "Widgets")
    message(STATUS "Building widgets module")
endif()

# Always build Quick plugin
add_subdirectory(quick)

# Windows metadata
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/version_info.rc.in"
        "${CMAKE_BINARY_DIR}/version_info.rc"
        @ONLY
    )
endif()

# Setup package and install
configure_package_config_file(
    "config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    PATH_VARS CMAKE_INSTALL_PREFIX CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}ConfigVersion.cmake
    VERSION ${MLN_QT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${MLN_QT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)
