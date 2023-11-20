find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

# Param _targetname     The name of new the custom make target
# Param _testrunner     The name of the target which runs the tests.
#                        MUST return ZERO always, even on errors.
#                        If not, no coverage report will be created!
# Param _outputname     lcov output is generated as _outputname.info
#                       HTML report is generated in _outputname/index.html
# Optional fourth parameter is passed as arguments to _testrunner
#   Pass them in list form, e.g.: "-j;2" for -j 2
function(setup_target_for_coverage _targetname _testrunner _outputname)
    if(NOT GCOV_PATH)
        message(FATAL_ERROR "gcov not found! Aborting...")
    endif()

    if(NOT LCOV_PATH)
        message(FATAL_ERROR "lcov not found! Aborting...")
    endif()

    if(NOT GENHTML_PATH)
        message(FATAL_ERROR "genhtml not found! Aborting...")
    endif()

    # Setup target
    add_custom_target(
        ${_targetname}
        # Cleanup and prepare lcov
        ${LCOV_PATH} --directory src --zerocounters
        COMMAND
            ${LCOV_PATH} --directory src --capture --initial
            --ignore-errors inconsistent,inconsistent
            --output-file ${_outputname}.info.initial
            --gcov-tool ${GCOV_PATH}
        # Run tests
        COMMAND ${_testrunner} ${ARGV3}
        # Capturing lcov counters and generating report
        COMMAND
            ${LCOV_PATH} --directory src --capture
            --ignore-errors inconsistent,inconsistent
            --output-file ${_outputname}.info.full
            --gcov-tool ${GCOV_PATH}
        COMMAND
            ${LCOV_PATH} --remove ${_outputname}.info.full
            '/usr/*' '*/Qt/*'
            '*/examples/*' '*/vendor/*'
            '${CMAKE_BINARY_DIR}*'
            --ignore-errors inconsistent,inconsistent,unused
            --output-file ${_outputname}.info
            --gcov-tool ${GCOV_PATH}
        COMMAND
            ${GENHTML_PATH} -t "${PROJECT_NAME}" -o ${_outputname}
            ${_outputname}.info -p "${CMAKE_SOURCE_DIR}"
            --ignore-errors inconsistent,inconsistent
        COMMAND
            ${LCOV_PATH} --summary ${_outputname}.info --ignore-errors inconsistent,inconsistent > ${_outputname}.summary
        COMMAND
            ${CMAKE_COMMAND} -E remove ${_outputname}.info.initial ${_outputname}.info.full
        WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}
        COMMENT
            "Resetting code coverage counters to zero. \nProcessing code coverage counters and generating report."
    )
endfunction()
