#
# Tests for whisker library modules
#
add_executable(test_whisker_memory test_whisker_memory.c)
target_link_libraries(test_whisker_memory PRIVATE
    Check::check whisker)
add_test(NAME test_whisker_memory COMMAND test_whisker_memory)

add_executable(test_whisker_array test_whisker_array.c)
target_link_libraries(test_whisker_array PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_array COMMAND test_whisker_array)

add_executable(test_whisker_block_array test_whisker_block_array.c)
target_link_libraries(test_whisker_block_array PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_block_array COMMAND test_whisker_block_array)

add_executable(test_whisker_string test_whisker_string.c)
target_link_libraries(test_whisker_string PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_string COMMAND test_whisker_string)

add_executable(test_whisker_trie test_whisker_trie.c)
target_link_libraries(test_whisker_trie PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_trie COMMAND test_whisker_trie)

add_executable(test_whisker_dict test_whisker_dict.c)
target_link_libraries(test_whisker_dict PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_dict COMMAND test_whisker_dict)

add_executable(test_whisker_sparse_set test_whisker_sparse_set.c)
target_link_libraries(test_whisker_sparse_set PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_sparse_set COMMAND test_whisker_sparse_set)

add_executable(test_whisker_buffered_sparse_set test_whisker_buffered_sparse_set.c)
target_link_libraries(test_whisker_buffered_sparse_set PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_buffered_sparse_set COMMAND test_whisker_buffered_sparse_set)

add_executable(test_whisker_thread_pool test_whisker_thread_pool.c)
target_link_libraries(test_whisker_thread_pool PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_thread_pool COMMAND test_whisker_thread_pool)

add_executable(test_whisker_ecs test_whisker_ecs.c)
target_link_libraries(test_whisker_ecs PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_ecs COMMAND test_whisker_ecs)

add_executable(test_whisker_ecs_entity test_whisker_ecs_entity.c)
target_link_libraries(test_whisker_ecs_entity PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_ecs_entity COMMAND test_whisker_ecs_entity)

add_executable(test_whisker_ecs_component test_whisker_ecs_component.c)
target_link_libraries(test_whisker_ecs_component PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_ecs_component COMMAND test_whisker_ecs_component)

add_executable(test_whisker_ecs_system test_whisker_ecs_system.c)
target_link_libraries(test_whisker_ecs_system PRIVATE
	Check::check whisker)
add_test(NAME test_whisker_ecs_system COMMAND test_whisker_ecs_system)
