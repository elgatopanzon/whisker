######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Monday May 18, 2026 13:07:57 CST
# @description : Build config for assets module
######################################################################

file(GLOB_RECURSE ASSETS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_assets OBJECT
    ${ASSETS_SOURCES}
)

target_include_directories(whisker_module_assets
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_assets PRIVATE --std=c11)
