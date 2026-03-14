######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Friday Mar 13, 2026 17:44:31 CST
# @description : Build config for relationships module
######################################################################

file(GLOB_RECURSE RELATIONSHIPS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_relationships OBJECT
    ${RELATIONSHIPS_SOURCES}
)

target_include_directories(whisker_module_relationships
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_relationships PRIVATE --std=c11)
