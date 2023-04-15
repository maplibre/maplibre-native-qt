if (Qt5_FOUND)
    return()
endif()

set(test_sources
    ../common/test_rendering.cpp ../common/test_rendering.hpp
    map_tester.cpp map_tester.hpp
    test_core.cpp
)
if (COMMAND qt_add_executable)
    qt_add_executable(test_mln_core ${test_sources})
else()
    add_executable(test_mln_core ${test_sources})
endif()

target_include_directories(test_mln_core
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_BINARY_DIR}/src/core/include
        ${CMAKE_SOURCE_DIR}/test/common
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)
target_link_libraries(test_mln_core
    PRIVATE
    $<$<BOOL:${Qt6_FOUND}>:Qt::OpenGLWidgets>
    Qt::Test
    ${MLN_QT_CORE_LIB}
)
set_target_properties(test_mln_core PROPERTIES AUTOMOC ON)

add_test(NAME test_mln_core COMMAND $<TARGET_FILE:test_mln_core>)
set_tests_properties(test_mln_core PROPERTIES
    ENVIRONMENT "MLN_FIXTURES_PATH=${CMAKE_SOURCE_DIR}/vendor/maplibre-native/test/fixtures")
