qt_add_executable(test_mln_location test_location.cpp)

qt_add_resources(test_mln_location test_fixtures
    PREFIX "/"
    BASE "../../vendor/maplibre-native/metrics/integration/sprites"
    FILES "../../vendor/maplibre-native/metrics/integration/sprites/1x.png"
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS QuickTest REQUIRED)
target_link_libraries(
    test_mln_location
    PRIVATE
        Qt${QT_VERSION_MAJOR}::QuickTest
        $<BUILD_INTERFACE:mbgl-compiler-options>
)

get_target_property(MLNQtLocationTargetType MLNQtLocation TYPE)
if (MLNQtLocationTargetType STREQUAL STATIC_LIBRARY)
    target_link_libraries(
        test_mln_location
        PRIVATE
            ${MLN_QT_GEOSERVICES_PLUGIN}
            ${MLN_QT_QML_PLUGIN_LOCATION}
    )
endif()

if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(test_mln_location PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

add_test(
    NAME test_mln_location
    COMMAND $<TARGET_FILE:test_mln_location> -input ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(
    test_mln_location
    PROPERTIES
        ENVIRONMENT "$<$<PLATFORM_ID:macOS>:DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/src/core:${CMAKE_BINARY_DIR}/src/quick:${CMAKE_BINARY_DIR}/src/location;>QSG_RHI_BACKEND=${MLN_QT_TEST_RENDERER};QML_IMPORT_PATH=${CMAKE_BINARY_DIR}/src/location/plugins;QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/location/plugins"
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_tests_properties(
        test_mln_location
        PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:MLNQtCore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:MLNQtQuickPrivate>;PATH=path_list_prepend:$<TARGET_FILE_DIR:MLNQtLocation>")
endif()
