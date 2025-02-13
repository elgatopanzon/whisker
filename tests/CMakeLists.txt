#
# Tests for whisker library modules
#
add_executable(test_whisker_memory test_whisker_memory.c)
target_link_libraries(test_whisker_memory PRIVATE
    Check::check whisker_objs)
add_test(NAME test_whisker_memory COMMAND test_whisker_memory)

add_executable(test_whisker_array test_whisker_array.c)
target_link_libraries(test_whisker_array PRIVATE
	Check::check whisker_objs)
add_test(NAME test_whisker_array COMMAND test_whisker_array)

add_executable(test_whisker_string test_whisker_string.c)
target_link_libraries(test_whisker_string PRIVATE
	Check::check whisker_objs)
add_test(NAME test_whisker_string COMMAND test_whisker_string)

add_executable(test_whisker_trie test_whisker_trie.c)
target_link_libraries(test_whisker_trie PRIVATE
	Check::check whisker_objs)
add_test(NAME test_whisker_trie COMMAND test_whisker_trie)

add_executable(test_whisker_dict test_whisker_dict.c)
target_link_libraries(test_whisker_dict PRIVATE
	Check::check whisker_objs)
add_test(NAME test_whisker_dict COMMAND test_whisker_dict)
