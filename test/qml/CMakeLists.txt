if(COMMAND qt_add_executable)
    qt_add_executable(test_mln_qml test_qml.cpp)

    qt_add_resources(test_mln_qml test_fixtures
        PREFIX "/"
        BASE "../../vendor/maplibre-native/metrics/integration/sprites"
        FILES "../../vendor/maplibre-native/metrics/integration/sprites/1x.png"
    )
else()
    add_executable(test_mln_qml test_qml.cpp)
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS QuickTest REQUIRED)
target_link_libraries(
    test_mln_qml
    PRIVATE
        Qt${QT_VERSION_MAJOR}::QuickTest
        $<BUILD_INTERFACE:mbgl-compiler-options>
)

get_target_property(MLNQtLocationTargetType MLNQtLocation TYPE)
if (MLNQtLocationTargetType STREQUAL STATIC_LIBRARY)
    target_link_libraries(
        test_mln_qml
        PRIVATE
            ${MLN_QT_GEOSERVICES_PLUGIN}
            ${MLN_QT_QML_PLUGIN}
    )
endif()

if(MLN_QT_WITH_CLANG_TIDY)
    set_target_properties(test_mln_qml PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

add_test(
    NAME test_mln_qml
    COMMAND $<TARGET_FILE:test_mln_qml> -input ${CMAKE_CURRENT_SOURCE_DIR}/qt${QT_VERSION_MAJOR}
)
set_tests_properties(
    test_mln_qml
    PROPERTIES
        ENVIRONMENT "$<$<PLATFORM_ID:macOS>:DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/src/core:${CMAKE_BINARY_DIR}/src/location;>QSG_RHI_BACKEND=opengl;QML_IMPORT_PATH=${CMAKE_BINARY_DIR}/src/location/plugins;QML2_IMPORT_PATH=${CMAKE_BINARY_DIR}/src/location/plugins;QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/location/plugins"
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_tests_properties(
        test_mln_qml
        PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:MLNQtCore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:MLNQtLocation>")
endif()
