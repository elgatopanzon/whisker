######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Tuesday Mar 24, 2026 21:21:46 CST
# @description : Build config for component_events module
######################################################################

file(GLOB_RECURSE COMPONENT_EVENTS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_component_events OBJECT
    ${COMPONENT_EVENTS_SOURCES}
)

target_include_directories(whisker_module_component_events
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_component_events PRIVATE --std=c11)
