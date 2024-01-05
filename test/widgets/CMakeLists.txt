set(test_sources
    ../common/test_rendering.cpp ../common/test_rendering.hpp
    gl_tester.cpp gl_tester.hpp
    test_widgets.cpp
)
if(COMMAND qt_add_executable)
    qt_add_executable(test_mln_widgets ${test_sources})
else()
    add_executable(test_mln_widgets ${test_sources})
endif()

target_include_directories(
    test_mln_widgets
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
        ${CMAKE_SOURCE_DIR}/src/widgets
        ${CMAKE_BINARY_DIR}/src/widgets/include
        ${CMAKE_SOURCE_DIR}/test/common
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)
target_link_libraries(
    test_mln_widgets
    PRIVATE
        Widgets
        Qt${QT_VERSION_MAJOR}::Test
        $<BUILD_INTERFACE:mbgl-compiler-options>
)
set_target_properties(test_mln_widgets PROPERTIES AUTOMOC ON)

if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(test_mln_widgets PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

add_test(
    NAME test_mln_widgets
    COMMAND $<TARGET_FILE:test_mln_widgets>
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_tests_properties(
        test_mln_widgets
        PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Core>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Widgets>")
endif()
