/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_memory
 * @created     : Thursday Feb 26, 2026 14:37:33 CST
 * @description : tests for whisker_memory.h allocation and memory block functions
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
*  block_create tcase        *
*****************************/

START_TEST(test_block_create_returns_nonnull)
{
    w_mem_block *b = w_mem_block_create(64, 16);
    ck_assert_ptr_nonnull(b);
    free(b);
}
END_TEST

START_TEST(test_block_create_stores_data_size)
{
    w_mem_block *b = w_mem_block_create(64, 16);
    ck_assert_int_eq(b->data_size, 64);
    free(b);
}
END_TEST

START_TEST(test_block_create_stores_header_size)
{
    w_mem_block *b = w_mem_block_create(64, 16);
    ck_assert_int_eq(b->header_size, 16);
    free(b);
}
END_TEST

START_TEST(test_block_create_header_data_null_before_init)
{
    w_mem_block *b = w_mem_block_create(64, 16);
    ck_assert_ptr_null(b->header);
    ck_assert_ptr_null(b->data);
    free(b);
}
END_TEST


/*****************************
*  block_init tcase          *
*****************************/

static w_mem_block *g_block;

static void block_setup(void)
{
    g_block = w_mem_block_create_and_init(64, 16);
}

static void block_teardown(void)
{
    w_mem_block_free_all(g_block);
    g_block = NULL;
}

START_TEST(test_block_init_sets_header)
{
    ck_assert_ptr_nonnull(g_block->header);
}
END_TEST

START_TEST(test_block_init_sets_data)
{
    ck_assert_ptr_nonnull(g_block->data);
}
END_TEST

START_TEST(test_block_init_data_offset_correct)
{
    char *expected = (char *)g_block->header + g_block->header_size;
    ck_assert_ptr_eq(g_block->data, expected);
}
END_TEST

START_TEST(test_block_init_memory_zeroed)
{
    unsigned char *p = g_block->header;
    size_t total = g_block->header_size + g_block->data_size;
    for (size_t i = 0; i < total; i++) {
        ck_assert_int_eq(p[i], 0);
    }
}
END_TEST


/*****************************
*  block_realloc tcase       *
*****************************/

static w_mem_block *g_realloc_block;

static void block_realloc_setup(void)
{
    g_realloc_block = w_mem_block_create_and_init(32, 8);
    // write a known pattern to header so we can verify it survives realloc
    memset(g_realloc_block->header, 0xAB, 8);
}

static void block_realloc_teardown(void)
{
    w_mem_block_free_all(g_realloc_block);
    g_realloc_block = NULL;
}

START_TEST(test_block_realloc_grows_data_size)
{
    w_mem_block_realloc(g_realloc_block, 128);
    ck_assert_int_eq(g_realloc_block->data_size, 128);
}
END_TEST

START_TEST(test_block_realloc_new_bytes_zeroed)
{
    w_mem_block_realloc(g_realloc_block, 128);
    unsigned char *data = g_realloc_block->data;
    for (int i = 32; i < 128; i++) {
        ck_assert_int_eq(data[i], 0);
    }
}
END_TEST

START_TEST(test_block_realloc_header_preserved)
{
    w_mem_block_realloc(g_realloc_block, 128);
    unsigned char *hdr = g_realloc_block->header;
    for (int i = 0; i < 8; i++) {
        ck_assert_int_eq(hdr[i], (unsigned char)0xAB);
    }
}
END_TEST

START_TEST(test_block_realloc_data_ptr_valid)
{
    w_mem_block_realloc(g_realloc_block, 128);
    ck_assert_ptr_nonnull(g_realloc_block->data);
    char *expected = (char *)g_realloc_block->header + g_realloc_block->header_size;
    ck_assert_ptr_eq(g_realloc_block->data, expected);
}
END_TEST


/*****************************
*  block_free tcase          *
*****************************/

START_TEST(test_block_free_no_crash)
{
    w_mem_block *b = w_mem_block_create_and_init(32, 8);
    w_mem_block_free(b);
    free(b);
}
END_TEST

START_TEST(test_block_free_all_no_crash)
{
    w_mem_block *b = w_mem_block_create_and_init(32, 8);
    w_mem_block_free_all(b);
}
END_TEST


/*****************************
*  block_utils tcase         *
*****************************/

START_TEST(test_header_from_data_pointer_correct)
{
    char buf[32];
    size_t header_size = 8;
    char *data = buf + header_size;
    void *header = w_mem_block_header_from_data_pointer(data, header_size);
    ck_assert_ptr_eq(header, buf);
}
END_TEST

START_TEST(test_calc_header_size_header_smaller_than_data)
{
    // header_type_size <= data_type_size: returns data_type_size
    ck_assert_int_eq(w_mem_block_calc_header_size(4, 8), 8);
}
END_TEST

START_TEST(test_calc_header_size_equal)
{
    ck_assert_int_eq(w_mem_block_calc_header_size(4, 4), 4);
}
END_TEST

START_TEST(test_calc_header_size_header_larger_than_data)
{
    // header > data: (header/data)*data
    ck_assert_int_eq(w_mem_block_calc_header_size(8, 4), 8);
}
END_TEST

START_TEST(test_calc_header_size_header_exact_multiple)
{
    ck_assert_int_eq(w_mem_block_calc_header_size(12, 4), 12);
}
END_TEST

START_TEST(test_calc_header_size_one_byte)
{
    ck_assert_int_eq(w_mem_block_calc_header_size(1, 1), 1);
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

    TCase *tc_block_create = tcase_create("block_create");
    tcase_set_timeout(tc_block_create, 10);
    tcase_add_test(tc_block_create, test_block_create_returns_nonnull);
    tcase_add_test(tc_block_create, test_block_create_stores_data_size);
    tcase_add_test(tc_block_create, test_block_create_stores_header_size);
    tcase_add_test(tc_block_create, test_block_create_header_data_null_before_init);
    suite_add_tcase(s, tc_block_create);

    TCase *tc_block_init = tcase_create("block_init");
    tcase_add_checked_fixture(tc_block_init, block_setup, block_teardown);
    tcase_set_timeout(tc_block_init, 10);
    tcase_add_test(tc_block_init, test_block_init_sets_header);
    tcase_add_test(tc_block_init, test_block_init_sets_data);
    tcase_add_test(tc_block_init, test_block_init_data_offset_correct);
    tcase_add_test(tc_block_init, test_block_init_memory_zeroed);
    suite_add_tcase(s, tc_block_init);

    TCase *tc_block_realloc = tcase_create("block_realloc");
    tcase_add_checked_fixture(tc_block_realloc, block_realloc_setup, block_realloc_teardown);
    tcase_set_timeout(tc_block_realloc, 10);
    tcase_add_test(tc_block_realloc, test_block_realloc_grows_data_size);
    tcase_add_test(tc_block_realloc, test_block_realloc_new_bytes_zeroed);
    tcase_add_test(tc_block_realloc, test_block_realloc_header_preserved);
    tcase_add_test(tc_block_realloc, test_block_realloc_data_ptr_valid);
    suite_add_tcase(s, tc_block_realloc);

    TCase *tc_block_free = tcase_create("block_free");
    tcase_set_timeout(tc_block_free, 10);
    tcase_add_test(tc_block_free, test_block_free_no_crash);
    tcase_add_test(tc_block_free, test_block_free_all_no_crash);
    suite_add_tcase(s, tc_block_free);

    TCase *tc_block_utils = tcase_create("block_utils");
    tcase_set_timeout(tc_block_utils, 10);
    tcase_add_test(tc_block_utils, test_header_from_data_pointer_correct);
    tcase_add_test(tc_block_utils, test_calc_header_size_header_smaller_than_data);
    tcase_add_test(tc_block_utils, test_calc_header_size_equal);
    tcase_add_test(tc_block_utils, test_calc_header_size_header_larger_than_data);
    tcase_add_test(tc_block_utils, test_calc_header_size_header_exact_multiple);
    tcase_add_test(tc_block_utils, test_calc_header_size_one_byte);
    suite_add_tcase(s, tc_block_utils);

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
