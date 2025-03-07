/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_memory
 * @created     : Tuesday Feb 04, 2025 19:08:50 CST
 */

#include <stdio.h>
#include <stdlib.h>
#include "check.h"

#include "whisker_memory.h"

// TODO: test_whisker_mem_try_malloc
// TODO: test_whisker_mem_try_calloc
// TODO: test_whisker_mem_try_realloc

START_TEST(test_whisker_mem_try_malloc_block)
{
	// define size for header and data
	size_t header_size = sizeof(uint64_t);
	size_t data_size = sizeof(uint64_t) * 100;

	whisker_memory_block* block;
	whisker_mem_block_try_malloc(data_size, header_size, &block);

	whisker_mem_block_free(block);

	return;
}
END_TEST

START_TEST(test_whisker_mem_try_realloc_block_data)
{
	// define size for header and data
	size_t header_size = sizeof(uint64_t);
	size_t data_size = sizeof(uint64_t) * 100;

	whisker_memory_block* block;
	whisker_mem_block_try_malloc(data_size, header_size, &block);

	// realloc the block and double it's size
	void* data_prev = block->data;
	whisker_mem_block_try_realloc_data(block, data_size * 2);

	// check the data size changed
	ck_assert_uint_eq(block->data_size, data_size * 2);

	whisker_mem_block_free(block);

	return;
}
END_TEST

START_TEST(test_whisker_mem_block_header_from_data_pointer)
{
	// define size for header and data
	size_t header_size = sizeof(uint64_t);
	size_t data_size = sizeof(uint64_t) * 100;

	whisker_memory_block* block;
	whisker_mem_block_try_malloc(data_size, header_size, &block);

	// check the obtained header matches the one from the block
	ck_assert(whisker_mem_block_header_from_data_pointer(block->data, header_size) == block->header);

	whisker_mem_block_free(block);

	return;
}
END_TEST

struct whisker_test_struct_12b
{
	uint8_t b8[12];
};
START_TEST(test_whisker_mem_calc_header_size)
{
	// check header size matches when both data types are the same
	ck_assert_uint_eq(whisker_mem_block_calc_header_size(sizeof(uint64_t), sizeof(uint64_t)), sizeof(uint64_t));

	// check smaller header size is rounded up to data size 
	ck_assert_uint_eq(whisker_mem_block_calc_header_size(sizeof(uint8_t), sizeof(uint64_t)), sizeof(uint64_t));
	
	// check larger header size is padded to multiple of data size 
	ck_assert_uint_eq(whisker_mem_block_calc_header_size(sizeof(uint32_t), sizeof(uint8_t)), sizeof(uint8_t) * 4);

	// check with data size byte and larger header size
	ck_assert_uint_eq(whisker_mem_block_calc_header_size(sizeof(uint16_t), sizeof(char)), sizeof(char) * 2);

	// check with a mismatches struct e.g. 12 bytes
	ck_assert_uint_eq(whisker_mem_block_calc_header_size(sizeof(uint32_t), sizeof(struct whisker_test_struct_12b)), sizeof(uint32_t) * 3);

	// check with 12 byte struct as header
	ck_assert_uint_eq(whisker_mem_block_calc_header_size(sizeof(struct whisker_test_struct_12b), sizeof(uint8_t)), sizeof(uint8_t) * 12);
}
END_TEST

START_TEST(test_whisker_mem_realloc_zero)
{
	// define size for header and data
	size_t header_size = sizeof(uint64_t);
	size_t data_size = sizeof(uint64_t) * 100;

	whisker_memory_block* block;
	whisker_mem_block_try_malloc(data_size, header_size, &block);

	// verify data bytes are 0
	for (int i = 0; i < block->data_size; ++i)
	{
		ck_assert_int_eq(0, ((char*)block->data)[i]);
	}

	// write some bytes
	for (int i = 0; i < 10; ++i)
	{
		((char*)block->data)[i] = 10;
	}

	// double the size of the block with a realloc
	whisker_mem_block_try_realloc_data(block, block->data_size * 2);

	// verify data bytes are 0 except first 10
	for (int i = 0; i < block->data_size; ++i)
	{
		if (i < 10)
		{
			ck_assert_int_eq(10, ((char*)block->data)[i]);
		}
		else
		{
			ck_assert_int_eq(0, ((char*)block->data)[i]);
		}
	}

	whisker_mem_block_free(block);

	return;
}
END_TEST

Suite* whisker_memory_suite(void)
{
    Suite *s;
    TCase *tc_core;
 
    s = suite_create("whisker_memory");
 
    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 10);
 
    tcase_add_test(tc_core, test_whisker_mem_try_malloc_block);
    tcase_add_test(tc_core, test_whisker_mem_try_realloc_block_data);
    tcase_add_test(tc_core, test_whisker_mem_block_header_from_data_pointer);
    tcase_add_test(tc_core, test_whisker_mem_calc_header_size);
    tcase_add_test(tc_core, test_whisker_mem_realloc_zero);
 
    suite_add_tcase(s, tc_core);
 
    return s;
}
 
int main(int argc, char **argv)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
 
    s = whisker_memory_suite();
    sr = srunner_create(s);
 
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
