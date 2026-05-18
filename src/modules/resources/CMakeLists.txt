######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Sunday May 17, 2026 15:49:12 CST
# @description : Build config for resources module
######################################################################

file(GLOB_RECURSE RESOURCES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_resources OBJECT
    ${RESOURCES_SOURCES}
)

target_include_directories(whisker_module_resources
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_resources PRIVATE --std=c11)
