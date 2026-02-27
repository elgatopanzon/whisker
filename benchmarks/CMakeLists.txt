# @file        : CMakeLists.txt
# @author      : ElGatoPanzon
# @created     : 2026-02-26
# @description : CMake config for whisker benchmarks -- one binary per benchmark file

# fetch ubench header-only benchmark library
FetchContent_Declare(
    ubench
    GIT_REPOSITORY https://github.com/sheredom/ubench.h.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(ubench)

# collect all benchmark source files
file(GLOB BENCHMARK_SOURCES "benchmark_*.c")

foreach(src ${BENCHMARK_SOURCES})
    # strip benchmark_ prefix for binary name: benchmark_whisker_memory.c -> whisker_memory_benchmarks
    get_filename_component(base "${src}" NAME_WE)
    string(REGEX REPLACE "^benchmark_" "" bin_name "${base}")
    set(target_name "${bin_name}_benchmarks")

    add_executable(${target_name} "${src}")

    target_link_libraries(${target_name}
        PRIVATE
            ${PROJECT_NAME}
            m
    )

    target_include_directories(${target_name}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
            ${ubench_SOURCE_DIR}
    )

    # gnu11 required: ubench uses GNU asm extensions for its do-nothing optimizer barrier
    target_compile_options(${target_name} PRIVATE --std=gnu11)
endforeach()
