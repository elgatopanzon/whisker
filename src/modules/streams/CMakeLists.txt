######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Wednesday May 27, 2026 12:04:15 CST
# @description : Build config for streams module
######################################################################

file(GLOB_RECURSE STREAMS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_streams OBJECT
    ${STREAMS_SOURCES}
)

target_include_directories(whisker_module_streams
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_streams PRIVATE --std=c11)
