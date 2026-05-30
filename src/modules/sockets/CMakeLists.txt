######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Friday May 29, 2026 00:00:00 CST
# @description : Build config for sockets module
######################################################################

file(GLOB_RECURSE SOCKETS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_sockets OBJECT
    ${SOCKETS_SOURCES}
)

target_include_directories(whisker_module_sockets
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_sockets PRIVATE --std=c11)
