# @file        : DiscoverCheckTests.cmake
# @author      : ElGatoPanzon
# @created     : 2026-02-26
# @description : CMake module to auto-discover libcheck START_TEST entries from source files

# discover_check_tests(TARGET <target> SOURCE <src>)
#
# Scans the source file for START_TEST(test_name) macros and calls add_test()
# for each discovered test. Each test runs the target binary with CK_RUN_TEST
# set so libcheck runs only that specific test case.
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

    foreach(line ${src_lines})
        # match START_TEST(identifier) -- identifier is C name (letters/digits/underscore)
        # note: CMake regex does not support POSIX [[:space:]] -- use literal match
        if(line MATCHES "^START_TEST\\(([A-Za-z_][A-Za-z0-9_]*)\\)")
            set(test_name "${CMAKE_MATCH_1}")
            set(ctest_name "${suite_name}.${test_name}")
            add_test(
                NAME "${ctest_name}"
                COMMAND "$<TARGET_FILE:${DCT_TARGET}>"
            )
            set_tests_properties("${ctest_name}" PROPERTIES
                ENVIRONMENT "CK_RUN_TEST=${test_name}"
            )
        endif()
    endforeach()
endfunction()
