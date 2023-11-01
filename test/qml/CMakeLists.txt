if (COMMAND qt_add_executable)
    qt_add_executable(test_mln_qml test_qml.cpp)
else()
    add_executable(test_mln_qml test_qml.cpp)
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS QuickTest REQUIRED)
target_link_libraries(test_mln_qml
    PRIVATE
        Qt${QT_VERSION_MAJOR}::QuickTest
        $<BUILD_INTERFACE:mbgl-compiler-options>
)

add_test(NAME test_mln_qml COMMAND $<TARGET_FILE:test_mln_qml> -input ${CMAKE_CURRENT_SOURCE_DIR}/qt${QT_VERSION_MAJOR})
set_tests_properties(
    test_mln_qml
    PROPERTIES
        ENVIRONMENT "DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/src/core:${CMAKE_BINARY_DIR}/src/location;QSG_RHI_BACKEND=opengl;QML_IMPORT_PATH=${CMAKE_BINARY_DIR}/src/location/plugins;QML2_IMPORT_PATH=${CMAKE_BINARY_DIR}/src/location/plugins;QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/location/plugins"
)
