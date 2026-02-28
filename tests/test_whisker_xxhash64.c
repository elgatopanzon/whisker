/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_xxhash64
 * @created     : Saturday Feb 28, 2026 13:08:56 CST
 * @description : tests for xxHash64 hash function with verified test vectors
 */

#include "whisker_std.h"

#include "whisker_hash_xxhash64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  empty and single byte     *
*****************************/

START_TEST(test_hash_empty_string_seed_0)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("", 0, 0);
	ck_assert_uint_eq(hash, 0xef46db3751d8e999ULL);
}
END_TEST

START_TEST(test_hash_single_byte_a)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("a", 1, 0);
	ck_assert_uint_eq(hash, 0xd24ec4f1a98c6e5bULL);
}
END_TEST

START_TEST(test_hash_single_byte_null)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("\x00", 1, 0);
	ck_assert_uint_eq(hash, 0xe934a84adb052768ULL);
}
END_TEST

START_TEST(test_hash_single_byte_0xff)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("\xff", 1, 0);
	ck_assert_uint_eq(hash, 0x95634172a60b7544ULL);
}
END_TEST


/*****************************
*  short strings (2-7 bytes) *
*****************************/

START_TEST(test_hash_2_bytes)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("ab", 2, 0);
	ck_assert_uint_eq(hash, 0x65f708ca92d04a61ULL);
}
END_TEST

START_TEST(test_hash_3_bytes)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abc", 3, 0);
	ck_assert_uint_eq(hash, 0x44bc2cf5ad770999ULL);
}
END_TEST

START_TEST(test_hash_4_bytes)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("test", 4, 0);
	ck_assert_uint_eq(hash, 0x4fdcca5ddb678139ULL);
}
END_TEST

START_TEST(test_hash_5_bytes)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("Hello", 5, 0);
	ck_assert_uint_eq(hash, 0x0a75a91375b27d44ULL);
}
END_TEST

START_TEST(test_hash_7_bytes)
{
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefg", 7, 0);
	ck_assert_uint_eq(hash, 0x1860940e2902822dULL);
}
END_TEST


/*****************************
*  medium strings (8-31 bytes)
*****************************/

START_TEST(test_hash_8_bytes)
{
	// exactly 8 bytes - one full 8-byte chunk
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefgh", 8, 0);
	ck_assert_uint_eq(hash, 0x3ad351775b4634b7ULL);
}
END_TEST

START_TEST(test_hash_10_bytes)
{
	// 10 bytes - one 8-byte chunk + 2 remaining bytes
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefghij", 10, 0);
	ck_assert_uint_eq(hash, 0xd6287a1de5498bb2ULL);
}
END_TEST

START_TEST(test_hash_12_bytes)
{
	// 12 bytes - one 8-byte chunk + one 4-byte chunk
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefghijkl", 12, 0);
	ck_assert_uint_eq(hash, 0x4b09b7d3a233d4b3ULL);
}
END_TEST

START_TEST(test_hash_16_bytes)
{
	// 16 bytes - two 8-byte chunks
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefghijklmnop", 16, 0);
	ck_assert_uint_eq(hash, 0x71ce8137ca2dd53dULL);
}
END_TEST

START_TEST(test_hash_24_bytes)
{
	// 24 bytes - three 8-byte chunks
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefghijklmnopqrstuvwx", 24, 0);
	ck_assert_uint_eq(hash, 0x0bec95e34669983bULL);
}
END_TEST

START_TEST(test_hash_31_bytes)
{
	// 31 bytes - just under the 32-byte stripe threshold
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefghijklmnopqrstuvwxyz12345", 31, 0);
	ck_assert_uint_eq(hash, 0x467605901b01d6f6ULL);
}
END_TEST


/*****************************
*  32-byte stripe boundary   *
*****************************/

START_TEST(test_hash_32_bytes)
{
	// exactly 32 bytes - one full stripe, exercises the 4-accumulator path
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefghijklmnopqrstuvwxyz123456", 32, 0);
	ck_assert_uint_eq(hash, 0x0022ee3b5a18531bULL);
}
END_TEST

START_TEST(test_hash_33_bytes)
{
	// 33 bytes - one stripe + 1 remaining byte
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefghijklmnopqrstuvwxyz1234567", 33, 0);
	ck_assert_uint_eq(hash, 0x23bbd16d29353c5fULL);
}
END_TEST


/*****************************
*  long strings (>32 bytes)  *
*****************************/

START_TEST(test_hash_43_bytes_pangram)
{
	// classic pangram - exercises stripe path with remainder
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("The quick brown fox jumps over the lazy dog", 43, 0);
	ck_assert_uint_eq(hash, 0x0b242d361fda71bcULL);
}
END_TEST

START_TEST(test_hash_64_bytes)
{
	// 64 bytes - exactly two stripes
	// verified against xxhsum -H64
	uint64_t hash = w_xxhash64_hash("abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ!@", 64, 0);
	ck_assert_uint_eq(hash, 0x71238680860515a7ULL);
}
END_TEST


/*****************************
*  non-zero seeds            *
*****************************/

START_TEST(test_hash_seed_1_empty)
{
	uint64_t hash = w_xxhash64_hash("", 0, 1);
	ck_assert_uint_eq(hash, 0xd5afba1336a3be4bULL);
}
END_TEST

START_TEST(test_hash_seed_1_single_byte)
{
	uint64_t hash = w_xxhash64_hash("a", 1, 1);
	ck_assert_uint_eq(hash, 0xdec2bc81c3cd46c6ULL);
}
END_TEST

START_TEST(test_hash_seed_1_short)
{
	uint64_t hash = w_xxhash64_hash("test", 4, 1);
	ck_assert_uint_eq(hash, 0x99ebbf9ba48f4c5dULL);
}
END_TEST

START_TEST(test_hash_seed_42_short)
{
	uint64_t hash = w_xxhash64_hash("Hello", 5, 42);
	ck_assert_uint_eq(hash, 0x5e0ed0f2695a805aULL);
}
END_TEST

START_TEST(test_hash_seed_large)
{
	// large seed value
	uint64_t hash = w_xxhash64_hash("abcdefgh", 8, 0x123456789ABCDEF0ULL);
	ck_assert_uint_eq(hash, 0x9968ac4e662aea77ULL);
}
END_TEST

START_TEST(test_hash_seed_max)
{
	// max uint64 seed
	uint64_t hash = w_xxhash64_hash("test", 4, 0xFFFFFFFFFFFFFFFFULL);
	ck_assert_uint_eq(hash, 0x8ae3e1fd39b1c88aULL);
}
END_TEST

START_TEST(test_hash_seed_42_long)
{
	// non-zero seed with long string (exercises stripe path with seed)
	uint64_t hash = w_xxhash64_hash("abcdefghijklmnopqrstuvwxyz1234567", 33, 42);
	ck_assert_uint_eq(hash, 0x07a16228d57d97f5ULL);
}
END_TEST


/*****************************
*  consistency tests         *
*****************************/

START_TEST(test_hash_same_input_same_output)
{
	const char *data = "test data for consistency";
	size_t len = strlen(data);
	uint64_t hash1 = w_xxhash64_hash(data, len, 0);
	uint64_t hash2 = w_xxhash64_hash(data, len, 0);
	ck_assert_uint_eq(hash1, hash2);
}
END_TEST

START_TEST(test_hash_different_seed_different_output)
{
	const char *data = "test";
	uint64_t hash0 = w_xxhash64_hash(data, 4, 0);
	uint64_t hash1 = w_xxhash64_hash(data, 4, 1);
	ck_assert_uint_ne(hash0, hash1);
}
END_TEST

START_TEST(test_hash_different_data_different_output)
{
	uint64_t hash1 = w_xxhash64_hash("abc", 3, 0);
	uint64_t hash2 = w_xxhash64_hash("abd", 3, 0);
	ck_assert_uint_ne(hash1, hash2);
}
END_TEST

START_TEST(test_hash_binary_data)
{
	// test with binary data containing embedded nulls
	const unsigned char data[] = {0x00, 0x01, 0x02, 0x03, 0x00, 0x05, 0x06, 0x07};
	uint64_t hash1 = w_xxhash64_hash(data, sizeof(data), 0);
	uint64_t hash2 = w_xxhash64_hash(data, sizeof(data), 0);
	ck_assert_uint_eq(hash1, hash2);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_xxhash64_suite(void)
{
	Suite *s = suite_create("whisker_xxhash64");

	TCase *tc_empty_single = tcase_create("empty_and_single_byte");
	tcase_set_timeout(tc_empty_single, 10);
	tcase_add_test(tc_empty_single, test_hash_empty_string_seed_0);
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

	TCase *tc_stripe = tcase_create("stripe_boundary");
	tcase_set_timeout(tc_stripe, 10);
	tcase_add_test(tc_stripe, test_hash_32_bytes);
	tcase_add_test(tc_stripe, test_hash_33_bytes);
	suite_add_tcase(s, tc_stripe);

	TCase *tc_long = tcase_create("long_strings");
	tcase_set_timeout(tc_long, 10);
	tcase_add_test(tc_long, test_hash_43_bytes_pangram);
	tcase_add_test(tc_long, test_hash_64_bytes);
	suite_add_tcase(s, tc_long);

	TCase *tc_seeds = tcase_create("non_zero_seeds");
	tcase_set_timeout(tc_seeds, 10);
	tcase_add_test(tc_seeds, test_hash_seed_1_empty);
	tcase_add_test(tc_seeds, test_hash_seed_1_single_byte);
	tcase_add_test(tc_seeds, test_hash_seed_1_short);
	tcase_add_test(tc_seeds, test_hash_seed_42_short);
	tcase_add_test(tc_seeds, test_hash_seed_large);
	tcase_add_test(tc_seeds, test_hash_seed_max);
	tcase_add_test(tc_seeds, test_hash_seed_42_long);
	suite_add_tcase(s, tc_seeds);

	TCase *tc_consistency = tcase_create("consistency");
	tcase_set_timeout(tc_consistency, 10);
	tcase_add_test(tc_consistency, test_hash_same_input_same_output);
	tcase_add_test(tc_consistency, test_hash_different_seed_different_output);
	tcase_add_test(tc_consistency, test_hash_different_data_different_output);
	tcase_add_test(tc_consistency, test_hash_binary_data);
	suite_add_tcase(s, tc_consistency);

	return s;
}

int main(void)
{
	Suite *s = whisker_xxhash64_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
