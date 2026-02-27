/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_array
 * @created     : Thursday Feb 26, 2026 12:20:43 CST
 * @description : tests for whisker_array.h macro interface
 */

#include <stdio.h>
#include <stdlib.h>

#include <check.h>
#include "whisker_array.h"

// test fixtures
static int *arr;
static size_t arr_size;
static size_t arr_length;

static void whisker_array_setup()
{
    arr = NULL;
    arr_size = 0;
    arr_length = 0;
}
static void whisker_array_teardown()
{
    free(arr);
    arr = NULL;
    arr_size = 0;
    arr_length = 0;
}

// init tcase
START_TEST(test_array_init_allocates_memory)
{
    w_array_init_t(arr, 4);
    ck_assert_ptr_nonnull(arr);
    ck_assert_int_eq(arr_size, 4 * sizeof(int));
}
END_TEST

START_TEST(test_array_init_zeroes_memory)
{
    w_array_init_t(arr, 4);
    for (int i = 0; i < 4; i++) {
        ck_assert_int_eq(arr[i], 0);
    }
}
END_TEST

START_TEST(test_array_init_length_unchanged)
{
    // init does not set length
    w_array_init_t(arr, 4);
    ck_assert_int_eq(arr_length, 0);
}
END_TEST


// realloc tcase
START_TEST(test_array_realloc_grows)
{
    w_array_init_t(arr, 2);
    w_array_realloc(arr, 8);
    ck_assert_ptr_nonnull(arr);
    ck_assert_int_eq(arr_size, 8 * sizeof(int));
}
END_TEST

START_TEST(test_array_realloc_preserves_length_when_larger)
{
    w_array_init_t(arr, 2);
    arr_length = 2;
    w_array_realloc(arr, 8);
    ck_assert_int_eq(arr_length, 2);
}
END_TEST

// ensure_alloc tcase

START_TEST(test_ensure_alloc_triggers_when_small)
{
    w_array_init_t(arr, 2);
    size_t old_size = arr_size;
    w_array_ensure_alloc(arr, 10);
    ck_assert_int_gt(arr_size, old_size);
    ck_assert_int_eq(arr_size, 10 * sizeof(int));
}
END_TEST

START_TEST(test_ensure_alloc_noop_when_sufficient)
{
    w_array_init_t(arr, 16);
    int *ptr_before = arr;
    w_array_ensure_alloc(arr, 8);
    // size should be unchanged
    ck_assert_int_eq(arr_size, 16 * sizeof(int));
    (void)ptr_before;
}
END_TEST

START_TEST(test_ensure_alloc_block_size_rounds_up)
{
    w_array_init_t(arr, 2);

    // requesting 5 elements with block_size 8 should allocate 8
    w_array_ensure_alloc_block_size(arr, 5, 8);
    ck_assert_int_ge(arr_size, 8 * sizeof(int));
}
END_TEST

START_TEST(test_ensure_alloc_block_size_noop_when_sufficient)
{
    w_array_init_t(arr, 32);
    w_array_ensure_alloc_block_size(arr, 5, 8);
    ck_assert_int_eq(arr_size, 32 * sizeof(int));
}
END_TEST

// test suite
Suite* whisker_array_suite(void)
{
	Suite *s = suite_create("whisker_array");

	// init tcase
	TCase *tc_init = tcase_create("init");
	tcase_add_checked_fixture(tc_init, whisker_array_setup, whisker_array_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_array_init_allocates_memory);
    tcase_add_test(tc_init, test_array_init_zeroes_memory);
    tcase_add_test(tc_init, test_array_init_length_unchanged);
	suite_add_tcase(s, tc_init);

    TCase *tc_realloc = tcase_create("realloc");
    tcase_add_checked_fixture(tc_realloc, whisker_array_setup, whisker_array_teardown);
    tcase_add_test(tc_realloc, test_array_realloc_grows);
    tcase_add_test(tc_realloc, test_array_realloc_preserves_length_when_larger);
    suite_add_tcase(s, tc_realloc);

    TCase *tc_ensure = tcase_create("ensure_alloc");
    tcase_add_checked_fixture(tc_ensure, whisker_array_setup, whisker_array_teardown);
    tcase_add_test(tc_ensure, test_ensure_alloc_triggers_when_small);
    tcase_add_test(tc_ensure, test_ensure_alloc_noop_when_sufficient);
    tcase_add_test(tc_ensure, test_ensure_alloc_block_size_rounds_up);
    tcase_add_test(tc_ensure, test_ensure_alloc_block_size_noop_when_sufficient);
    suite_add_tcase(s, tc_ensure);

	return s;
}

// test runner
int main()
{
	Suite *s = whisker_array_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
