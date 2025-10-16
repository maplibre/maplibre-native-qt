function(qmaplibre_quick_setup_plugins target)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _BuildConfiguration)
    get_target_property(_ImportedConfigurations QMapLibre::PluginQml IMPORTED_CONFIGURATIONS)
    if(NOT _BuildConfiguration)
        list(GET _ImportedConfigurations 0 _Configuration)
    elseif(_BuildConfiguration IN_LIST _ImportedConfigurations)
        set(_Configuration ${_BuildConfiguration})
    else()
        list(GET _ImportedConfigurations 0 _Configuration)
    endif()

    get_target_property(_ImportedQml QMapLibre::PluginQml IMPORTED_LOCATION_${_Configuration})
    get_filename_component(_ImportedPathQml ${_ImportedQml} DIRECTORY)
    get_filename_component(_ImportedPathQml ${_ImportedPathQml} DIRECTORY)

    get_property(_targetName TARGET ${target} PROPERTY OUTPUT_NAME)
    if(NOT _targetName)
        set(_targetName ${target})
    endif()
    get_property(_targetDestination TARGET ${target} PROPERTY RUNTIME_OUTPUT_DIRECTORY)
    if(_targetDestination)
        set(_targetDestination "${_targetDestination}/")
    endif()

    set_target_properties(
        ${target}
        PROPERTIES
            QT_QML_IMPORT_PATH "${_ImportedPathQml}"
    )
    # manipulate the cache
    set(_importPathCache ${QML_IMPORT_PATH})
    list(APPEND _importPathCache "${_ImportedPathQml}")
    list(REMOVE_DUPLICATES _importPathCache)
    set(QML_IMPORT_PATH ${_importPathCache} CACHE STRING "QML import path for QtCreator" FORCE)

    get_target_property(_targetTypeCore QMapLibre::Core TYPE)
    if(_targetTypeCore STREQUAL STATIC_LIBRARY)
        target_link_libraries(
            ${target}
            PRIVATE
                QMapLibre::PluginQml
        )
        return()
    endif()
endfunction()
