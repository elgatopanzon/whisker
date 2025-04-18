/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_sparse_set
 * @created     : Tuesday Feb 25, 2025 18:38:22 CST
 */

#include "whisker_std.h"

#include "whisker_sparse_set.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

START_TEST(test_whisker_sparse_set_create)
{
	w_sparse_set *ss = w_sparse_set_create_t(int);

	w_sparse_set_free_all(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_get_nonexistant_sparse_index)
{
	w_sparse_set *ss = w_sparse_set_create_t(int);

	// try to get a non-existant sparse index
	uint64_t dense_index = w_sparse_set_get_dense_index_(ss, 123);
	ck_assert_uint_eq(dense_index, UINT64_MAX);

	w_sparse_set_free_all(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_contains)
{
	w_sparse_set *ss = w_sparse_set_create_t(int);

	// check for index which doesn't exist
	ck_assert_int_eq(false, w_sparse_set_contains(ss, 123456));

	// set the index
	int val = 123;
	w_sparse_set_set(ss, 123456, &val);
	
	// check again
	ck_assert_int_eq(true, w_sparse_set_contains(ss, 123456));

	w_sparse_set_free_all(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_get_and_remove)
{
	w_sparse_set *ss = w_sparse_set_create_t(int);

	// set a value
	int created = 123;
	w_sparse_set_set(ss, 0, &created);

	/* for (int i = 0; i < whisker_arr_length(ss->dense); ++i) */
	/* { */
	/* 	printf("1 dense index %d: %zu\n", i, ss->sparse_index[i]); */
	/* 	printf("1 dense value %d: %d\n", i, ((int*)ss->dense)[i]); */
	/* } */

	// create some more values
	int created2 = 100;
	w_sparse_set_set(ss, 100, &created2);
	created2 = 456;
	w_sparse_set_set(ss, 1, &created2);

	/* for (int i = 0; i < whisker_arr_length(ss->dense); ++i) */
	/* { */
	/* 	printf("2 dense index %d: %zu\n", i, ss->sparse_index[i]); */
	/* 	printf("2 dense value %d: %d\n", i, ((int*)ss->dense)[i]); */
	/* } */

	/* for (int i = 0; i < whisker_arr_length(ss->dense); ++i) */
	/* { */
	/* 	printf("3 dense index %d: %zu\n", i, ss->sparse_index[i]); */
	/* 	printf("3 dense value %d: %d\n", i, ((int*)ss->dense)[i]); */
	/* } */

	// get it again create
	int *created_3 = w_sparse_set_get(ss, 1);

	/* for (int i = 0; i < whisker_arr_length(ss->dense); ++i) */
	/* { */
	/* 	printf("4 dense index %d: %zu\n", i, ss->sparse_index[i]); */
	/* 	printf("4 dense value %d: %d\n", i, ((int*)ss->dense)[i]); */
	/* } */

	ck_assert_int_eq(456, *created_3);

	// remove middle value
	w_sparse_set_remove(ss, 1);

	// get it again
	int *created_4 = w_sparse_set_get(ss, 1);
	ck_assert_ptr_eq(NULL, created_4);

	// get the same value
	int *created_again = w_sparse_set_get(ss, 1);
	ck_assert_ptr_eq(NULL, created_again);

	w_sparse_set_free_all(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_set)
{
	w_sparse_set *ss = w_sparse_set_create_t(int);

	// set a value
	int val = 12312;
	w_sparse_set_set(ss, 1, &val);

	int *created = w_sparse_set_get(ss, 1);
	ck_assert_int_eq(12312, *created);

	w_sparse_set_free_all(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_sort_by_index)
{
	w_sparse_set *ss = w_sparse_set_create_t(int);

	// set some values
	w_sparse_set_set(ss, 1, &(int){7});
	w_sparse_set_set(ss, 0, &(int){9});
	w_sparse_set_set(ss, 100, &(int){4});
	w_sparse_set_set(ss, 10, &(int){3});

	// TODO: add option to disable auto sort
	/* // validate dense index order */
	/* int expected[] = {1,0,100,10}; */
	/* for (int i = 0; i < whisker_arr_length(ss->sparse_index); ++i) */
	/* { */
	/* 	ck_assert_uint_eq(expected[i], ss->sparse_index[i]); */
	/* 	printf("ss sort dense index %d: %zu:%zu\n", i, ss->sparse_index[i], ((int*)ss->dense)[i]); */
	/* } */
    /*  */
	/* // validate dense order */
	/* int expected_dense[] = {7, 9, 4, 3}; */
	/* for (int i = 0; i < whisker_arr_length(ss->dense); ++i) */
	/* { */
	/* 	ck_assert_uint_eq(expected_dense[i], ((int*)ss->dense)[i]); */
	/* } */
    /*  */
	/* // sort dense array by sparse index */
	w_sparse_set_sort(ss);

	// validate dense index order
	int expected_sorted[] = {0,1,10,100};
	for (int i = 0; i < ss->sparse_index_length; ++i)
	{
		ck_assert_uint_eq(expected_sorted[i], ss->sparse_index[i]);
		printf("ss sorted dense index %d: %zu:%zu\n", i, ss->sparse_index[i], ((int*)ss->dense)[i]);
	}

	// validate dense order
	int expected_dense_sorted[] = {9, 7, 3, 4};
	for (int i = 0; i < ss->dense_length; ++i)
	{
		ck_assert_uint_eq(expected_dense_sorted[i], ((int*)ss->dense)[i]);
	}

	// validate dense indexes point to the correct values
	int *val1 = w_sparse_set_get(ss, 0);
	ck_assert_int_eq(9, *val1);
	int *val2 = w_sparse_set_get(ss, 1);
	ck_assert_int_eq(7, *val2);
	int *val3 = w_sparse_set_get(ss, 10);
	ck_assert_int_eq(3, *val3);
	int *val4 = w_sparse_set_get(ss, 100);
	ck_assert_int_eq(4, *val4);

	w_sparse_set_free_all(ss);
}
END_TEST

Suite* whisker_sparse_set_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_sparse_set");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_sparse_set_create);
	tcase_add_test(tc_core, test_whisker_sparse_set_get_nonexistant_sparse_index);
	tcase_add_test(tc_core, test_whisker_sparse_set_contains);
	tcase_add_test(tc_core, test_whisker_sparse_set_get_and_remove);
	tcase_add_test(tc_core, test_whisker_sparse_set_set);
	tcase_add_test(tc_core, test_whisker_sparse_set_sort_by_index);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_sparse_set_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
