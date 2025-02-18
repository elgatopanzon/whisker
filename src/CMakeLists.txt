# populate a list with all whisker library .c files
file(GLOB WHISKER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/whisker*.c")

# create whisker library from whisker sources
add_library(whisker OBJECT
	${WHISKER_SOURCES}
)
 
# set header include location for cmake objects library
target_include_directories(whisker
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
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
