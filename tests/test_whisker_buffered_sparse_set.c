/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_buffered_sparse_set
 * @created     : Wednesday Mar 12, 2025 11:34:28 CST
 */

#include "whisker_std.h"

#include "whisker_buffered_sparse_set.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

START_TEST(test_whisker_buffered_sparse_set_creation)
{
	w_buffered_sparse_set *bss = w_buf_sparse_set_create_and_init_f(2, 0);

	w_buf_sparse_set_free_all(bss);
}
END_TEST

START_TEST(test_whisker_buffered_sparse_set_set_and_get_correct_buffer)
{
	// create a buffered sparse set
	w_buffered_sparse_set *bss = whisker_bss_create_and_init_t(2, uint64_t);

	// set: sets data on the current back buffer
	uint64_t val = 1234;
	w_buf_sparse_set_set(bss, 11, &val);

	// verify the back buffer contains index
	ck_assert_int_eq(true, w_sparse_set_contains(bss->back_buffer, 11));

	// verify the front buffer contains index
	ck_assert_int_eq(false, w_sparse_set_contains(bss->front_buffer, 11));

	// verify getting the value is null because the front buffer doesn't have it
	uint64_t *val_back = w_buf_sparse_set_get(bss, 11);
	ck_assert_ptr_eq(NULL, val_back);

	// remove index and verify it's removed from back buffer
	w_buf_sparse_set_remove(bss, 11);
	ck_assert_int_eq(false, w_sparse_set_contains(bss->back_buffer, 11));

	// free
	w_buf_sparse_set_free_all(bss);
}
END_TEST

START_TEST(test_whisker_buffered_sparse_set_sync_and_swap)
{
	// create a buffered sparse set
	w_buffered_sparse_set *bss = whisker_bss_create_and_init_t(2, uint64_t);

	// frame 0: set some initial values
	// this populates the current back buffer and front buffer is empty
	uint64_t val0 = 545;
	uint64_t val1 = 3344;
	uint64_t val2 = 9887;

	w_buf_sparse_set_set(bss, 0, &val0);
	w_buf_sparse_set_set(bss, 1, &val1);
	w_buf_sparse_set_set(bss, 2, &val2);

	// verify buffer lengths
	ck_assert_int_eq(3, *bss->back_buffer->length);
	ck_assert_int_eq(0, *bss->front_buffer->length);

	// sync the buffers
	w_buf_sparse_set_sync(bss);

	// swap the buffers and verify values
	w_buf_sparse_set_swap(bss);
	uint64_t *val0_f0_swapped = w_sparse_set_get(bss->front_buffer, 0);
	ck_assert_int_eq(545, *val0_f0_swapped);

	// verify the swapped buffers lengths
	ck_assert_int_eq(3, *bss->front_buffer->length);
	ck_assert_int_eq(0, *bss->back_buffer->length);


	// free
	w_buf_sparse_set_free_all(bss);
}
END_TEST

Suite* whisker_buffered_sparse_set_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_buffered_sparse_set");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_buffered_sparse_set_creation);
	tcase_add_test(tc_core, test_whisker_buffered_sparse_set_set_and_get_correct_buffer);
	tcase_add_test(tc_core, test_whisker_buffered_sparse_set_sync_and_swap);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_buffered_sparse_set_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
