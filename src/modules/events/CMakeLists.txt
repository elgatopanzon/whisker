######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Wednesday Apr 01, 2026 16:56:23 CST
# @description : Build config for events module
######################################################################

file(GLOB_RECURSE EVENTS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_events OBJECT
    ${EVENTS_SOURCES}
)

target_include_directories(whisker_module_events
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_events PRIVATE --std=c11)
