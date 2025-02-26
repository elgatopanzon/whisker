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
	whisker_sparse_set *ss;
	whisker_ss_create_t(&ss, int);

	whisker_ss_free(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_get_nonexistant_sparse_index)
{
	whisker_sparse_set *ss;
	whisker_ss_create_t(&ss, int);

	// try to get a non-existant sparse index
	uint64_t dense_index = whisker_ss_get_dense_index(ss, 123);
	ck_assert_uint_eq(dense_index, UINT64_MAX);

	// test large index, it will try looking up in the TRIE
	uint64_t dense_index2 = whisker_ss_get_dense_index(ss, 4294967296871282);
	ck_assert_uint_eq(dense_index2, UINT64_MAX);

	whisker_ss_free(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_get_and_create)
{
	whisker_sparse_set *ss;
	whisker_ss_create_t(&ss, int);

	// get a value, which should init with 0x00
	int *created = whisker_ss_get(ss, 0, true);
	*created = 1;
	ck_assert_int_eq(1, *created);

	// get the value again, create shouldn't trigger change
	int *created2 = whisker_ss_get(ss, 0, true);
	ck_assert_int_eq(1, *created2);

	// get a value with same block id index, which should init with 0x00
	int *created3 = whisker_ss_get(ss, 1, true);
	*created3 = 10;
	ck_assert_int_eq(10, *created3);

	// get the value again, create shouldn't trigger change
	int *created4 = whisker_ss_get(ss, 1, true);
	ck_assert_int_eq(10, *created4);

	whisker_ss_free(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_get_and_remove)
{
	whisker_sparse_set *ss;
	whisker_ss_create_t(&ss, int);

	// get a value, which should init with 0x00
	int *created = whisker_ss_get(ss, 0, true);
	*created = 123;
	ck_assert_int_eq(123, *created);

	/* for (int i = 0; i < warr_length(ss->dense); ++i) */
	/* { */
	/* 	printf("1 dense %d: %d\n", i, ((int*)ss->dense)[i]); */
	/* 	printf("1 dense index %d: %zu\n", i, ss->dense_index[i]); */
	/* } */

	// create some more values
	whisker_ss_get(ss, 100, true);

	/* for (int i = 0; i < warr_length(ss->dense); ++i) */
	/* { */
	/* 	printf("2 dense %d: %d\n", i, ((int*)ss->dense)[i]); */
	/* 	printf("2 dense index %d: %zu\n", i, ss->dense_index[i]); */
	/* } */

	int *created_2 = whisker_ss_get(ss, 1, true);
	*created_2 = 456;
	ck_assert_int_eq(456, *created_2);

	/* for (int i = 0; i < warr_length(ss->dense); ++i) */
	/* { */
	/* 	printf("3 dense %d: %d\n", i, ((int*)ss->dense)[i]); */
	/* 	printf("3 dense index %d: %zu\n", i, ss->dense_index[i]); */
	/* } */

	// get it again without create
	int *created_3 = whisker_ss_get(ss, 1, false);

	/* for (int i = 0; i < warr_length(ss->dense); ++i) */
	/* { */
	/* 	printf("4 dense %d: %d\n", i, ((int*)ss->dense)[i]); */
	/* 	printf("4 dense index %d: %zu\n", i, ss->dense_index[i]); */
	/* } */

	ck_assert_int_eq(456, *created_3);

	// remove middle value
	whisker_ss_remove(ss, 1);

	// get it again without create
	int *created_4 = whisker_ss_get(ss, 1, false);
	ck_assert_ptr_eq(NULL, created_4);

	// get the same value
	int *created_again = whisker_ss_get(ss, 1, true);
	ck_assert_int_eq(0, *created_again);

	whisker_ss_free(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_get_and_create_large)
{
	whisker_sparse_set *ss;
	whisker_ss_create_t(&ss, int);

	// get a value with a large index
	int *created = whisker_ss_get(ss, 4294967296, true);
	ck_assert_int_eq(0, *created);

	// get another value with a large index
	int *created2 = whisker_ss_get(ss, 4294967296871282, true);
	ck_assert_int_eq(0, *created2);

	whisker_ss_free(ss);
}
END_TEST

START_TEST(test_whisker_sparse_set_set)
{
	whisker_sparse_set *ss;
	whisker_ss_create_t(&ss, int);

	// set a value
	int val = 12312;
	whisker_ss_set(ss, 4294967296, &val);

	int *created = whisker_ss_get(ss, 4294967296, false);
	ck_assert_int_eq(12312, *created);

	whisker_ss_free(ss);
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
	tcase_add_test(tc_core, test_whisker_sparse_set_get_and_create);
	tcase_add_test(tc_core, test_whisker_sparse_set_get_and_remove);
	tcase_add_test(tc_core, test_whisker_sparse_set_get_and_create_large);
	tcase_add_test(tc_core, test_whisker_sparse_set_set);

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
