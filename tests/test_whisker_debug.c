/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_debug
 * @created     : Thursday Feb 26, 2026 13:42:34 CST
 * @description : tests for whisker_debug.h assertion macros and debug functions
 */

#include "whisker_debug.h"

#include <check.h>
#include <unistd.h>

// stderr capture helpers using dup2+tmpfile pattern
static int saved_stderr;
static FILE *stderr_capture;

static void capture_stderr_start(void)
{
    fflush(stderr);
    saved_stderr = dup(STDERR_FILENO);
    stderr_capture = tmpfile();
    dup2(fileno(stderr_capture), STDERR_FILENO);
}

static char *capture_stderr_stop(void)
{
    fflush(stderr);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);

    fseek(stderr_capture, 0, SEEK_END);
    long len = ftell(stderr_capture);
    fseek(stderr_capture, 0, SEEK_SET);

    char *buf = malloc(len + 1);
    if (buf) {
        fread(buf, 1, len, stderr_capture);
        buf[len] = '\0';
    }
    fclose(stderr_capture);
    return buf;
}

// print_value_layout tcase
START_TEST(test_print_value_layout_outputs_to_stderr)
{
    int val = 0x12345678;
    capture_stderr_start();
    w_debug_print_value_layout(&val, sizeof(val), "test_val");
    char *output = capture_stderr_stop();
    ck_assert_ptr_nonnull(output);
    ck_assert_int_gt(strlen(output), 0);
    free(output);
}
END_TEST

START_TEST(test_print_value_layout_includes_name)
{
    int val = 42;
    capture_stderr_start();
    w_debug_print_value_layout(&val, sizeof(val), "my_variable");
    char *output = capture_stderr_stop();
    ck_assert_ptr_nonnull(output);
    ck_assert_ptr_nonnull(strstr(output, "my_variable"));
    free(output);
}
END_TEST

// assert success tcase - verify assertions pass without exiting
START_TEST(test_assert_int_eq_success)
{
    w_assert_int_eq(5, 5);
    w_assert_int_eq(-1, -1);
    w_assert_int_eq(0, 0);
}
END_TEST

START_TEST(test_assert_int_ne_success)
{
    w_assert_int_ne(5, 6);
    w_assert_int_ne(-1, 1);
    w_assert_int_ne(0, 1);
}
END_TEST

START_TEST(test_assert_int_lt_success)
{
    w_assert_int_lt(4, 5);
    w_assert_int_lt(-1, 0);
}
END_TEST

START_TEST(test_assert_int_gt_success)
{
    w_assert_int_gt(5, 4);
    w_assert_int_gt(0, -1);
}
END_TEST

START_TEST(test_assert_int_le_success)
{
    w_assert_int_le(4, 5);
    w_assert_int_le(5, 5);
}
END_TEST

START_TEST(test_assert_int_ge_success)
{
    w_assert_int_ge(5, 4);
    w_assert_int_ge(5, 5);
}
END_TEST

START_TEST(test_assert_uint_eq_success)
{
    w_assert_uint_eq(5u, 5u);
    w_assert_uint_eq(0u, 0u);
}
END_TEST

START_TEST(test_assert_long_eq_success)
{
    w_assert_long_eq(5L, 5L);
    w_assert_long_eq(-1L, -1L);
}
END_TEST

START_TEST(test_assert_ulong_eq_success)
{
    w_assert_ulong_eq(5UL, 5UL);
    w_assert_ulong_eq(0UL, 0UL);
}
END_TEST

START_TEST(test_assert_float_eq_success)
{
    w_assert_float_eq(1.5f, 1.5f);
    w_assert_float_eq(0.0f, 0.0f);
}
END_TEST

START_TEST(test_assert_double_eq_success)
{
    w_assert_double_eq(1.5, 1.5);
    w_assert_double_eq(0.0, 0.0);
}
END_TEST

START_TEST(test_assert_char_eq_success)
{
    w_assert_char_eq('a', 'a');
    w_assert_char_eq('\0', '\0');
}
END_TEST

START_TEST(test_assert_char_ne_success)
{
    w_assert_char_ne('a', 'b');
    w_assert_char_ne('x', 'y');
}
END_TEST

START_TEST(test_assert_str_eq_success)
{
    w_assert_str_eq("hello", "hello");
    w_assert_str_eq("", "");
}
END_TEST

START_TEST(test_assert_str_ne_success)
{
    w_assert_str_ne("hello", "world");
    w_assert_str_ne("a", "b");
}
END_TEST

START_TEST(test_assert_mem_eq_success)
{
    char a[] = {1, 2, 3, 4};
    char b[] = {1, 2, 3, 4};
    w_assert_mem_eq(a, b, 4);
}
END_TEST

START_TEST(test_assert_mem_ne_success)
{
    char a[] = {1, 2, 3, 4};
    char b[] = {1, 2, 3, 5};
    w_assert_mem_ne(a, b, 4);
}
END_TEST

START_TEST(test_assert_ptr_eq_success)
{
    int x;
    w_assert_ptr_eq(&x, &x);
    w_assert_ptr_eq(NULL, NULL);
}
END_TEST

START_TEST(test_assert_ptr_ne_success)
{
    int x, y;
    w_assert_ptr_ne(&x, &y);
    w_assert_ptr_ne(&x, NULL);
}
END_TEST

START_TEST(test_assert_generic_eq_success)
{
    w_assert_eq(1, 1);
    w_assert_eq(true, true);
}
END_TEST

START_TEST(test_assert_generic_ne_success)
{
    w_assert_ne(1, 2);
    w_assert_ne(true, false);
}
END_TEST

START_TEST(test_assert_success)
{
    w_assert(true);
    w_assert(1);
}
END_TEST

// assert failure tcase - verify assertions call exit(2)
START_TEST(test_assert_int_eq_failure)
{
    w_assert_int_eq(5, 6);
}
END_TEST

START_TEST(test_assert_int_ne_failure)
{
    w_assert_int_ne(5, 5);
}
END_TEST

START_TEST(test_assert_int_lt_failure)
{
    w_assert_int_lt(5, 5);
}
END_TEST

START_TEST(test_assert_int_gt_failure)
{
    w_assert_int_gt(5, 5);
}
END_TEST

START_TEST(test_assert_int_le_failure)
{
    w_assert_int_le(6, 5);
}
END_TEST

START_TEST(test_assert_int_ge_failure)
{
    w_assert_int_ge(4, 5);
}
END_TEST

START_TEST(test_assert_uint_eq_failure)
{
    w_assert_uint_eq(5u, 6u);
}
END_TEST

START_TEST(test_assert_long_eq_failure)
{
    w_assert_long_eq(5L, 6L);
}
END_TEST

START_TEST(test_assert_ulong_eq_failure)
{
    w_assert_ulong_eq(5UL, 6UL);
}
END_TEST

START_TEST(test_assert_float_eq_failure)
{
    w_assert_float_eq(1.5f, 2.5f);
}
END_TEST

START_TEST(test_assert_double_eq_failure)
{
    w_assert_double_eq(1.5, 2.5);
}
END_TEST

START_TEST(test_assert_char_eq_failure)
{
    w_assert_char_eq('a', 'b');
}
END_TEST

START_TEST(test_assert_str_eq_failure)
{
    w_assert_str_eq("hello", "world");
}
END_TEST

START_TEST(test_assert_mem_eq_failure)
{
    char a[] = {1, 2, 3, 4};
    char b[] = {1, 2, 3, 5};
    w_assert_mem_eq(a, b, 4);
}
END_TEST

START_TEST(test_assert_ptr_eq_failure)
{
    int x, y;
    w_assert_ptr_eq(&x, &y);
}
END_TEST

START_TEST(test_assert_ptr_ne_failure)
{
    int x;
    w_assert_ptr_ne(&x, &x);
}
END_TEST

START_TEST(test_assert_failure)
{
    w_assert(false);
}
END_TEST

// test suite
Suite *whisker_debug_suite(void)
{
    Suite *s = suite_create("whisker_debug");

    // print_value_layout tcase
    TCase *tc_print = tcase_create("print_value_layout");
    tcase_set_timeout(tc_print, 10);
    tcase_add_test(tc_print, test_print_value_layout_outputs_to_stderr);
    tcase_add_test(tc_print, test_print_value_layout_includes_name);
    suite_add_tcase(s, tc_print);

    // assert success tcase
    TCase *tc_success = tcase_create("assert_success");
    tcase_set_timeout(tc_success, 10);
    tcase_add_test(tc_success, test_assert_int_eq_success);
    tcase_add_test(tc_success, test_assert_int_ne_success);
    tcase_add_test(tc_success, test_assert_int_lt_success);
    tcase_add_test(tc_success, test_assert_int_gt_success);
    tcase_add_test(tc_success, test_assert_int_le_success);
    tcase_add_test(tc_success, test_assert_int_ge_success);
    tcase_add_test(tc_success, test_assert_uint_eq_success);
    tcase_add_test(tc_success, test_assert_long_eq_success);
    tcase_add_test(tc_success, test_assert_ulong_eq_success);
    tcase_add_test(tc_success, test_assert_float_eq_success);
    tcase_add_test(tc_success, test_assert_double_eq_success);
    tcase_add_test(tc_success, test_assert_char_eq_success);
    tcase_add_test(tc_success, test_assert_char_ne_success);
    tcase_add_test(tc_success, test_assert_str_eq_success);
    tcase_add_test(tc_success, test_assert_str_ne_success);
    tcase_add_test(tc_success, test_assert_mem_eq_success);
    tcase_add_test(tc_success, test_assert_mem_ne_success);
    tcase_add_test(tc_success, test_assert_ptr_eq_success);
    tcase_add_test(tc_success, test_assert_ptr_ne_success);
    tcase_add_test(tc_success, test_assert_generic_eq_success);
    tcase_add_test(tc_success, test_assert_generic_ne_success);
    tcase_add_test(tc_success, test_assert_success);
    suite_add_tcase(s, tc_success);

    // assert failure tcase - using exit tests
    TCase *tc_failure = tcase_create("assert_failure");
    tcase_set_timeout(tc_failure, 10);
    tcase_add_exit_test(tc_failure, test_assert_int_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_int_ne_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_int_lt_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_int_gt_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_int_le_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_int_ge_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_uint_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_long_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_ulong_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_float_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_double_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_char_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_str_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_mem_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_ptr_eq_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_ptr_ne_failure, 2);
    tcase_add_exit_test(tc_failure, test_assert_failure, 2);
    suite_add_tcase(s, tc_failure);

    return s;
}

// test runner
int main(void)
{
    Suite *s = whisker_debug_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
