/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_fnv1a
 * @created     : Saturday Feb 28, 2026 13:39:21 CST
 * @description : tests for FNV-1a 64-bit hash function with verified test vectors
 */

#include "whisker_std.h"

#include "whisker_hash_fnv1a.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  empty and single byte     *
*****************************/

START_TEST(test_hash_empty_string)
{
	// FNV-1a of empty string returns the offset basis
	uint64_t hash = w_fnv1a_hash("", 0, 0);
	ck_assert_uint_eq(hash, 0xcbf29ce484222325ULL);
}
END_TEST

START_TEST(test_hash_single_byte_a)
{
	// official FNV-1a test vector
	uint64_t hash = w_fnv1a_hash("a", 1, 0);
	ck_assert_uint_eq(hash, 0xaf63dc4c8601ec8cULL);
}
END_TEST

START_TEST(test_hash_single_byte_null)
{
	// single null byte
	uint64_t hash = w_fnv1a_hash("\x00", 1, 0);
	ck_assert_uint_eq(hash, 0xaf63bd4c8601b7dfULL);
}
END_TEST

START_TEST(test_hash_single_byte_0xff)
{
	// single 0xff byte
	uint64_t hash = w_fnv1a_hash("\xff", 1, 0);
	ck_assert_uint_eq(hash, 0xaf64724c8602eb6eULL);
}
END_TEST


/*****************************
*  short strings (2-7 bytes) *
*****************************/

START_TEST(test_hash_2_bytes)
{
	// official FNV-1a test vector for "ab"
	uint64_t hash = w_fnv1a_hash("ab", 2, 0);
	ck_assert_uint_eq(hash, 0x089c4407b545986aULL);
}
END_TEST

START_TEST(test_hash_3_bytes)
{
	// official FNV-1a test vector for "abc"
	uint64_t hash = w_fnv1a_hash("abc", 3, 0);
	ck_assert_uint_eq(hash, 0xe71fa2190541574bULL);
}
END_TEST

START_TEST(test_hash_4_bytes)
{
	uint64_t hash = w_fnv1a_hash("test", 4, 0);
	ck_assert_uint_eq(hash, 0xf9e6e6ef197c2b25ULL);
}
END_TEST

START_TEST(test_hash_5_bytes)
{
	uint64_t hash = w_fnv1a_hash("Hello", 5, 0);
	ck_assert_uint_eq(hash, 0x63f0bfacf2c00f6bULL);
}
END_TEST

START_TEST(test_hash_7_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefg", 7, 0);
	ck_assert_uint_eq(hash, 0x406e475017aa7737ULL);
}
END_TEST


/*****************************
*  medium strings (8-31 bytes)
*****************************/

START_TEST(test_hash_8_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefgh", 8, 0);
	ck_assert_uint_eq(hash, 0x25da8c1836a8d66dULL);
}
END_TEST

START_TEST(test_hash_10_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefghij", 10, 0);
	ck_assert_uint_eq(hash, 0xb9bbc7aa22d79212ULL);
}
END_TEST

START_TEST(test_hash_12_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefghijkl", 12, 0);
	ck_assert_uint_eq(hash, 0x6c3aaed3e05a5cb5ULL);
}
END_TEST

START_TEST(test_hash_16_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefghijklmnop", 16, 0);
	ck_assert_uint_eq(hash, 0x7ef46f6c05086855ULL);
}
END_TEST

START_TEST(test_hash_24_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefghijklmnopqrstuvwx", 24, 0);
	ck_assert_uint_eq(hash, 0xcfc57122610fadddULL);
}
END_TEST

START_TEST(test_hash_31_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefghijklmnopqrstuvwxyz12345", 31, 0);
	ck_assert_uint_eq(hash, 0xdb4dd2cc5f59ab49ULL);
}
END_TEST


/*****************************
*  long strings (>32 bytes)  *
*****************************/

START_TEST(test_hash_32_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefghijklmnopqrstuvwxyz123456", 32, 0);
	ck_assert_uint_eq(hash, 0xfee8b046055e68cdULL);
}
END_TEST

START_TEST(test_hash_43_bytes_pangram)
{
	// classic pangram
	uint64_t hash = w_fnv1a_hash("The quick brown fox jumps over the lazy dog", 43, 0);
	ck_assert_uint_eq(hash, 0xf3f9b7f5e7e47110ULL);
}
END_TEST

START_TEST(test_hash_64_bytes)
{
	uint64_t hash = w_fnv1a_hash("abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ!@", 64, 0);
	ck_assert_uint_eq(hash, 0x3da6e25c960dbe75ULL);
}
END_TEST

START_TEST(test_hash_chongo_vector)
{
	// FNV author's test string "chongo was here!"
	uint64_t hash = w_fnv1a_hash("chongo was here!", 16, 0);
	ck_assert_uint_eq(hash, 0x858e2fa32a55e61dULL);
}
END_TEST


/*****************************
*  consistency tests         *
*****************************/

START_TEST(test_hash_same_input_same_output)
{
	const char *data = "test data for consistency";
	size_t len = strlen(data);
	uint64_t hash1 = w_fnv1a_hash(data, len, 0);
	uint64_t hash2 = w_fnv1a_hash(data, len, 0);
	ck_assert_uint_eq(hash1, hash2);
}
END_TEST

START_TEST(test_hash_different_data_different_output)
{
	uint64_t hash1 = w_fnv1a_hash("abc", 3, 0);
	uint64_t hash2 = w_fnv1a_hash("abd", 3, 0);
	ck_assert_uint_ne(hash1, hash2);
}
END_TEST

START_TEST(test_hash_binary_data)
{
	// test with binary data containing embedded nulls
	const unsigned char data[] = {0x00, 0x01, 0x02, 0x03, 0x00, 0x05, 0x06, 0x07};
	uint64_t hash1 = w_fnv1a_hash(data, sizeof(data), 0);
	uint64_t hash2 = w_fnv1a_hash(data, sizeof(data), 0);
	ck_assert_uint_eq(hash1, hash2);
}
END_TEST

START_TEST(test_hash_avalanche)
{
	// changing a single bit should produce a different hash
	const char *data1 = "test";
	const char *data2 = "uest";  // first byte differs by 1 bit
	uint64_t hash1 = w_fnv1a_hash(data1, 4, 0);
	uint64_t hash2 = w_fnv1a_hash(data2, 4, 0);
	ck_assert_uint_ne(hash1, hash2);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_fnv1a_suite(void)
{
	Suite *s = suite_create("whisker_fnv1a");

	TCase *tc_empty_single = tcase_create("empty_and_single_byte");
	tcase_set_timeout(tc_empty_single, 10);
	tcase_add_test(tc_empty_single, test_hash_empty_string);
	tcase_add_test(tc_empty_single, test_hash_single_byte_a);
	tcase_add_test(tc_empty_single, test_hash_single_byte_null);
	tcase_add_test(tc_empty_single, test_hash_single_byte_0xff);
	suite_add_tcase(s, tc_empty_single);

	TCase *tc_short = tcase_create("short_strings");
	tcase_set_timeout(tc_short, 10);
	tcase_add_test(tc_short, test_hash_2_bytes);
	tcase_add_test(tc_short, test_hash_3_bytes);
	tcase_add_test(tc_short, test_hash_4_bytes);
	tcase_add_test(tc_short, test_hash_5_bytes);
	tcase_add_test(tc_short, test_hash_7_bytes);
	suite_add_tcase(s, tc_short);

	TCase *tc_medium = tcase_create("medium_strings");
	tcase_set_timeout(tc_medium, 10);
	tcase_add_test(tc_medium, test_hash_8_bytes);
	tcase_add_test(tc_medium, test_hash_10_bytes);
	tcase_add_test(tc_medium, test_hash_12_bytes);
	tcase_add_test(tc_medium, test_hash_16_bytes);
	tcase_add_test(tc_medium, test_hash_24_bytes);
	tcase_add_test(tc_medium, test_hash_31_bytes);
	suite_add_tcase(s, tc_medium);

	TCase *tc_long = tcase_create("long_strings");
	tcase_set_timeout(tc_long, 10);
	tcase_add_test(tc_long, test_hash_32_bytes);
	tcase_add_test(tc_long, test_hash_43_bytes_pangram);
	tcase_add_test(tc_long, test_hash_64_bytes);
	tcase_add_test(tc_long, test_hash_chongo_vector);
	suite_add_tcase(s, tc_long);

	TCase *tc_consistency = tcase_create("consistency");
	tcase_set_timeout(tc_consistency, 10);
	tcase_add_test(tc_consistency, test_hash_same_input_same_output);
	tcase_add_test(tc_consistency, test_hash_different_data_different_output);
	tcase_add_test(tc_consistency, test_hash_binary_data);
	tcase_add_test(tc_consistency, test_hash_avalanche);
	suite_add_tcase(s, tc_consistency);

	return s;
}

int main(void)
{
	Suite *s = whisker_fnv1a_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
