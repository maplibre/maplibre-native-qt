cmake_minimum_required(VERSION 3.19...3.21)

project(QMapLibreExampleQuick VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick REQUIRED)
qt_standard_project_setup()

if(QT_KNOWN_POLICY_QTP0002)
    qt_policy(SET QTP0002 NEW)
endif()

find_package(QMapLibre COMPONENTS Core REQUIRED)

qt_add_executable(
    QMapLibreExampleQuick
    main.cpp
)

qt_add_qml_module(
    QMapLibreExampleQuick
    URI Example
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
        main.qml
)

set_target_properties(
    QMapLibreExampleQuick
    PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
)

target_link_libraries(
    QMapLibreExampleQuick
    PRIVATE
        Qt6::Quick
        Qt6::Gui
        QMapLibre::Core
)

# Check if Vulkan is enabled
if(MLN_WITH_VULKAN)
    target_compile_definitions(QMapLibreExampleQuick PRIVATE MLN_WITH_VULKAN)
endif()

target_include_directories(
    QMapLibreExampleQuick
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/platform/qt/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/vendor
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/vendor/maplibre-native-base/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/vendor/maplibre-native-base/deps/variant/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/vendor/maplibre-native-base/deps/geometry.hpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/vendor/metal-cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/maplibre-native/src
)

install(
    TARGETS QMapLibreExampleQuick
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET QMapLibreExampleQuick
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(
        IMPORTED_RUNTIME_ARTIFACTS QMapLibre::Core
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)

    add_android_openssl_libraries(QMapLibreExampleQuick)
endif()

# Packaging
set(CPACK_GENERATOR TBZ2)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPACK_SYSTEM_NAME "macOS")
else()
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
endif()

include(CPack)
