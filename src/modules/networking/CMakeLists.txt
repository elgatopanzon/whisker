######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Thursday May 28, 2026 11:08:36 CST
# @description : Build config for networking module
######################################################################

file(GLOB_RECURSE NETWORKING_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_networking OBJECT
    ${NETWORKING_SOURCES}
)

target_include_directories(whisker_module_networking
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_networking PRIVATE --std=c11)
