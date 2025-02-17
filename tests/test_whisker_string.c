/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_string
 * @created     : Thursday Feb 06, 2025 10:49:43 CST
 */

#include "whisker_string.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

START_TEST(test_whisker_string_creation_and_header)
{
	char* wstring;
	char* str = "convert to whisker string";
	size_t str_len = strlen(str);
	whisker_str(str, &wstring);
	whisker_array_header* header = whisker_str_header(wstring);

	// verify length matches
	// NOTE: underlying array header is string length + 1
	ck_assert_int_eq(str_len, header->length - 1);

	// verify string matches
	ck_assert_str_eq(str, wstring);

	whisker_str_free(wstring);
}
END_TEST

START_TEST(test_whisker_string_length)
{
	char* wstring;
	char* str = "convert to whisker string";
	size_t str_len = strlen(str);
	whisker_str(str, &wstring);

	// verify length matches
	// NOTE: whisker_str_length() is actual string length
	ck_assert_int_eq(str_len, whisker_str_length(wstring));
	ck_assert_int_eq(25, whisker_str_length(wstring));

	whisker_str_free(wstring);
}
END_TEST

/* START_TEST(test_whisker_string_length_non_whisker_string) */
/* { */
/* 	char* str = "convert to whisker string"; */
/* 	size_t str_len = strlen(str); */
/* 	size_t length_from_bad_header = whisker_str_length(str); */
/*  */
/* 	// NOTE: a bad header wouldn't be equal */
/* 	ck_assert_int_ne(length_from_bad_header, str_len); */
/* } */
/* END_TEST */

START_TEST(test_whisker_string_join)
{
	char* str1 = "join";
	char* str2 = "this";
	char* str3 = "up";
	char* str_joined = "join this up";
	size_t join_len = 12;

	char* joined;
	whisker_str_join(" ", &joined, str1, str2, str3, NULL);

	// verify string is joined
	ck_assert_str_eq(str_joined, joined);

	// verify length matches
	ck_assert_int_eq(join_len, whisker_str_length(joined));

	whisker_str_free(joined);
}
END_TEST

START_TEST(test_whisker_string_copy)
{
	char* str;
	whisker_str("copy this!", &str);

	// copy the string
	char* copy;
	whisker_str_copy(str, &copy);

	// verify string is opied
	ck_assert_str_eq(copy, str);

	// verify length matches
	ck_assert_int_eq(strlen(str), whisker_str_length(copy));

	whisker_str_free(str);
	whisker_str_free(copy);
}
END_TEST

START_TEST(test_whisker_string_contains)
{
	char* str_haystack;
	whisker_str("a big long string", &str_haystack);

	ck_assert_int_eq(6, whisker_str_contains(str_haystack, "long"));
	ck_assert_int_eq(0, whisker_str_contains(str_haystack, "a big"));
	ck_assert_int_eq(-1, whisker_str_contains(str_haystack, "short"));

	whisker_str_free(str_haystack);
}
END_TEST

Suite* whisker_string_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_string");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_string_creation_and_header);
	tcase_add_test(tc_core, test_whisker_string_length);
	/* tcase_add_test_raise_signal(tc_core, test_whisker_string_length_non_whisker_string, 11); */
	tcase_add_test(tc_core, test_whisker_string_join);
	tcase_add_test(tc_core, test_whisker_string_copy);
	tcase_add_test(tc_core, test_whisker_string_contains);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_string_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
