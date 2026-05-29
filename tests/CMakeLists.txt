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

# module tests: tests/modules/<subdir>/test_*.c -> whisker_module_<subdir>_tests
file(GLOB module_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/modules ${CMAKE_CURRENT_SOURCE_DIR}/modules/*)
file(GLOB src_module_dirs RELATIVE ${PROJECT_SOURCE_DIR}/src/modules ${PROJECT_SOURCE_DIR}/src/modules/*)

set(all_module_test_libs "")
foreach(src_module_dir ${src_module_dirs})
    if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/src/modules/${src_module_dir}" AND EXISTS "${PROJECT_SOURCE_DIR}/src/modules/${src_module_dir}/CMakeLists.txt")
        list(APPEND all_module_test_libs "whisker_module_${src_module_dir}")
    endif()
endforeach()

foreach(dir ${module_dirs})
    set(mod_dir "${CMAKE_CURRENT_SOURCE_DIR}/modules/${dir}")
    if(IS_DIRECTORY "${mod_dir}")
        file(GLOB MOD_TEST_SOURCES "${mod_dir}/test_*.c")
    else()
        # single file: modules/test_<name>.c -> whisker_module_<name>_tests
        if(dir MATCHES "^test_.*\\.c$")
            set(MOD_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/modules/${dir}")
        else()
            set(MOD_TEST_SOURCES "")
        endif()
    endif()

    foreach(src ${MOD_TEST_SOURCES})
        get_filename_component(base "${src}" NAME_WE)
        string(REGEX REPLACE "^test_" "" bin_name "${base}")
        set(target_name "whisker_module_${bin_name}_tests")

        add_executable(${target_name} "${src}")

        target_link_libraries(${target_name}
            PRIVATE
                ${PROJECT_NAME}
                ${all_module_test_libs}
                check
        )

        target_include_directories(${target_name}
            PRIVATE
                ${PROJECT_SOURCE_DIR}/src
        )

        target_compile_options(${target_name} PRIVATE --std=c11 -UNDEBUG)

        discover_check_tests(TARGET ${target_name} SOURCE "${src}")
    endforeach()
endforeach()
