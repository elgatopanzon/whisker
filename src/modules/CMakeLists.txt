######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : CMakeLists
# @created     : Saturday Mar 07, 2026 11:27:59 CST
# @description : Auto-discover and include all module subdirectories
######################################################################

# discover all subdirectories containing a CMakeLists.txt
file(GLOB module_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

# first pass: collect module names and include dirs before creating targets
set(all_module_targets "")
set(all_module_include_dirs "")
foreach(dir ${module_dirs})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir} AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
        list(APPEND all_module_targets "whisker_module_${dir}")
        list(APPEND all_module_include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
    endif()
endforeach()

# second pass: add subdirectories (creates all module targets)
foreach(dir ${module_dirs})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir} AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
        add_subdirectory(${dir})
    endif()
endforeach()

# third pass: add all module include dirs to every module so they can depend on each other
foreach(target ${all_module_targets})
    target_include_directories(${target} PUBLIC ${all_module_include_dirs})
endforeach()
