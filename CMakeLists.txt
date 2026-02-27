######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : 30/01/2025
######################################################################

# set cmake min version for script features
cmake_minimum_required(VERSION 3.20)

# prevent in-source building to enforce clean builds
if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.
    Please create a subfolder and use `cmake ..` inside it.
    NOTE: cmake creates CMakeCache.txt and CMakeFiles/*.
          Remove them, or cmake will refuse to work.")
endif()

####################
#  Project config  #
####################
project(whisker C)

# set and force c standard
set(C_STANDARD 11)
set(C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# pass warning cli switches to compiler
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra)
endif()

# add binary path to prefix
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# match build of the Conan libs to RELEASE
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG RELEASE)
set(CMAKE_MAP_IMPORTED_CONFIG_ASAN RELEASE)
set(CMAKE_MAP_IMPORTED_CONFIG_LSAN RELEASE)
set(CMAKE_MAP_IMPORTED_CONFIG_TSAN RELEASE)
set(CMAKE_MAP_IMPORTED_CONFIG_MSAN RELEASE)
set(CMAKE_MAP_IMPORTED_CONFIG_UBSAN RELEASE)

# set default build type to Asan to enable address sanitizer
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Asan")
endif()

# inform what the build type is
message(STATUS "CMake build type: " ${CMAKE_BUILD_TYPE})

# add correct compiler switches for different sanitizer builds
if(MSVC)
    if(NOT "Asan" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES Asan)
    endif()

    set(CMAKE_EXE_LINKER_FLAGS_ASAN "/DEBUG /INCREMENTAL:NO")
    add_compile_options("$<$<CONFIG:Asan>:/DEBUG /fsanitize=address>")
else()
    # Support for sanitizer build types
    set(CMAKE_C_FLAGS_ASAN
        "-fsanitize=address -static-libasan -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1"
        CACHE STRING "Build with AddressSanitizer"
        FORCE)

    set(CMAKE_C_FLAGS_LSAN
        "-fsanitize=leak -fno-omit-frame-pointer -g -O1"
        CACHE STRING "Build with LeakSanitizer"
        FORCE)

    set(CMAKE_CXX_FLAGS_TSAN
        "-fsanitize=thread -g -O1"
        CACHE STRING "Build with ThreadSanitizer"
        FORCE)

    set(CMAKE_C_FLAGS_MSAN
        "-fsanitize=memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O2"
        CACHE STRING "Build with MemorySanitizer"
        FORCE)

    set(CMAKE_C_FLAGS_UBSAN
        "-fsanitize=undefined"
        CACHE STRING "Build with UndefinedBehaviourSanitizer"
        FORCE)
endif()

###########################
#  Dependencies & Source  #
###########################
include(FetchContent)

# include the src directory to import cmake config
add_subdirectory(src)

# enable ctest
enable_testing()

# include the tests directory
add_subdirectory(tests)

# include the benchmarks directory
add_subdirectory(benchmarks)
