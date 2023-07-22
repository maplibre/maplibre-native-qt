if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    return()
endif()

add_subdirectory(core)
add_subdirectory(qml)
