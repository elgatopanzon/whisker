/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_array
 * @created     : Wednesday Feb 05, 2025 12:39:26 CST
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "check.h"
#include "whisker_array.h"
#include "generics/whisker_generic_array_int.h"

START_TEST(test_whisker_arr_create_and_free)
{
	// create an array of 10 ints
	int* arr;
	whisker_arr_create(int, 10, &arr);

	// free the array
	whisker_arr_free(arr);
}
END_TEST

START_TEST(test_whisker_arr_header)
{
	// create an array of 10 ints
	int* arr;
	whisker_arr_create(*arr, 10, &arr);

	// get the array header
	whisker_array_header* header = whisker_arr_header(arr);

	// verify header values
	ck_assert_int_eq(header->element_size, sizeof(int));
	ck_assert_int_eq(header->length, 10);

	// free the array
	whisker_arr_free(arr);
}
END_TEST

START_TEST(test_whisker_arr_length)
{
	// create an array of 10 ints
	int* arr;
	whisker_arr_create(*arr, 10, &arr);

	// get the array length
	size_t length = whisker_arr_length(arr);

	// verify header values
	ck_assert_int_eq(length, 10);

	// free the array
	whisker_arr_free(arr);
}
END_TEST

START_TEST(test_whisker_arr_try_resize)
{
	// create an array of 1 ints
	int* arr;
	whisker_arr_create(int, 1, &arr);

	// set first element
	arr[0] = 123;
	ck_assert_int_eq(arr[0], 123);

	// resize the array to 2 elements
	int* arr_resized = arr;
	whisker_arr_resize(&arr_resized, 2);

	// verify pointer changed
	ck_assert(arr != arr_resized);

	// set a new element
	arr_resized[1] = 456;

	// verify accessing the array and the set values
	ck_assert_int_eq(arr_resized[0], 123);
	ck_assert_int_eq(arr_resized[1], 456);

	// free the array
	whisker_arr_free(arr_resized);
}
END_TEST

START_TEST(test_whisker_arr_try_push)
{
	// create an array of 2 ints
	int* arr;
	whisker_arr_create(int, 2, &arr);

	// set elements
	arr[0] = 123;
	arr[1] = 456;

	// push value (triggers resize and pointer update)
	int* arr_resized = arr;
	int val = 789;
	whisker_arr_push(&arr_resized, &val);

	// verify pointer changed
	ck_assert(arr != arr_resized);

	// verify length
	ck_assert_int_eq(whisker_arr_length(arr_resized), 3);

	// verify accessing the array and the set values
	ck_assert_int_eq(arr_resized[0], 123);
	ck_assert_int_eq(arr_resized[1], 456);
	ck_assert_int_eq(arr_resized[2], 789);

	// free the array
	whisker_arr_free(arr_resized);
}
END_TEST

START_TEST(test_whisker_arr_try_pop)
{
	// create an array of 2 ints
	int* arr;
	whisker_arr_create(int, 2, &arr);

	// set elements
	arr[0] = 123;
	arr[1] = 456;

	// pop last value (triggers resize and pointer update)
	int* arr_resized = arr;
	whisker_arr_pop_c(&arr_resized, int, popped);

	// verify popped valus is set
	ck_assert_int_eq(popped, 456);

	// verify pointer didn't change (pop keeps capacity)
	ck_assert(arr == arr_resized);

	// verify accessing the array and the set values
	ck_assert_int_eq(arr_resized[0], 123);

	// verify changed size
	ck_assert_int_eq(whisker_arr_length(arr_resized), 1);

	// free the array
	whisker_arr_free(arr_resized);
}
END_TEST

START_TEST(test_whisker_arr_try_pop_front)
{
	// create an array of 2 ints
	int* arr;
	whisker_arr_create(int, 2, &arr);

	// set elements
	arr[0] = 123;
	arr[1] = 456;

	// pop front value (triggers resize and pointer update)
	int* arr_resized = arr;
	whisker_arr_pop_front_c(&arr_resized, int, popped);

	// verify popped valus is set
	ck_assert_int_eq(popped, 123);

	// verify pointer didn't change (pop keeps capacity)
	ck_assert(arr == arr_resized);

	// verify accessing the array and the set values
	ck_assert_int_eq(arr_resized[0], 456);

	// verify changed size
	ck_assert_int_eq(whisker_arr_length(arr_resized), 1);

	// free the array
	whisker_arr_free(arr_resized);
}
END_TEST

START_TEST(test_whisker_arr_try_push_pop_push_same_capacity)
{
	// create an array of 2 ints
	int* arr;
	whisker_arr_create(int, 2, &arr);

	// set elements
	arr[0] = 123;
	arr[1] = 456;

	// push value (triggers resize and pointer update)
	int* arr_resized = arr;
	int val = 789;
	whisker_arr_push(&arr_resized, &val);

	// verify pointer changed
	ck_assert(arr != arr_resized);

	// verify accessing the array and the set values
	ck_assert_int_eq(arr_resized[0], 123);
	ck_assert_int_eq(arr_resized[1], 456);
	ck_assert_int_eq(arr_resized[2], 789);

	// pop 2 values
	int* pop1;
	int* pop2;
	int* arr_pop = arr_resized;
	whisker_arr_pop(&arr_pop, &pop1);
	whisker_arr_pop(&arr_pop, &pop2);

	// verify pointer didn't change
	ck_assert(arr_resized == arr_pop);

	// verify length
	ck_assert_int_eq(whisker_arr_length(arr_pop), 1);

	int val2 = 111;
	int* arr_push = arr_pop;
	whisker_arr_push(&arr_push, &val2);

	// verify pointer didn't change
	ck_assert(arr_pop == arr_push);

	ck_assert_int_eq(arr_push[0], 123);
	ck_assert_int_eq(arr_push[2], 789); // removed, but technically still
										// present
	ck_assert_int_eq(arr_push[1], 111);

	// check compact() to shrink array
	int* arr_compact = arr_push;
	whisker_arr_compact(&arr_compact);

	// free the array
	whisker_arr_free(arr_compact);
}
END_TEST

START_TEST(test_whisker_arr_try_insert)
{
	// create an array of 2 ints
	int* arr;
	whisker_arr_create(int, 2, &arr);

	// set elements
	arr[0] = 123;
	arr[1] = 456;

	// push value (triggers resize and pointer update)
	int* arr_resized = arr;
	int val = 789;
	whisker_arr_push(&arr_resized, &val);

	// verify pointer changed
	ck_assert(arr != arr_resized);

	// verify length
	ck_assert_int_eq(whisker_arr_length(arr_resized), 3);

	// verify accessing the array and the set values
	ck_assert_int_eq(arr_resized[0], 123);
	ck_assert_int_eq(arr_resized[1], 456);
	ck_assert_int_eq(arr_resized[2], 789);

	// insert at index 1
	int insert_val = 999;
	whisker_arr_insert(&arr_resized, 1, &insert_val);

	// verify new values
	ck_assert_int_eq(arr_resized[0], 123);
	ck_assert_int_eq(arr_resized[1], 999);
	ck_assert_int_eq(arr_resized[2], 456);
	ck_assert_int_eq(arr_resized[3], 789);

	// free the array
	whisker_arr_free(arr_resized);
}
END_TEST

START_TEST(test_whisker_arr_strings)
{
	// create an array of type char*
	char** arr;
	whisker_arr_create(char*, 2, &arr);

	// set elements
	arr[0] = "12";
	arr[1] = "34567";

	// push value (triggers resize and pointer update)
	char** arr_resized = arr;
	char* val = "89";
	whisker_arr_push(&arr_resized, &val);

	// verify pointer changed
	ck_assert(arr != arr_resized);

	// verify length
	ck_assert_int_eq(whisker_arr_length(arr_resized), 3);

	// verify accessing the array and the set values
	ck_assert_str_eq(arr_resized[0], "12");
	ck_assert_str_eq(arr_resized[1], "34567");
	ck_assert_str_eq(arr_resized[2], "89");

	// free the array
	whisker_arr_free(arr_resized);
}
END_TEST

START_TEST(test_whisker_arr_grow_get)
{
	// create an array of 10 ints
	int* arr;
	whisker_arr_create(int, 10, &arr);

	// use grow get to get the 20th index
	int grown = *(int*)whisker_arr_grow_get(&arr, 20);
	ck_assert_int_eq(0, grown);
	ck_assert_int_eq(21, whisker_arr_length(arr));

	// free the array
	whisker_arr_free(arr);
}
END_TEST

START_TEST(test_whisker_arr_generic_create)
{
	// create int array
	whisker_arr_int *arr; whisker_arr_create_int(&arr, 10);

	// free int array
	whisker_arr_free_int(arr);
}
END_TEST

START_TEST(test_whisker_arr_generic_resize)
{
	// create int array
	whisker_arr_int *arr; whisker_arr_create_int(&arr, 10);

	arr->arr[5] = 123;

	ck_assert_int_eq(10, arr->length);
	ck_assert_int_eq(sizeof(int) * 10, arr->alloc_size);
	ck_assert_int_eq(123, arr->arr[5]);

	// resize to 20 elements
	whisker_arr_resize_int(arr, 20, true);

	arr->arr[15] = 456;

	ck_assert_int_eq(20, arr->length);
	ck_assert_int_eq(sizeof(int) * 20, arr->alloc_size);
	ck_assert_int_eq(123, arr->arr[5]);
	ck_assert_int_eq(456, arr->arr[15]);

	// test soft resize
	whisker_arr_resize_int(arr, 10, true);

	ck_assert_int_eq(10, arr->length);
	ck_assert_int_eq(sizeof(int) * 20, arr->alloc_size);

	// resize back to alloc size shouldn't increase alloc size
	whisker_arr_resize_int(arr, 20, true);

	ck_assert_int_eq(20, arr->length);
	ck_assert_int_eq(sizeof(int) * 20, arr->alloc_size);

	// free int array
	whisker_arr_free_int(arr);
}
END_TEST

START_TEST(test_whisker_arr_generic_push_and_pop)
{
	// create an empty int array
	whisker_arr_int *arr; whisker_arr_create_int(&arr, 0);

	ck_assert_int_eq(0, arr->length);
	ck_assert_int_eq(sizeof(int) * 0, arr->alloc_size);

	// push a value to it
	ck_assert_int_eq(E_WHISKER_ARR_OK, whisker_arr_push_int(arr, 123));
	ck_assert_int_eq(123, arr->arr[0]);
	ck_assert_int_eq(1, arr->length);
	ck_assert_int_eq(sizeof(int) * 1, arr->alloc_size);

	// pop the value from it
	int popped;
	ck_assert_int_eq(E_WHISKER_ARR_OK, whisker_arr_pop_int(arr, &popped));
	ck_assert_int_eq(123, popped);
	ck_assert_int_eq(0, arr->length);

	// pop on empty array
	ck_assert_int_eq(E_WHISKER_ARR_OUT_OF_BOUNDS, whisker_arr_pop_int(arr, &popped));

	// free int array
	whisker_arr_free_int(arr);
}
END_TEST

START_TEST(test_whisker_arr_generic_swap)
{
	// create an empty int array
	whisker_arr_int *arr; whisker_arr_create_int(&arr, 0);

	ck_assert_int_eq(0, arr->length);
	ck_assert_int_eq(sizeof(int) * 0, arr->alloc_size);

	// push some values
	ck_assert_int_eq(E_WHISKER_ARR_OK, whisker_arr_push_int(arr, 123));
	ck_assert_int_eq(E_WHISKER_ARR_OK, whisker_arr_push_int(arr, 456));
	ck_assert_int_eq(E_WHISKER_ARR_OK, whisker_arr_push_int(arr, 789));

	int expected[] = {123, 456, 789};
	for (int i = 0; i < arr->length; ++i)
	{
		ck_assert_int_eq(expected[i], arr->arr[i]);
	}

	// swap index 0 with 2
	whisker_arr_swap_int(arr, 0, 2);

	int expected_swapped[] = {789, 456, 123};
	for (int i = 0; i < arr->length; ++i)
	{
		ck_assert_int_eq(expected_swapped[i], arr->arr[i]);
	}

	// check swap buffer clear
	ck_assert_int_eq(0, arr->swap_buffer);

	// free int array
	whisker_arr_free_int(arr);
}
END_TEST

START_TEST(test_whisker_arr_generic_compact)
{
	// create an empty int array
	whisker_arr_int *arr; whisker_arr_create_int(&arr, 0);

	ck_assert_int_eq(0, arr->length);
	ck_assert_int_eq(sizeof(int) * 0, arr->alloc_size);

	// push a value to it
	ck_assert_int_eq(E_WHISKER_ARR_OK, whisker_arr_push_int(arr, 123));
	ck_assert_int_eq(123, arr->arr[0]);
	ck_assert_int_eq(1, arr->length);
	ck_assert_int_eq(sizeof(int) * 1, arr->alloc_size);

	// pop the value from it
	int popped;
	ck_assert_int_eq(E_WHISKER_ARR_OK, whisker_arr_pop_int(arr, &popped));
	ck_assert_int_eq(123, popped);

	// verify alloc size is the same
	ck_assert_int_eq(sizeof(int) * 1, arr->alloc_size);
	ck_assert_int_eq(0, arr->length);

	// compact down to size
	whisker_arr_compact_int(arr);
	ck_assert_int_eq(sizeof(int) * 0, arr->alloc_size);

	// free int array
	whisker_arr_free_int(arr);
}
END_TEST
Suite* whisker_array_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_array");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_arr_create_and_free);
	tcase_add_test(tc_core, test_whisker_arr_header);
	tcase_add_test(tc_core, test_whisker_arr_length);
	tcase_add_test(tc_core, test_whisker_arr_try_resize);
	tcase_add_test(tc_core, test_whisker_arr_try_push);
	tcase_add_test(tc_core, test_whisker_arr_try_pop);
	tcase_add_test(tc_core, test_whisker_arr_try_pop_front);
	tcase_add_test(tc_core, test_whisker_arr_try_push_pop_push_same_capacity);
	tcase_add_test(tc_core, test_whisker_arr_try_insert);
	tcase_add_test(tc_core, test_whisker_arr_strings);
	tcase_add_test(tc_core, test_whisker_arr_grow_get);
	tcase_add_test(tc_core, test_whisker_arr_generic_create);
	tcase_add_test(tc_core, test_whisker_arr_generic_resize);
	tcase_add_test(tc_core, test_whisker_arr_generic_push_and_pop);
	tcase_add_test(tc_core, test_whisker_arr_generic_swap);
	tcase_add_test(tc_core, test_whisker_arr_generic_compact);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_array_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
