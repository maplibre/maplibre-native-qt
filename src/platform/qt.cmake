message(STATUS "Configuring MapLibre Native with Qt platform")

option(MLN_QT_LIBRARY_ONLY "Build only MapLibre Native Qt bindings libraries" OFF)
option(MLN_QT_WITH_INTERNAL_SQLITE "Build MapLibre Native Qt bindings with internal sqlite" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Threads REQUIRED)

    option(MLN_QT_WITH_INTERNAL_ICU "Build MapLibre GL Qt bindings with internal ICU" OFF)
    if(NOT MLN_QT_WITH_INTERNAL_ICU)
        # find ICU ignoring Qt paths
        option(MLN_QT_IGNORE_ICU "Ignore Qt-provided ICU library" ON)
        if(MLN_QT_IGNORE_ICU)
            set(_CMAKE_PREFIX_PATH_ORIG ${CMAKE_PREFIX_PATH})
            set(_CMAKE_FIND_ROOT_PATH_ORIG ${CMAKE_FIND_ROOT_PATH})
            unset(CMAKE_PREFIX_PATH)
            unset(CMAKE_FIND_ROOT_PATH)
        endif()

        # Use PkgConfig to find ICU
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(ICU_UC REQUIRED icu-uc)
        pkg_check_modules(ICU_I18N REQUIRED icu-i18n)

        if(MLN_QT_IGNORE_ICU)
            set(CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH_ORIG})
            set(CMAKE_FIND_ROOT_PATH ${_CMAKE_FIND_ROOT_PATH_ORIG})
            unset(_CMAKE_PREFIX_PATH_ORIG)
            unset(_CMAKE_FIND_ROOT_PATH_ORIG)
        endif()
    else()
        message(STATUS "Using internal ICU")
        include(${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/icu.cmake)
    endif()
endif()

if("${QT_VERSION_MAJOR}" STREQUAL "")
    find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
else()
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
endif()
find_package(Qt${QT_VERSION_MAJOR}
             COMPONENTS Gui
                        Network
             REQUIRED)

if(NOT MLN_QT_WITH_INTERNAL_SQLITE)
    find_package(Qt${QT_VERSION_MAJOR}Sql REQUIRED)
else()
    message(STATUS "Using internal sqlite")
    include(${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/sqlite.cmake)
endif()

# Debugging & ccache on Windows
if (MSVC)
    foreach(config DEBUG RELWITHDEBINFO)
        foreach(lang C CXX)
            set(flags_var "CMAKE_${lang}_FLAGS_${config}")
            string(REPLACE "/Zi" "/Z7" ${flags_var} "${${flags_var}}")
            set(${flags_var} "${${flags_var}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endif()

target_sources(
    mbgl-core
    PRIVATE
        $<IF:$<PLATFORM_ID:Linux>,${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/text/bidi.cpp,${CMAKE_CURRENT_LIST_DIR}/src/mbgl/bidi.cpp>
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/include/mbgl/gfx/headless_backend.hpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/include/mbgl/gfx/headless_frontend.hpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/gfx/headless_backend.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/gfx/headless_frontend.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/i18n/collator.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/layermanager/layer_manager.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/platform/time.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/asset_file_source.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/database_file_source.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/file_source_manager.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/file_source_request.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/local_file_request.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/local_file_source.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/main_resource_loader.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/mbtiles_file_source.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/offline.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/offline_database.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/offline_download.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/online_file_source.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/$<IF:$<BOOL:${MLN_WITH_PMTILES}>,pmtiles_file_source.cpp,pmtiles_file_source_stub.cpp>
        $<IF:$<BOOL:${MLN_QT_WITH_INTERNAL_SQLITE}>,${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/storage/sqlite3.cpp,${CMAKE_CURRENT_LIST_DIR}/src/mbgl/sqlite3.cpp>
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/util/compression.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/util/filesystem.cpp
        ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/util/monotonic_timer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/async_task.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/async_task_impl.hpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/gl_functions.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/http_file_source.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/http_file_source.hpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/http_request.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/http_request.hpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/image.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/number_format.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/local_glyph_rasterizer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/logging_qt.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/run_loop.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/run_loop_impl.hpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/string_stdlib.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/thread.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/thread_local.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/timer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/timer_impl.hpp
        ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/utf.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/utils/renderer_observer.hpp
        ${CMAKE_CURRENT_LIST_DIR}/src/utils/scheduler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/utils/scheduler.hpp
)


# Add Qt Metal renderer backend only when Metal backend is enabled
if(MLN_WITH_METAL)
    target_sources(
        mbgl-core
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/src/utils/metal_renderer_backend.mm
            ${CMAKE_CURRENT_LIST_DIR}/src/utils/metal_renderer_backend.hpp
            ${CMAKE_CURRENT_LIST_DIR}/src/utils/renderer_backend.hpp

            # Metal head-less backend (core implementation)
            ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/mtl/headless_backend.cpp
    )
    target_compile_definitions(
        mbgl-core
        PRIVATE MLN_RENDER_BACKEND_METAL=1
    )
endif()

# Add Qt OpenGL renderer backend only when OpenGL backend is enabled
if(MLN_WITH_OPENGL)
    target_sources(
        mbgl-core
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/src/mbgl/headless_backend_qt.cpp
            ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/gl/headless_backend.cpp

            ${CMAKE_CURRENT_LIST_DIR}/src/utils/opengl_renderer_backend.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/utils/opengl_renderer_backend.hpp
            ${CMAKE_CURRENT_LIST_DIR}/src/utils/renderer_backend.hpp
    )

    # Define backend selector macro when OpenGL is enabled (and Metal/Vulkan not preferred)
    target_compile_definitions(
        mbgl-core
        PRIVATE MLN_RENDER_BACKEND_OPENGL=1
    )
endif()

# ---------------------------------------------------------------------------
# Qt Vulkan renderer backend: only when MLN_WITH_VULKAN and qvulkaninstance.h present
# ---------------------------------------------------------------------------
if(MLN_WITH_VULKAN)
    find_path(QT_VULKAN_HEADER qvulkaninstance.h
        PATHS ${Qt${QT_VERSION_MAJOR}Gui_INCLUDE_DIRS}
        NO_DEFAULT_PATH)

    if(QT_VULKAN_HEADER)
        target_sources(
            mbgl-core
            PRIVATE
                ${CMAKE_CURRENT_LIST_DIR}/src/utils/vulkan_renderer_backend.cpp
                ${CMAKE_CURRENT_LIST_DIR}/src/utils/vulkan_renderer_backend.hpp
                ${CMAKE_CURRENT_LIST_DIR}/src/utils/renderer_backend.hpp

                # Vulkan head-less backend (core implementation)
                ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/src/mbgl/vulkan/headless_backend.cpp
        )


    else()
        message(STATUS "Qt build has no Vulkan headers; skipping Qt Vulkan backend")
        set(MLN_WITH_VULKAN OFF CACHE BOOL "Disable Vulkan backend due to missing Qt Vulkan support" FORCE)
    endif()

    target_compile_definitions(
        mbgl-core
        PRIVATE MLN_RENDER_BACKEND_VULKAN=1
    )

endif()

target_compile_definitions(
    mbgl-core
    PRIVATE QT_IMAGE_DECODERS
    PUBLIC __QT__
)

# On macOS, libc++ defines __cpp_lib_format while still marking 'to_chars' for
# floating-point as unavailable for deployment targets < 13.3, causing the
# Vulkan headers to trip over it.  Force-undefine the macro before compilation
# so that the header falls back to its safe <sstream> path.
target_compile_options(
    mbgl-core
    PRIVATE
        $<$<PLATFORM_ID:Darwin>:-U__cpp_lib_format -D__cpp_lib_format=0>
)

target_include_directories(
    mbgl-core
    PRIVATE ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/platform/default/include
            ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/metal-cpp
            ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/nunicode/include
)

include(${PROJECT_SOURCE_DIR}/vendor/maplibre-native/vendor/nunicode.cmake)

set_property(TARGET mbgl-core PROPERTY AUTOMOC ON)
if (Qt6_FOUND AND COMMAND qt_enable_autogen_tool)
    qt_enable_autogen_tool(mbgl-core "moc" ON)
endif()

target_link_libraries(
    mbgl-core
    PUBLIC
        $<$<NOT:$<OR:$<PLATFORM_ID:Windows>,$<PLATFORM_ID:Emscripten>>>:z>
        $<IF:$<BOOL:${MLN_QT_WITH_INTERNAL_SQLITE}>,$<BUILD_INTERFACE:mbgl-vendor-sqlite>,Qt${QT_VERSION_MAJOR}::Sql>
    PRIVATE
        $<$<PLATFORM_ID:Linux>:${CMAKE_THREAD_LIBS_INIT}>
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (MLN_QT_WITH_INTERNAL_ICU)
        target_link_libraries(mbgl-core PUBLIC $<BUILD_INTERFACE:mbgl-vendor-icu>)
    else()
        target_link_libraries(mbgl-core PUBLIC ${ICU_UC_LIBRARIES} ${ICU_I18N_LIBRARIES})
        target_include_directories(mbgl-core PRIVATE ${ICU_UC_INCLUDE_DIRS} ${ICU_I18N_INCLUDE_DIRS})
    endif()
endif()

# Object library list
get_directory_property(MLN_QT_HAS_PARENT PARENT_DIRECTORY)
if(MLN_QT_HAS_PARENT)
    set(MLN_QT_VENDOR_LIBRARIES
        $<$<BOOL:${MLN_QT_WITH_INTERNAL_SQLITE}>:$<BUILD_INTERFACE:mbgl-vendor-sqlite>>
        $<$<AND:$<PLATFORM_ID:Linux>,$<BOOL:${MLN_QT_WITH_INTERNAL_ICU}>>:$<BUILD_INTERFACE:mbgl-vendor-icu>>
        PARENT_SCOPE
    )
endif()

if(NOT MLN_QT_LIBRARY_ONLY)
    # test runner
    add_executable(
        mbgl-test-runner
        ${CMAKE_CURRENT_LIST_DIR}/test/main.cpp
    )

    target_include_directories(
        mbgl-test-runner
        PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/test/include
    )

    target_compile_definitions(
        mbgl-test-runner
        PRIVATE
            WORK_DIRECTORY=${PROJECT_SOURCE_DIR}
            $<$<PLATFORM_ID:Windows>:MBGL_BUILDING_LIB>
    )

    target_link_libraries(
        mbgl-test-runner
        PRIVATE
            Qt${QT_VERSION_MAJOR}::Gui
            mbgl-compiler-options
    )

    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        target_link_libraries(
            mbgl-test-runner
            PRIVATE -Wl,-force_load mbgl-test
        )
    else()
        target_link_libraries(
            mbgl-test-runner
            PRIVATE $<LINK_LIBRARY:WHOLE_ARCHIVE,mbgl-test>
        )
    endif()

    add_test(NAME mbgl-test-runner COMMAND mbgl-test-runner WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()

# Metal backend sources (macOS only)
if(APPLE AND MLN_WITH_METAL)
    target_link_libraries(
        mbgl-core
        PRIVATE
            "-framework QuartzCore"
            "-framework Metal"
            "-framework CoreFoundation"
    )
    target_compile_definitions(
        mbgl-core
        PRIVATE MLN_RENDER_BACKEND_METAL=1
    )
endif()
