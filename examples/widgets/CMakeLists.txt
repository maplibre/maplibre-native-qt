cmake_minimum_required(VERSION 3.19...3.21)

project(QMapLibreExampleWidgets VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE) # suppress some warnings in Qt6
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets REQUIRED)
qt_standard_project_setup()

find_package(QMapLibre COMPONENTS Widgets REQUIRED)

qt_add_executable(
    QMapLibreExampleWidgets
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
    window.cpp
    window.hpp
)

set_target_properties(
    QMapLibreExampleWidgets
    PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
)

target_link_libraries(
    QMapLibreExampleWidgets
    PRIVATE
        QMapLibre::Widgets
)

install(TARGETS QMapLibreExampleWidgets
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET QMapLibreExampleWidgets
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(
        IMPORTED_RUNTIME_ARTIFACTS QMapLibre::Core QMapLibre::Widgets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Packaging
set(CPACK_GENERATOR TBZ2)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPACK_SYSTEM_NAME "macOS")
else()
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
endif()

include(CPack)
