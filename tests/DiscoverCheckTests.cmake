# @file        : DiscoverCheckTests.cmake
# @author      : ElGatoPanzon
# @created     : 2026-02-26
# @description : CMake module to auto-discover libcheck test cases from source files

# discover_check_tests(TARGET <target> SOURCE <src>)
#
# Scans the source file for tcase_create("name") and tcase_add_test(var, func)
# calls. Registers one ctest entry per tcase, with CK_RUN_CASE set so libcheck
# runs only that specific test case.
#
# Requires: target already defined via add_executable() before calling this.
function(discover_check_tests)
    cmake_parse_arguments(DCT "" "TARGET;SOURCE" "" ${ARGN})

    if(NOT DCT_TARGET)
        message(FATAL_ERROR "discover_check_tests: TARGET is required")
    endif()

    if(NOT DCT_SOURCE)
        message(FATAL_ERROR "discover_check_tests: SOURCE is required")
    endif()

    set(src "${DCT_SOURCE}")

    # resolve relative paths against the current source dir
    if(NOT IS_ABSOLUTE "${src}")
        set(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
    endif()

    if(NOT EXISTS "${src}")
        message(WARNING "discover_check_tests: source file not found: ${src}")
        return()
    endif()

    # derive suite name from binary target name: strip "test_" prefix if present
    set(suite_name "${DCT_TARGET}")
    if(suite_name MATCHES "^test_(.*)")
        set(suite_name "${CMAKE_MATCH_1}")
    endif()

    # read all lines from the file
    file(STRINGS "${src}" src_lines)

    # pass 1: collect tcase names from tcase_create("name") calls
    # and track which variable each tcase is assigned to
    set(tcase_names "")
    set(tcase_vars "")
    foreach(line ${src_lines})
        # match: TCase *var = tcase_create("name")  or similar patterns
        if(line MATCHES "tcase_create\\(\"([A-Za-z_][A-Za-z0-9_]*)\"\\)")
            set(case_name "${CMAKE_MATCH_1}")
            # extract the variable name (e.g. tc_init from "TCase *tc_init = ...")
            if(line MATCHES "([A-Za-z_][A-Za-z0-9_]*)[ \t]*=[ \t]*tcase_create")
                set(case_var "${CMAKE_MATCH_1}")
                list(APPEND tcase_names "${case_name}")
                list(APPEND tcase_vars "${case_var}")
            endif()
        endif()
    endforeach()

    # pass 2: register one ctest entry per tcase with CK_RUN_CASE
    foreach(case_name ${tcase_names})
        set(ctest_name "${suite_name}.${case_name}")
        add_test(
            NAME "${ctest_name}"
            COMMAND "$<TARGET_FILE:${DCT_TARGET}>"
        )
        set_tests_properties("${ctest_name}" PROPERTIES
            ENVIRONMENT "CK_RUN_CASE=${case_name}"
        )
    endforeach()
endfunction()
