function(qmaplibre_location_setup_plugins target)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _BuildConfiguration)
    get_target_property(_ImportedConfigurations QMapLibre::PluginGeoServices IMPORTED_CONFIGURATIONS)
    if(NOT _BuildConfiguration)
        list(GET _ImportedConfigurations 0 _Configuration)
    elseif(_BuildConfiguration IN_LIST _ImportedConfigurations)
        set(_Configuration ${_BuildConfiguration})
    else()
        list(GET _ImportedConfigurations 0 _Configuration)
    endif()

    get_target_property(_ImportedLocationGeoServices QMapLibre::PluginGeoServices IMPORTED_LOCATION_${_Configuration})
    get_filename_component(_ImportedLocationPathGeoServices ${_ImportedLocationGeoServices} DIRECTORY)

    get_target_property(_ImportedLocationQml QMapLibre::PluginQml IMPORTED_LOCATION_${_Configuration})
    get_filename_component(_ImportedLocationPathQml ${_ImportedLocationQml} DIRECTORY)
    get_filename_component(_ImportedLocationPathQml ${_ImportedLocationPathQml} DIRECTORY)

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
            QT_QML_IMPORT_PATH "${_ImportedLocationPathQml}"
    )
    # manipulate the cache
    set(_importPathCache ${QML_IMPORT_PATH})
    list(APPEND _importPathCache "${_ImportedLocationPathQml}")
    list(REMOVE_DUPLICATES _importPathCache)
    set(QML_IMPORT_PATH ${_importPathCache} CACHE STRING "QML import path for QtCreator" FORCE)

    get_target_property(_targetTypeCore QMapLibre::Core TYPE)
    if(_targetTypeCore STREQUAL STATIC_LIBRARY)
        target_link_libraries(
            ${target}
            PRIVATE
                QMapLibre::PluginGeoServices
                QMapLibre::PluginQml
        )
        return()
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        file(
            COPY "${_ImportedLocationGeoServices}"
            DESTINATION "${_targetDestination}${_targetName}.app/Contents/PlugIns/geoservices"
        )
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        install(
            FILES "${_ImportedLocationGeoServices}"
            DESTINATION "plugins/geoservices"
        )
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
        set_target_properties(
            ${target}
            PROPERTIES
                QT_ANDROID_EXTRA_PLUGINS "${_ImportedLocationPathGeoServices}"
        )
    endif()
endfunction()
