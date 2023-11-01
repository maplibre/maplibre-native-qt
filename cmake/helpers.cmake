function(mln_header_rename HEADER HEADER_OUT HEADER_INCLUDE)
    string(REGEX REPLACE ".*/" "" HEADER ${HEADER})
    string(REPLACE "gl_" "GL_" HEADER_TMP ${HEADER})
    string(REPLACE "_" ";" HEADER_SPLIT ${HEADER_TMP})

    foreach(part ${HEADER_SPLIT})
        string(REGEX REPLACE "\.hpp" "" part ${part})
        string(SUBSTRING ${part} 0 1 part_first_letter)
        string(TOUPPER ${part_first_letter} part_first_letter)
        string(REGEX REPLACE "^.(.*)" "${part_first_letter}\\1" part_out ${part})
        list(APPEND HEADER_OUT_LIST ${part_out})
    endforeach()

    string(CONCAT HEADER_OUT_TMP ${HEADER_OUT_LIST})
    string(REGEX REPLACE "Export.*" "Export" HEADER_OUT_TMP "${HEADER_OUT_TMP}")

    set(${HEADER_OUT} ${HEADER_OUT_TMP} PARENT_SCOPE)
    set(${HEADER_INCLUDE} ${HEADER} PARENT_SCOPE)
endfunction()

function(mln_header_preprocess HEADER LIBRARY_NAME LIBRARY_HEADER)
    mln_header_rename(${HEADER} HEADER_OUT HEADER_INCLUDE)

    set(OUT_NAME "${CMAKE_CURRENT_BINARY_DIR}/include/${LIBRARY_NAME}/${HEADER_OUT}")
    if(NOT EXISTS ${OUT_NAME})
        file(WRITE "${OUT_NAME}" "#include \"${HEADER_INCLUDE}\"")
    endif()
    set(${LIBRARY_HEADER} ${OUT_NAME} PARENT_SCOPE)
endfunction()
