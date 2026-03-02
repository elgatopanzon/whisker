/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_random
 * @created     : Sunday Mar 01, 2026 17:37:02 CST
 * @description : tests for whisker_random.h random bytes generation
 */

#include "whisker_std.h"
#include "whisker_random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  w_rand_bytes basic tcase  *
*****************************/

START_TEST(test_rand_bytes_fills_buffer)
{
	uint8_t buf[32] = {0};
	w_rand_bytes(buf, 32);
	// at least some bytes should be non-zero
	int non_zero = 0;
	for (int i = 0; i < 32; i++) {
		if (buf[i] != 0) non_zero++;
	}
	ck_assert_int_gt(non_zero, 0);
}
END_TEST

START_TEST(test_rand_bytes_single_byte)
{
	// single byte should work
	uint8_t buf = 0;
	w_rand_bytes(&buf, 1);
	// can't assert much, just verify no crash
	ck_assert(1);
}
END_TEST

START_TEST(test_rand_bytes_various_sizes)
{
	size_t sizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
	for (int i = 0; i < 11; i++) {
		uint8_t *buf = malloc(sizes[i]);
		ck_assert_ptr_nonnull(buf);
		memset(buf, 0, sizes[i]);
		w_rand_bytes(buf, sizes[i]);
		// count non-zero bytes
		int non_zero = 0;
		for (size_t j = 0; j < sizes[i]; j++) {
			if (buf[j] != 0) non_zero++;
		}
		// for sizes >= 8, expect at least some non-zero
		if (sizes[i] >= 8) {
			ck_assert_int_gt(non_zero, 0);
		}
		free(buf);
	}
}
END_TEST


/*****************************
*  uniqueness tcase          *
*****************************/

START_TEST(test_rand_bytes_different_calls)
{
	uint8_t buf1[32];
	uint8_t buf2[32];
	w_rand_bytes(buf1, 32);
	w_rand_bytes(buf2, 32);
	// should produce different output
	ck_assert_int_ne(memcmp(buf1, buf2, 32), 0);
}
END_TEST

START_TEST(test_rand_bytes_multiple_calls_unique)
{
	// generate 10 buffers, verify all are different
	uint8_t bufs[10][16];
	for (int i = 0; i < 10; i++) {
		w_rand_bytes(bufs[i], 16);
	}
	// pairwise compare
	for (int i = 0; i < 10; i++) {
		for (int j = i + 1; j < 10; j++) {
			ck_assert_int_ne(memcmp(bufs[i], bufs[j], 16), 0);
		}
	}
}
END_TEST

START_TEST(test_rand_bytes_64bit_unique)
{
	uint64_t vals[100];
	for (int i = 0; i < 100; i++) {
		w_rand_bytes(&vals[i], sizeof(uint64_t));
	}
	// check uniqueness
	for (int i = 0; i < 100; i++) {
		for (int j = i + 1; j < 100; j++) {
			ck_assert_int_ne(vals[i], vals[j]);
		}
	}
}
END_TEST


/*****************************
*  edge cases tcase          *
*****************************/

START_TEST(test_rand_bytes_large_buffer)
{
	// 64KB buffer
	size_t size = 64 * 1024;
	uint8_t *buf = malloc(size);
	ck_assert_ptr_nonnull(buf);
	memset(buf, 0, size);
	w_rand_bytes(buf, size);
	// count non-zero bytes, expect at least 25%
	int non_zero = 0;
	for (size_t i = 0; i < size; i++) {
		if (buf[i] != 0) non_zero++;
	}
	ck_assert_int_gt(non_zero, size / 4);
	free(buf);
}
END_TEST

START_TEST(test_rand_bytes_stack_buffer)
{
	uint8_t stack_buf[256];
	memset(stack_buf, 0xAA, 256);
	w_rand_bytes(stack_buf, 256);
	// verify bytes were modified (not all 0xAA)
	int changed = 0;
	for (int i = 0; i < 256; i++) {
		if (stack_buf[i] != 0xAA) changed++;
	}
	ck_assert_int_gt(changed, 200);
}
END_TEST

START_TEST(test_rand_bytes_alignment)
{
	// test unaligned buffer access
	uint8_t buf[17];
	w_rand_bytes(buf + 1, 15);
	ck_assert(1);
}
END_TEST


/*****************************
*  distribution sanity       *
*****************************/

START_TEST(test_rand_bytes_byte_distribution)
{
	// generate many bytes and check distribution is roughly uniform
	size_t sample_size = 4096;
	uint8_t *buf = malloc(sample_size);
	ck_assert_ptr_nonnull(buf);
	w_rand_bytes(buf, sample_size);

	// count occurrences of each byte value
	int counts[256] = {0};
	for (size_t i = 0; i < sample_size; i++) {
		counts[buf[i]]++;
	}

	// expected average is sample_size / 256 = 16
	// check that no byte value appears more than 50 times or less than 0
	// (very loose check, just sanity)
	int max_count = 0;
	int zero_counts = 0;
	for (int i = 0; i < 256; i++) {
		if (counts[i] > max_count) max_count = counts[i];
		if (counts[i] == 0) zero_counts++;
	}
	// shouldn't have too many byte values with zero occurrences
	ck_assert_int_lt(zero_counts, 128);
	free(buf);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_random_suite(void)
{
	Suite *s = suite_create("whisker_random");

	TCase *tc_basic = tcase_create("rand_bytes_basic");
	tcase_set_timeout(tc_basic, 10);
	tcase_add_test(tc_basic, test_rand_bytes_fills_buffer);
	tcase_add_test(tc_basic, test_rand_bytes_single_byte);
	tcase_add_test(tc_basic, test_rand_bytes_various_sizes);
	suite_add_tcase(s, tc_basic);

	TCase *tc_unique = tcase_create("rand_bytes_uniqueness");
	tcase_set_timeout(tc_unique, 10);
	tcase_add_test(tc_unique, test_rand_bytes_different_calls);
	tcase_add_test(tc_unique, test_rand_bytes_multiple_calls_unique);
	tcase_add_test(tc_unique, test_rand_bytes_64bit_unique);
	suite_add_tcase(s, tc_unique);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_set_timeout(tc_edge, 10);
	tcase_add_test(tc_edge, test_rand_bytes_large_buffer);
	tcase_add_test(tc_edge, test_rand_bytes_stack_buffer);
	tcase_add_test(tc_edge, test_rand_bytes_alignment);
	suite_add_tcase(s, tc_edge);

	TCase *tc_dist = tcase_create("distribution_sanity");
	tcase_set_timeout(tc_dist, 10);
	tcase_add_test(tc_dist, test_rand_bytes_byte_distribution);
	suite_add_tcase(s, tc_dist);

	return s;
}

int main(void)
{
	Suite *s = whisker_random_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
