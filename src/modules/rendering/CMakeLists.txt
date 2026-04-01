######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Monday Mar 30, 2026 11:17:25 CST
# @description : Build config for rendering module
######################################################################

file(GLOB_RECURSE RENDERING_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_rendering OBJECT
    ${RENDERING_SOURCES}
)

target_include_directories(whisker_module_rendering
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_rendering PRIVATE --std=c11)
