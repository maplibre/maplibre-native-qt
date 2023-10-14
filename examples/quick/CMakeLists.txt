cmake_minimum_required(VERSION 3.19)

project(QMapLibreExampleQuick VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick REQUIRED)
qt_standard_project_setup()

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

qmaplibre_location_copy_plugin(QMapLibreExampleQuick)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_custom_target(deploy
        COMMAND macdeployqt QMapLibreExampleQuick.app -qmldir=${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(deploy QMapLibreExampleQuick)
endif()
