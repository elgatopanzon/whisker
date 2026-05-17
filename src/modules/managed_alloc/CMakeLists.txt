######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Saturday May 16, 2026 18:59:41 CST
# @description : Build config for managed_alloc module
######################################################################

file(GLOB_RECURSE MANAGED_ALLOC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_managed_alloc OBJECT
    ${MANAGED_ALLOC_SOURCES}
)

target_include_directories(whisker_module_managed_alloc
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_managed_alloc PRIVATE --std=c11)
