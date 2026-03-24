######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Saturday Mar 07, 2026 12:49:55 CST
# @description : Build config for utilities module
######################################################################

file(GLOB_RECURSE UTILITIES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_utilities OBJECT
    ${UTILITIES_SOURCES}
)

target_include_directories(whisker_module_utilities
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_utilities PRIVATE --std=c11)
