######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Thursday Mar 27, 2026 14:38:00 CST
# @description : Build config for system_groups module
######################################################################

file(GLOB_RECURSE SYSTEM_GROUPS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_system_groups OBJECT
    ${SYSTEM_GROUPS_SOURCES}
)

target_include_directories(whisker_module_system_groups
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_system_groups PRIVATE --std=c11)
