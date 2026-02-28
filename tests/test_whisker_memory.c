/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_memory
 * @created     : Thursday Feb 26, 2026 14:37:33 CST
 * @description : tests for whisker_memory.h allocation functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <check.h>
#include "whisker_memory.h"


/*****************************
*  malloc tcase              *
*****************************/

START_TEST(test_malloc_returns_nonnull)
{
    void *p = w_mem_malloc(16);
    ck_assert_ptr_nonnull(p);
    free(p);
}
END_TEST

START_TEST(test_malloc_zero_returns_nonnull)
{
    // size=0 is clamped to 1 internally
    void *p = w_mem_malloc(0);
    ck_assert_ptr_nonnull(p);
    free(p);
}
END_TEST

START_TEST(test_malloc_memory_is_writable)
{
    unsigned char *p = w_mem_malloc(64);
    ck_assert_ptr_nonnull(p);
    for (int i = 0; i < 64; i++) {
        p[i] = (unsigned char)i;
    }
    ck_assert_int_eq(p[63], 63);
    free(p);
}
END_TEST


/*****************************
*  calloc tcase              *
*****************************/

START_TEST(test_calloc_returns_nonnull)
{
    void *p = w_mem_calloc(4, 8);
    ck_assert_ptr_nonnull(p);
    free(p);
}
END_TEST

START_TEST(test_calloc_zero_size_returns_nonnull)
{
    // size=0 is clamped to 1 internally
    void *p = w_mem_calloc(4, 0);
    ck_assert_ptr_nonnull(p);
    free(p);
}
END_TEST

START_TEST(test_calloc_zero_count_returns_nonnull)
{
    // count=0 is clamped to 1 internally
    void *p = w_mem_calloc(0, 8);
    ck_assert_ptr_nonnull(p);
    free(p);
}
END_TEST

START_TEST(test_calloc_memory_is_zeroed)
{
    unsigned char *p = w_mem_calloc(4, 8);
    ck_assert_ptr_nonnull(p);
    for (int i = 0; i < 32; i++) {
        ck_assert_int_eq(p[i], 0);
    }
    free(p);
}
END_TEST


/*****************************
*  realloc tcase             *
*****************************/

START_TEST(test_realloc_returns_nonnull)
{
    void *p = w_mem_malloc(8);
    void *q = w_mem_realloc(p, 64);
    ck_assert_ptr_nonnull(q);
    free(q);
}
END_TEST

START_TEST(test_realloc_zero_size_returns_nonnull)
{
    // size=0 is clamped to 1 internally, does not free
    void *p = w_mem_malloc(8);
    void *q = w_mem_realloc(p, 0);
    ck_assert_ptr_nonnull(q);
    free(q);
}
END_TEST

START_TEST(test_realloc_from_null)
{
    // realloc(NULL, n) is equivalent to malloc(n)
    void *p = w_mem_realloc(NULL, 32);
    ck_assert_ptr_nonnull(p);
    free(p);
}
END_TEST

START_TEST(test_realloc_preserves_data)
{
    unsigned char *p = w_mem_malloc(8);
    ck_assert_ptr_nonnull(p);
    for (int i = 0; i < 8; i++) {
        p[i] = (unsigned char)i;
    }
    unsigned char *q = w_mem_realloc(p, 64);
    ck_assert_ptr_nonnull(q);
    for (int i = 0; i < 8; i++) {
        ck_assert_int_eq(q[i], i);
    }
    free(q);
}
END_TEST


/*****************************
*  callbacks tcase           *
*****************************/

static bool g_warning_called;
static bool g_panic_called;

static void warning_cb(void *arg) { (void)arg; g_warning_called = true; }
static void panic_cb(void *arg)   { (void)arg; g_panic_called = true; }

static void callback_setup(void)
{
    g_warning_called = false;
    g_panic_called = false;
    alloc_warning_callback_ = NULL;
    alloc_warning_callback_arg_ = NULL;
    alloc_panic_callback_ = NULL;
    alloc_panic_callback_arg_ = NULL;
}

static void callback_teardown(void)
{
    alloc_warning_callback_ = NULL;
    alloc_warning_callback_arg_ = NULL;
    alloc_panic_callback_ = NULL;
    alloc_panic_callback_arg_ = NULL;
}

START_TEST(test_register_warning_callback_sets_func)
{
    w_mem_register_alloc_warning_callback(warning_cb, NULL);
    ck_assert_ptr_eq(alloc_warning_callback_, warning_cb);
}
END_TEST

START_TEST(test_register_warning_callback_sets_arg)
{
    int arg = 42;
    w_mem_register_alloc_warning_callback(warning_cb, &arg);
    ck_assert_ptr_eq(alloc_warning_callback_arg_, &arg);
}
END_TEST

START_TEST(test_register_warning_callback_null_clears)
{
    w_mem_register_alloc_warning_callback(warning_cb, NULL);
    w_mem_register_alloc_warning_callback(NULL, NULL);
    ck_assert_ptr_null(alloc_warning_callback_);
}
END_TEST

START_TEST(test_register_panic_callback_sets_func)
{
    w_mem_register_alloc_panic_callback(panic_cb, NULL);
    ck_assert_ptr_eq(alloc_panic_callback_, panic_cb);
}
END_TEST

START_TEST(test_register_panic_callback_sets_arg)
{
    int arg = 42;
    w_mem_register_alloc_panic_callback(panic_cb, &arg);
    ck_assert_ptr_eq(alloc_panic_callback_arg_, &arg);
}
END_TEST

START_TEST(test_register_panic_callback_null_clears)
{
    w_mem_register_alloc_panic_callback(panic_cb, NULL);
    w_mem_register_alloc_panic_callback(NULL, NULL);
    ck_assert_ptr_null(alloc_panic_callback_);
}
END_TEST

START_TEST(test_panic_callback_invoked_on_alloc_failure)
{
    w_mem_register_alloc_panic_callback(panic_cb, NULL);
    w_mem_handle_alloc_failed_(1, NULL, __LINE__, __FILE__, alloc_panic_callback_, alloc_panic_callback_arg_);
    ck_assert(g_panic_called);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_memory_suite(void)
{
    Suite *s = suite_create("whisker_memory");

    TCase *tc_malloc = tcase_create("malloc");
    tcase_set_timeout(tc_malloc, 10);
    tcase_add_test(tc_malloc, test_malloc_returns_nonnull);
    tcase_add_test(tc_malloc, test_malloc_zero_returns_nonnull);
    tcase_add_test(tc_malloc, test_malloc_memory_is_writable);
    suite_add_tcase(s, tc_malloc);

    TCase *tc_calloc = tcase_create("calloc");
    tcase_set_timeout(tc_calloc, 10);
    tcase_add_test(tc_calloc, test_calloc_returns_nonnull);
    tcase_add_test(tc_calloc, test_calloc_zero_size_returns_nonnull);
    tcase_add_test(tc_calloc, test_calloc_zero_count_returns_nonnull);
    tcase_add_test(tc_calloc, test_calloc_memory_is_zeroed);
    suite_add_tcase(s, tc_calloc);

    TCase *tc_realloc = tcase_create("realloc");
    tcase_set_timeout(tc_realloc, 10);
    tcase_add_test(tc_realloc, test_realloc_returns_nonnull);
    tcase_add_test(tc_realloc, test_realloc_zero_size_returns_nonnull);
    tcase_add_test(tc_realloc, test_realloc_from_null);
    tcase_add_test(tc_realloc, test_realloc_preserves_data);
    suite_add_tcase(s, tc_realloc);

    TCase *tc_callbacks = tcase_create("callbacks");
    tcase_add_checked_fixture(tc_callbacks, callback_setup, callback_teardown);
    tcase_set_timeout(tc_callbacks, 10);
    tcase_add_test(tc_callbacks, test_register_warning_callback_sets_func);
    tcase_add_test(tc_callbacks, test_register_warning_callback_sets_arg);
    tcase_add_test(tc_callbacks, test_register_warning_callback_null_clears);
    tcase_add_test(tc_callbacks, test_register_panic_callback_sets_func);
    tcase_add_test(tc_callbacks, test_register_panic_callback_sets_arg);
    tcase_add_test(tc_callbacks, test_register_panic_callback_null_clears);
    tcase_add_test(tc_callbacks, test_panic_callback_invoked_on_alloc_failure);
    suite_add_tcase(s, tc_callbacks);

    return s;
}

int main(void)
{
    Suite *s = whisker_memory_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
