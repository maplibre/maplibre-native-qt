cmake_minimum_required(VERSION 3.19)

project(QMapLibreExampleQuick VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick REQUIRED)
qt_standard_project_setup()

if(QT_KNOWN_POLICY_QTP0002)
    qt_policy(SET QTP0002 NEW)
endif()

find_package(QMapLibre COMPONENTS Location REQUIRED)

qt_add_executable(QMapLibreExampleQuick
    main.cpp
)

qt_add_qml_module(QMapLibreExampleQuick
    URI Example
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
        main.qml
)

set_target_properties(QMapLibreExampleQuick PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

target_link_libraries(QMapLibreExampleQuick
    PRIVATE
        Qt::Quick
        QMapLibre::Location
)

qmaplibre_location_setup_plugins(QMapLibreExampleQuick)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    get_property(_qmlImport TARGET QMapLibreExampleQuick PROPERTY QT_QML_IMPORT_PATH)
    add_custom_target(deploy
        COMMAND macdeployqt QMapLibreExampleQuick.app -qmldir=${CMAKE_SOURCE_DIR} -qmlimport=${_qmlImport}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(deploy QMapLibreExampleQuick)
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
