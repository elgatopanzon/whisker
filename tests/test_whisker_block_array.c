/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_block_array
 * @created     : Monday Feb 17, 2025 14:25:44 CST
 */

#include "whisker_std.h"

#include "whisker_block_array.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

START_TEST(test_whisker_block_arr_create)
{
	// create a block array
	whisker_block_array *b_arr;
	whisker_block_arr_create(int, 16, &b_arr);

	// free
	whisker_block_arr_free(b_arr);
}
END_TEST

START_TEST(test_whisker_block_arr_get_and_set)
{
	// create a block array of ints
	whisker_block_array *b_arr;
	whisker_block_arr_create(int, 16, &b_arr);

	// get a random value (should create a block automatically)
	int *int_val = whisker_block_arr_get(b_arr, 123);

	ck_assert_int_eq(0, *int_val);

	// set the value
	*int_val = 123;

	// verify the value matches when obtained directly
	ck_assert_int_eq(123, *(int*)whisker_block_arr_get(b_arr, 123));

	// set value of a different block
	whisker_block_arr_set(b_arr, 12, int_val);

	// get and verify the newly set value
	ck_assert_int_eq(123, *(int*)whisker_block_arr_get(b_arr, 12));

	// verify the block count
	ck_assert_int_eq(2, b_arr->block_count);

	// free
	whisker_block_arr_free(b_arr);
}
END_TEST

START_TEST(test_whisker_block_arr_nested)
{
	// create a block array of block arrays
	whisker_block_array *b_arr;
	whisker_block_arr_create(whisker_block_array, 16, &b_arr);

	// get a random value (should create a block automatically)
	whisker_block_array *nested_block_array = whisker_block_arr_get(b_arr, 56);
	// init the block array
	whisker_block_arr_create(int, 16, &nested_block_array);

	int *int_val = whisker_block_arr_get(nested_block_array, 123);

	ck_assert_int_eq(0, *int_val);

	// set the value
	*int_val = 123;

	// verify the value matches when obtained directly
	ck_assert_int_eq(123, *(int*)whisker_block_arr_get(nested_block_array, 123));

	// set value of a different block
	whisker_block_arr_set(nested_block_array, 12, int_val);

	// get and verify the newly set value
	ck_assert_int_eq(123, *(int*)whisker_block_arr_get(nested_block_array, 12));

	// verify the block count
	ck_assert_int_eq(2, nested_block_array->block_count);

	// verify the outer block count
	ck_assert_int_eq(1, b_arr->block_count);

	// free
	whisker_block_arr_free(b_arr);
	whisker_block_arr_free(nested_block_array);
}
END_TEST

Suite* whisker_block_arr_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_block_arr");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_block_arr_create);
	tcase_add_test(tc_core, test_whisker_block_arr_get_and_set);
	tcase_add_test(tc_core, test_whisker_block_arr_nested);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_block_arr_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
