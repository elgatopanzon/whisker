# populate a list with all whisker library .c files
function(recurse_files dir pattern result)
    file(GLOB_RECURSE temp_files "${dir}/${pattern}")
    set(${result} ${temp_files} PARENT_SCOPE)
endfunction()

recurse_files(${CMAKE_CURRENT_SOURCE_DIR} "whisker*.c" WHISKER_SOURCES)

# create whisker library from whisker sources
add_library(whisker OBJECT
	${WHISKER_SOURCES}
)
 
# set header include location for cmake objects library
target_include_directories(whisker
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)
target_include_directories(whisker
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/generics
)
#
# set_property(TARGET whisker
#     PROPERTY
#     INTERFACE_INCLUDE_DIRECTORIES
#     "${CMAKE_SOURCE_DIR}/src")

# link raylib library with whisker
target_link_libraries(whisker PRIVATE raylib)
 
# add executable for asteroids-example
file(GLOB ASTEROIDS_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/asteroids-example.c")
add_executable(asteroids-example
    ${ASTEROIDS_EXAMPLE_SOURCES}
)
 
# link whisker library to target executable
target_link_libraries(asteroids-example PRIVATE whisker)
target_link_libraries(asteroids-example PRIVATE raylib)

# add executable for asteroids-example-v2
file(GLOB ASTEROIDS_EXAMPLE_V2_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/asteroids-v2*.c")
add_executable(asteroids-example-v2
    ${ASTEROIDS_EXAMPLE_V2_SOURCES}
)
 
# link whisker library to target executable
target_link_libraries(asteroids-example-v2 PRIVATE whisker)
target_link_libraries(asteroids-example-v2 PRIVATE raylib)
