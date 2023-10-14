function(qmaplibre_location_copy_plugin target)
    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        return()
    endif()

    string(TOUPPER "${CMAKE_BUILD_TYPE}" _BuildConfiguration)
    get_target_property(_ImportedConfigurations QMapLibre::PluginGeoServices IMPORTED_CONFIGURATIONS)
    if(NOT _BuildConfiguration)
        list(GET _ImportedConfigurations 0 _Configuration)
    elseif(_BuildConfiguration IN_LIST _ImportedConfigurations)
        set(_Configuration ${_BuildConfiguration})
    else()
        list(GET _ImportedConfigurations 0 _Configuration)
    endif()

    get_target_property(_ImportedLocation QMapLibre::PluginGeoServices IMPORTED_LOCATION_${_Configuration})

    get_property(_targetName TARGET ${target} PROPERTY OUTPUT_NAME)
    if (NOT _targetName)
        set(_targetName ${target})
    endif()

    file(COPY "${_ImportedLocation}" DESTINATION "${_targetName}.app/Contents/PlugIns/geoservices")
endfunction()
