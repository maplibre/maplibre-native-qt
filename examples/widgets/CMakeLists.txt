cmake_minimum_required(VERSION 3.19)

project(QMapLibreExampleWidgets VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)  # suppress some warnings in Qt6
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets REQUIRED)
qt_standard_project_setup()

find_package(QMapLibre COMPONENTS Widgets REQUIRED)

qt_add_executable(QMapLibreExampleWidgets
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
    window.cpp
    window.hpp
)

set_target_properties(QMapLibreExampleWidgets PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

target_link_libraries(QMapLibreExampleWidgets
    PRIVATE
        QMapLibre::Widgets
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_custom_target(deploy
        COMMAND macdeployqt QMapLibreExampleWidgets.app
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(deploy QMapLibreExampleWidgets)
endif()
