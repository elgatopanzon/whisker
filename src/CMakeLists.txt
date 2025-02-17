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
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()

# find_package(raylib REQUIRED)
target_link_libraries(whisker PUBLIC raylib)
 
# add executable for asteroids-example
file(GLOB ASTEROIDS_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/asteroids*.c")
add_executable(asteroids-example
    ${ASTEROIDS_EXAMPLE_SOURCES}
)
 
# link whisker library to target executable
target_link_libraries(asteroids-example PRIVATE whisker)
