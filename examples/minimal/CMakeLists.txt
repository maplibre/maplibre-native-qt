cmake_minimum_required(VERSION 3.16)

project(minimal VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Gui)
qt_standard_project_setup()

qt_add_executable(minimal
    main.cpp
)

qt_add_qml_module(minimal
    URI minimal
    VERSION 1.0
    QML_FILES
        main.qml
)

set_target_properties(minimal PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

target_link_libraries(minimal PRIVATE Qt6::Gui Qt6::Quick)
