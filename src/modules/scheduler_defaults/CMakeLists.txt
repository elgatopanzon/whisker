######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Saturday Mar 07, 2026 11:27:59 CST
# @description : Build config for scheduler_defaults module
######################################################################

file(GLOB_RECURSE SCHEDULER_DEFAULTS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_scheduler_defaults OBJECT
    ${SCHEDULER_DEFAULTS_SOURCES}
)

target_include_directories(whisker_module_scheduler_defaults
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_scheduler_defaults PRIVATE --std=c11)
