######################################################################
# @author      : {{cookiecutter.author}} (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Saturday Mar 07, 2026 12:49:55 CST
# @description : Build config for {{cookiecutter.module_name}} module
######################################################################

file(GLOB_RECURSE {{cookiecutter.module_name | upper}}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(whisker_module_{{cookiecutter.module_name}} OBJECT
    ${ {{-cookiecutter.module_name | upper}}_SOURCES}
)

target_include_directories(whisker_module_{{cookiecutter.module_name}}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(whisker_module_{{cookiecutter.module_name}} PRIVATE --std=c11)
