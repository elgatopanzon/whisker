######################################################################
# @author      : ElGatoPanzon
# @file        : CMakeLists
# @created     : Thursday Mar 26, 2026 20:32:23 CST
# @description : Build config for buffers module
######################################################################

file(GLOB_RECURSE BUFFERS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_buffers OBJECT
    ${BUFFERS_SOURCES}
)

target_include_directories(whisker_module_buffers
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/modules/serialisation
)

target_compile_options(whisker_module_buffers PRIVATE --std=c11)
