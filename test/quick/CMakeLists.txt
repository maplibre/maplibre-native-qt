if(COMMAND qt_add_executable)
    qt_add_executable(test_mln_quick test_quick.cpp)

    qt_add_resources(test_mln_quick test_fixtures
        PREFIX "/"
        BASE "../../vendor/maplibre-native/metrics/integration/sprites"
        FILES "../../vendor/maplibre-native/metrics/integration/sprites/1x.png"
    )
else()
    add_executable(test_mln_quick test_quick.cpp)
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS QuickTest REQUIRED)
target_link_libraries(
    test_mln_quick
    PRIVATE
        Qt${QT_VERSION_MAJOR}::QuickTest
        $<BUILD_INTERFACE:mbgl-compiler-options>
)

get_target_property(MLNQtCoreTargetType MLNQtCore TYPE)
if (MLNQtCoreTargetType STREQUAL STATIC_LIBRARY)
    target_link_libraries(
        test_mln_quick
        PRIVATE
            ${MLN_QT_QUICK_MODULE}
    )
endif()

if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(test_mln_quick PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

add_test(
    NAME test_mln_quick
    COMMAND $<TARGET_FILE:test_mln_quick> -input ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(
    test_mln_quick
    PROPERTIES
        ENVIRONMENT "$<$<PLATFORM_ID:macOS>:DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/src/core>;QSG_RHI_BACKEND=metal;QML_IMPORT_PATH=${CMAKE_BINARY_DIR}/src/quick;"
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_tests_properties(
        test_mln_quick
        PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:MLNQtCore>")
endif()
