# @file        : CMakeLists.txt
# @author      : ElGatoPanzon
# @created     : 2026-02-26
# @description : CMake config for whisker test suite -- one binary per test file

include(FetchContent)

# libcheck test framework
FetchContent_Declare(
    check
    GIT_REPOSITORY https://github.com/libcheck/check.git
    GIT_TAG        0.15.2
)
FetchContent_MakeAvailable(check)

include(${CMAKE_CURRENT_SOURCE_DIR}/DiscoverCheckTests.cmake)

# collect all test source files
file(GLOB TEST_SOURCES "test_*.c")

foreach(src ${TEST_SOURCES})
    # strip test_ prefix for binary name: test_whisker_array.c -> whisker_array_tests
    get_filename_component(base "${src}" NAME_WE)
    string(REGEX REPLACE "^test_" "" bin_name "${base}")
    set(target_name "${bin_name}_tests")

    add_executable(${target_name} "${src}")

    target_link_libraries(${target_name}
        PRIVATE
            ${PROJECT_NAME}
            check
    )

    target_include_directories(${target_name}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )

    target_compile_options(${target_name} PRIVATE --std=c11 -UNDEBUG)

    # register granular per-test ctest entries (one entry per START_TEST)
    discover_check_tests(TARGET ${target_name} SOURCE "${src}")
endforeach()
