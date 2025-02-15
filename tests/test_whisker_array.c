/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_array
 * @created     : Wednesday Feb 05, 2025 12:39:26 CST
 */

#include <stdio.h>
#include <stdlib.h>

#include <check.h>
#include "whisker_array.h"

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
	whisker_arr_create(int, 10, &arr);

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
	whisker_arr_create(int, 10, &arr);

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

	// verify pointer changed
	ck_assert(arr_push != arr_compact);

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
