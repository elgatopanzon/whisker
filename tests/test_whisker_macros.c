/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_macros
 * @created     : Thursday Feb 26, 2026 14:24:00 CST
 * @description : tests for whisker_macros.h string macros
 */

#include <string.h>

#include <check.h>
#include "whisker_macros.h"

// helper macros for expansion tests
#define TEST_FOO hello
#define TEST_BAR world
#define TEST_NUM 42

// STR tcase - stringifies without expanding macros
START_TEST(test_str_plain_identifier)
{
    const char *result = STR(hello);
    ck_assert_int_eq(strcmp(result, "hello"), 0);
}
END_TEST

START_TEST(test_str_number_literal)
{
    const char *result = STR(42);
    ck_assert_int_eq(strcmp(result, "42"), 0);
}
END_TEST

START_TEST(test_str_does_not_expand_macro)
{
    // STR does not expand TEST_FOO -- result is the macro name as a string
    const char *result = STR(TEST_FOO);
    ck_assert_int_eq(strcmp(result, "TEST_FOO"), 0);
}
END_TEST

// MACRO_STR tcase - expands macros before stringifying
START_TEST(test_macro_str_plain_identifier)
{
    const char *result = MACRO_STR(hello);
    ck_assert_int_eq(strcmp(result, "hello"), 0);
}
END_TEST

START_TEST(test_macro_str_expands_macro)
{
    // MACRO_STR expands TEST_FOO to hello before stringifying
    const char *result = MACRO_STR(TEST_FOO);
    ck_assert_int_eq(strcmp(result, "hello"), 0);
}
END_TEST

START_TEST(test_macro_str_expands_numeric_macro)
{
    const char *result = MACRO_STR(TEST_NUM);
    ck_assert_int_eq(strcmp(result, "42"), 0);
}
END_TEST

// JOIN_STR_NEXT tcase - concatenates and stringifies without expanding
START_TEST(test_join_str_next_two_identifiers)
{
    const char *result = JOIN_STR_NEXT(foo, bar);
    ck_assert_int_eq(strcmp(result, "foobar"), 0);
}
END_TEST

START_TEST(test_join_str_next_does_not_expand_macros)
{
    // JOIN_STR_NEXT pastes tokens before expansion -- macros are not expanded
    const char *result = JOIN_STR_NEXT(TEST_FOO, TEST_BAR);
    ck_assert_int_eq(strcmp(result, "TEST_FOOTEST_BAR"), 0);
}
END_TEST

// JOIN_STR tcase - expands macros before concatenating and stringifying
START_TEST(test_join_str_two_identifiers)
{
    const char *result = JOIN_STR(foo, bar);
    ck_assert_int_eq(strcmp(result, "foobar"), 0);
}
END_TEST

START_TEST(test_join_str_expands_macros)
{
    // JOIN_STR expands TEST_FOO -> hello and TEST_BAR -> world before joining
    const char *result = JOIN_STR(TEST_FOO, TEST_BAR);
    ck_assert_int_eq(strcmp(result, "helloworld"), 0);
}
END_TEST

START_TEST(test_join_str_expands_first_macro_only)
{
    const char *result = JOIN_STR(TEST_FOO, baz);
    ck_assert_int_eq(strcmp(result, "hellobaz"), 0);
}
END_TEST

// test suite
Suite *whisker_macros_suite(void)
{
    Suite *s = suite_create("whisker_macros");

    // STR tcase
    TCase *tc_str = tcase_create("STR");
    tcase_set_timeout(tc_str, 10);
    tcase_add_test(tc_str, test_str_plain_identifier);
    tcase_add_test(tc_str, test_str_number_literal);
    tcase_add_test(tc_str, test_str_does_not_expand_macro);
    suite_add_tcase(s, tc_str);

    // MACRO_STR tcase
    TCase *tc_macro_str = tcase_create("MACRO_STR");
    tcase_set_timeout(tc_macro_str, 10);
    tcase_add_test(tc_macro_str, test_macro_str_plain_identifier);
    tcase_add_test(tc_macro_str, test_macro_str_expands_macro);
    tcase_add_test(tc_macro_str, test_macro_str_expands_numeric_macro);
    suite_add_tcase(s, tc_macro_str);

    // JOIN_STR_NEXT tcase
    TCase *tc_join_next = tcase_create("JOIN_STR_NEXT");
    tcase_set_timeout(tc_join_next, 10);
    tcase_add_test(tc_join_next, test_join_str_next_two_identifiers);
    tcase_add_test(tc_join_next, test_join_str_next_does_not_expand_macros);
    suite_add_tcase(s, tc_join_next);

    // JOIN_STR tcase
    TCase *tc_join = tcase_create("JOIN_STR");
    tcase_set_timeout(tc_join, 10);
    tcase_add_test(tc_join, test_join_str_two_identifiers);
    tcase_add_test(tc_join, test_join_str_expands_macros);
    tcase_add_test(tc_join, test_join_str_expands_first_macro_only);
    suite_add_tcase(s, tc_join);

    return s;
}

// test runner
int main(void)
{
    Suite *s = whisker_macros_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
