######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Sunday Mar 15, 2026 21:24:35 CST
# @description : Build config for serialisation module
######################################################################

file(GLOB_RECURSE SERIALISATION_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_serialisation OBJECT
    ${SERIALISATION_SOURCES}
)

target_include_directories(whisker_module_serialisation
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_serialisation PRIVATE --std=c11)
