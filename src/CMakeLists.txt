# populate a list with all whisker library .c files
function(recurse_files dir pattern result)
    file(GLOB_RECURSE temp_files "${dir}/${pattern}")
    set(${result} ${temp_files} PARENT_SCOPE)
endfunction()

recurse_files(${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}*.c" WHISKER_SOURCES)

# create library from sources
add_library(${PROJECT_NAME} OBJECT
	${WHISKER_SOURCES}
)

# set header include location for cmake objects library
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

# set C standard to C11
target_compile_options(${PROJECT_NAME} PRIVATE --std=c11)
