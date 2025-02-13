# populate a list with all whisker library .c files
file(GLOB WHISKER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/whisker*.c")
# remove main.c from
# list(REMOVE_ITEM WHISKER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
# create whisker_objs source library
add_library(whisker_objs OBJECT
	${WHISKER_SOURCES}
)
 
# set header include location for cmake objects library
set_property(TARGET whisker_objs
    PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/src")

# link raylib library with whisker_objs
find_package(raylib REQUIRED)
target_link_libraries(whisker_objs PUBLIC raylib)

 
# # find and add the math library to cmake objects library
# find_library(MATH_LIBRARY m)
# if(MATH_LIBRARY)
#     set_property(TARGET asteroids_objs
#         PROPERTY
#         INTERFACE_LINK_LIBRARIES
#         ${MATH_LIBRARY})
# endif()
 
# add executable for asteroids-example
file(GLOB ASTEROIDS_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/asteroids*.c")
add_executable(asteroids-example
    ${ASTEROIDS_EXAMPLE_SOURCES}
)
 
# link objects library to target executable
target_link_libraries(asteroids-example PRIVATE whisker_objs)
