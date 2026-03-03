/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_sparse_bitset
 * @created     : Monday Mar 02, 2026 16:12:08 CST
 * @description : tests for sparse bitset data structure
 */

#include "whisker_std.h"

#include "whisker_sparse_bitset.h"
#include "whisker_random.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

// test fixtures
static struct w_sparse_bitset g_bitset;
static struct w_arena g_arena;

static void sparse_bitset_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_sparse_bitset_init(&g_bitset, &g_arena, W_SPARSE_BITSET_PAGE_SIZE_WORDS);
}

static void sparse_bitset_teardown(void)
{
	w_sparse_bitset_free(&g_bitset);
	w_arena_free(&g_arena);
}


/*****************************
*  init/free tcase           *
*****************************/

START_TEST(test_init_pages_length_zero)
{
	ck_assert_int_eq(g_bitset.pages_length, 0);
}
END_TEST

START_TEST(test_init_lookup_pages_length_zero)
{
	ck_assert_int_eq(g_bitset.lookup_pages_length, 0);
}
END_TEST

START_TEST(test_init_page_size_stored)
{
	ck_assert_int_eq(g_bitset.page_size_, W_SPARSE_BITSET_PAGE_SIZE_WORDS);
}
END_TEST

START_TEST(test_init_arena_stored)
{
	ck_assert_ptr_eq(g_bitset.arena, &g_arena);
}
END_TEST

START_TEST(test_free_clears_pages)
{
	w_sparse_bitset_set(&g_bitset, 0);
	w_sparse_bitset_free(&g_bitset);
	ck_assert_ptr_null(g_bitset.pages);
	ck_assert_int_eq(g_bitset.pages_length, 0);
}
END_TEST

START_TEST(test_free_clears_lookup_pages)
{
	w_sparse_bitset_set(&g_bitset, 0);
	w_sparse_bitset_free(&g_bitset);
	ck_assert_ptr_null(g_bitset.lookup_pages);
	ck_assert_int_eq(g_bitset.lookup_pages_length, 0);
}
END_TEST

START_TEST(test_free_clears_arena)
{
	w_sparse_bitset_set(&g_bitset, 0);
	w_sparse_bitset_free(&g_bitset);
	ck_assert_ptr_null(g_bitset.arena);
}
END_TEST


/*****************************
*  set single bit tcase      *
*****************************/

START_TEST(test_set_bit_zero)
{
	w_sparse_bitset_set(&g_bitset, 0);
	ck_assert(w_sparse_bitset_get(&g_bitset, 0));
}
END_TEST

START_TEST(test_set_bit_one)
{
	w_sparse_bitset_set(&g_bitset, 1);
	ck_assert(w_sparse_bitset_get(&g_bitset, 1));
}
END_TEST

START_TEST(test_set_bit_63)
{
	w_sparse_bitset_set(&g_bitset, 63);
	ck_assert(w_sparse_bitset_get(&g_bitset, 63));
}
END_TEST

START_TEST(test_set_bit_64)
{
	w_sparse_bitset_set(&g_bitset, 64);
	ck_assert(w_sparse_bitset_get(&g_bitset, 64));
}
END_TEST

START_TEST(test_set_bit_allocates_page)
{
	w_sparse_bitset_set(&g_bitset, 0);
	ck_assert_ptr_nonnull(g_bitset.pages);
	ck_assert_int_eq(g_bitset.pages_length, 1);
	ck_assert_ptr_nonnull(g_bitset.pages[0].bits);
}
END_TEST

START_TEST(test_set_bit_allocates_lookup_page)
{
	w_sparse_bitset_set(&g_bitset, 0);
	ck_assert_ptr_nonnull(g_bitset.lookup_pages);
	ck_assert_int_eq(g_bitset.lookup_pages_length, 1);
}
END_TEST


/*****************************
*  get bit tcase             *
*****************************/

START_TEST(test_get_unset_bit_returns_false)
{
	ck_assert(!w_sparse_bitset_get(&g_bitset, 0));
}
END_TEST

START_TEST(test_get_out_of_range_returns_false)
{
	ck_assert(!w_sparse_bitset_get(&g_bitset, 1000000));
}
END_TEST

START_TEST(test_get_set_bit_returns_true)
{
	w_sparse_bitset_set(&g_bitset, 42);
	ck_assert(w_sparse_bitset_get(&g_bitset, 42));
}
END_TEST

START_TEST(test_get_adjacent_unset_returns_false)
{
	w_sparse_bitset_set(&g_bitset, 42);
	ck_assert(!w_sparse_bitset_get(&g_bitset, 41));
	ck_assert(!w_sparse_bitset_get(&g_bitset, 43));
}
END_TEST


/*****************************
*  clear bit tcase           *
*****************************/

START_TEST(test_clear_unset_bit_no_crash)
{
	w_sparse_bitset_clear(&g_bitset, 0);
	ck_assert(!w_sparse_bitset_get(&g_bitset, 0));
}
END_TEST

START_TEST(test_clear_out_of_range_no_crash)
{
	w_sparse_bitset_clear(&g_bitset, 1000000);
	// should not crash
}
END_TEST

START_TEST(test_clear_set_bit)
{
	w_sparse_bitset_set(&g_bitset, 42);
	ck_assert(w_sparse_bitset_get(&g_bitset, 42));
	w_sparse_bitset_clear(&g_bitset, 42);
	ck_assert(!w_sparse_bitset_get(&g_bitset, 42));
}
END_TEST

START_TEST(test_clear_doesnt_affect_others)
{
	w_sparse_bitset_set(&g_bitset, 41);
	w_sparse_bitset_set(&g_bitset, 42);
	w_sparse_bitset_set(&g_bitset, 43);
	w_sparse_bitset_clear(&g_bitset, 42);
	ck_assert(w_sparse_bitset_get(&g_bitset, 41));
	ck_assert(!w_sparse_bitset_get(&g_bitset, 42));
	ck_assert(w_sparse_bitset_get(&g_bitset, 43));
}
END_TEST


/*****************************
*  multiple bits tcase       *
*****************************/

START_TEST(test_set_multiple_bits_same_word)
{
	w_sparse_bitset_set(&g_bitset, 0);
	w_sparse_bitset_set(&g_bitset, 1);
	w_sparse_bitset_set(&g_bitset, 63);
	ck_assert(w_sparse_bitset_get(&g_bitset, 0));
	ck_assert(w_sparse_bitset_get(&g_bitset, 1));
	ck_assert(w_sparse_bitset_get(&g_bitset, 63));
}
END_TEST

START_TEST(test_set_multiple_bits_different_words)
{
	w_sparse_bitset_set(&g_bitset, 0);
	w_sparse_bitset_set(&g_bitset, 64);
	w_sparse_bitset_set(&g_bitset, 128);
	ck_assert(w_sparse_bitset_get(&g_bitset, 0));
	ck_assert(w_sparse_bitset_get(&g_bitset, 64));
	ck_assert(w_sparse_bitset_get(&g_bitset, 128));
}
END_TEST

START_TEST(test_set_100_sequential_bits)
{
	for (int i = 0; i < 100; i++) {
		w_sparse_bitset_set(&g_bitset, i);
	}
	for (int i = 0; i < 100; i++) {
		ck_assert(w_sparse_bitset_get(&g_bitset, i));
	}
	ck_assert(!w_sparse_bitset_get(&g_bitset, 100));
}
END_TEST

START_TEST(test_set_sparse_bits)
{
	w_sparse_bitset_set(&g_bitset, 10);
	w_sparse_bitset_set(&g_bitset, 1000);
	w_sparse_bitset_set(&g_bitset, 100000);
	ck_assert(w_sparse_bitset_get(&g_bitset, 10));
	ck_assert(w_sparse_bitset_get(&g_bitset, 1000));
	ck_assert(w_sparse_bitset_get(&g_bitset, 100000));
	ck_assert(!w_sparse_bitset_get(&g_bitset, 11));
	ck_assert(!w_sparse_bitset_get(&g_bitset, 1001));
}
END_TEST


/*****************************
*  page boundaries tcase     *
*****************************/

START_TEST(test_page_boundary_last_bit_of_page)
{
	// last bit of first page: page_size * 64 - 1
	uint64_t last_bit = (W_SPARSE_BITSET_PAGE_SIZE_WORDS * 64) - 1;
	w_sparse_bitset_set(&g_bitset, last_bit);
	ck_assert(w_sparse_bitset_get(&g_bitset, last_bit));
	ck_assert_int_eq(g_bitset.pages_length, 1);
}
END_TEST

START_TEST(test_page_boundary_first_bit_of_second_page)
{
	// first bit of second page: page_size * 64
	uint64_t first_bit = W_SPARSE_BITSET_PAGE_SIZE_WORDS * 64;
	w_sparse_bitset_set(&g_bitset, first_bit);
	ck_assert(w_sparse_bitset_get(&g_bitset, first_bit));
	ck_assert_int_eq(g_bitset.pages_length, 2);
}
END_TEST

START_TEST(test_page_boundary_adjacent_bits_span_pages)
{
	uint64_t last_of_first = (W_SPARSE_BITSET_PAGE_SIZE_WORDS * 64) - 1;
	uint64_t first_of_second = W_SPARSE_BITSET_PAGE_SIZE_WORDS * 64;
	w_sparse_bitset_set(&g_bitset, last_of_first);
	w_sparse_bitset_set(&g_bitset, first_of_second);
	ck_assert(w_sparse_bitset_get(&g_bitset, last_of_first));
	ck_assert(w_sparse_bitset_get(&g_bitset, first_of_second));
	ck_assert_int_eq(g_bitset.pages_length, 2);
}
END_TEST

START_TEST(test_page_sparse_allocation)
{
	// set bit in page 0 and page 10
	w_sparse_bitset_set(&g_bitset, 0);
	w_sparse_bitset_set(&g_bitset, 10 * W_SPARSE_BITSET_PAGE_SIZE_WORDS * 64);
	ck_assert_int_eq(g_bitset.pages_length, 11);
	// page 0 should have bits
	ck_assert_ptr_nonnull(g_bitset.pages[0].bits);
	// intermediate pages should be NULL
	ck_assert_ptr_null(g_bitset.pages[5].bits);
	// page 10 should have bits
	ck_assert_ptr_nonnull(g_bitset.pages[10].bits);
}
END_TEST


/*****************************
*  lookup page updates tcase *
*****************************/

START_TEST(test_lookup_page_set_on_first_set)
{
	w_sparse_bitset_set(&g_bitset, 0);
	ck_assert(g_bitset.lookup_pages[0] & 1);
}
END_TEST

START_TEST(test_lookup_page_clear_on_empty_page)
{
	w_sparse_bitset_set(&g_bitset, 0);
	ck_assert(g_bitset.lookup_pages[0] & 1);
	w_sparse_bitset_clear(&g_bitset, 0);
	ck_assert(!(g_bitset.lookup_pages[0] & 1));
}
END_TEST

START_TEST(test_lookup_page_stays_set_with_remaining_bits)
{
	w_sparse_bitset_set(&g_bitset, 0);
	w_sparse_bitset_set(&g_bitset, 1);
	w_sparse_bitset_clear(&g_bitset, 0);
	ck_assert(g_bitset.lookup_pages[0] & 1);
}
END_TEST

START_TEST(test_lookup_page_multiple_pages)
{
	// set bits in page 0 and page 1
	w_sparse_bitset_set(&g_bitset, 0);
	w_sparse_bitset_set(&g_bitset, W_SPARSE_BITSET_PAGE_SIZE_WORDS * 64);
	// both page bits should be set in lookup
	ck_assert(g_bitset.lookup_pages[0] & 1);
	ck_assert(g_bitset.lookup_pages[0] & 2);
}
END_TEST


/*****************************
*  first_set/last_set tcase  *
*****************************/

START_TEST(test_first_set_initialized_max)
{
	w_sparse_bitset_set(&g_bitset, 0);
	// after setting bit 0, first_set should be 0
	ck_assert_int_eq(g_bitset.pages[0].first_set, 0);
}
END_TEST

START_TEST(test_last_set_initialized_zero)
{
	w_sparse_bitset_set(&g_bitset, 0);
	// after setting bit 0, last_set should be 0
	ck_assert_int_eq(g_bitset.pages[0].last_set, 0);
}
END_TEST

START_TEST(test_first_set_tracks_minimum_word)
{
	// set bit in word 10 first, then word 5
	w_sparse_bitset_set(&g_bitset, 10 * 64);
	ck_assert_int_eq(g_bitset.pages[0].first_set, 10);
	w_sparse_bitset_set(&g_bitset, 5 * 64);
	ck_assert_int_eq(g_bitset.pages[0].first_set, 5);
}
END_TEST

START_TEST(test_last_set_tracks_maximum_word)
{
	// set bit in word 5 first, then word 10
	w_sparse_bitset_set(&g_bitset, 5 * 64);
	ck_assert_int_eq(g_bitset.pages[0].last_set, 5);
	w_sparse_bitset_set(&g_bitset, 10 * 64);
	ck_assert_int_eq(g_bitset.pages[0].last_set, 10);
}
END_TEST

START_TEST(test_first_last_set_same_word)
{
	w_sparse_bitset_set(&g_bitset, 3 * 64);
	w_sparse_bitset_set(&g_bitset, 3 * 64 + 1);
	ck_assert_int_eq(g_bitset.pages[0].first_set, 3);
	ck_assert_int_eq(g_bitset.pages[0].last_set, 3);
}
END_TEST

START_TEST(test_first_last_reset_on_page_clear)
{
	w_sparse_bitset_set(&g_bitset, 5 * 64);
	w_sparse_bitset_clear(&g_bitset, 5 * 64);
	ck_assert_int_eq(g_bitset.pages[0].first_set, UINT32_MAX);
	ck_assert_int_eq(g_bitset.pages[0].last_set, 0);
}
END_TEST


/*****************************
*  stress test tcase         *
*****************************/

START_TEST(test_stress_random_bits)
{
	uint64_t bits[1000];

	// set 1000 random bits
	for (int i = 0; i < 1000; i++) {
		uint64_t random_bytes;
		w_rand_bytes(&random_bytes, sizeof(random_bytes));
		bits[i] = random_bytes % 1000000;
		w_sparse_bitset_set(&g_bitset, bits[i]);
	}

	// verify all set bits
	for (int i = 0; i < 1000; i++) {
		ck_assert(w_sparse_bitset_get(&g_bitset, bits[i]));
	}
}
END_TEST

START_TEST(test_stress_set_clear_cycle)
{
	// set 100 bits
	for (int i = 0; i < 100; i++) {
		w_sparse_bitset_set(&g_bitset, i * 1000);
	}
	// clear them
	for (int i = 0; i < 100; i++) {
		w_sparse_bitset_clear(&g_bitset, i * 1000);
	}
	// verify all cleared
	for (int i = 0; i < 100; i++) {
		ck_assert(!w_sparse_bitset_get(&g_bitset, i * 1000));
	}
}
END_TEST

START_TEST(test_stress_large_index)
{
	// set a very large bit index
	uint64_t large_index = 1ULL << 30;
	w_sparse_bitset_set(&g_bitset, large_index);
	ck_assert(w_sparse_bitset_get(&g_bitset, large_index));
	ck_assert(!w_sparse_bitset_get(&g_bitset, large_index - 1));
	ck_assert(!w_sparse_bitset_get(&g_bitset, large_index + 1));
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite* whisker_sparse_bitset_suite(void)
{
	Suite *s = suite_create("whisker_sparse_bitset");

	TCase *tc_init = tcase_create("init_free");
	tcase_add_checked_fixture(tc_init, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_pages_length_zero);
	tcase_add_test(tc_init, test_init_lookup_pages_length_zero);
	tcase_add_test(tc_init, test_init_page_size_stored);
	tcase_add_test(tc_init, test_init_arena_stored);
	tcase_add_test(tc_init, test_free_clears_pages);
	tcase_add_test(tc_init, test_free_clears_lookup_pages);
	tcase_add_test(tc_init, test_free_clears_arena);
	suite_add_tcase(s, tc_init);

	TCase *tc_set = tcase_create("set_single_bit");
	tcase_add_checked_fixture(tc_set, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_set, 10);
	tcase_add_test(tc_set, test_set_bit_zero);
	tcase_add_test(tc_set, test_set_bit_one);
	tcase_add_test(tc_set, test_set_bit_63);
	tcase_add_test(tc_set, test_set_bit_64);
	tcase_add_test(tc_set, test_set_bit_allocates_page);
	tcase_add_test(tc_set, test_set_bit_allocates_lookup_page);
	suite_add_tcase(s, tc_set);

	TCase *tc_get = tcase_create("get_bit");
	tcase_add_checked_fixture(tc_get, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_get, 10);
	tcase_add_test(tc_get, test_get_unset_bit_returns_false);
	tcase_add_test(tc_get, test_get_out_of_range_returns_false);
	tcase_add_test(tc_get, test_get_set_bit_returns_true);
	tcase_add_test(tc_get, test_get_adjacent_unset_returns_false);
	suite_add_tcase(s, tc_get);

	TCase *tc_clear = tcase_create("clear_bit");
	tcase_add_checked_fixture(tc_clear, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_clear, 10);
	tcase_add_test(tc_clear, test_clear_unset_bit_no_crash);
	tcase_add_test(tc_clear, test_clear_out_of_range_no_crash);
	tcase_add_test(tc_clear, test_clear_set_bit);
	tcase_add_test(tc_clear, test_clear_doesnt_affect_others);
	suite_add_tcase(s, tc_clear);

	TCase *tc_multiple = tcase_create("multiple_bits");
	tcase_add_checked_fixture(tc_multiple, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_multiple, 10);
	tcase_add_test(tc_multiple, test_set_multiple_bits_same_word);
	tcase_add_test(tc_multiple, test_set_multiple_bits_different_words);
	tcase_add_test(tc_multiple, test_set_100_sequential_bits);
	tcase_add_test(tc_multiple, test_set_sparse_bits);
	suite_add_tcase(s, tc_multiple);

	TCase *tc_page = tcase_create("page_boundaries");
	tcase_add_checked_fixture(tc_page, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_page, 10);
	tcase_add_test(tc_page, test_page_boundary_last_bit_of_page);
	tcase_add_test(tc_page, test_page_boundary_first_bit_of_second_page);
	tcase_add_test(tc_page, test_page_boundary_adjacent_bits_span_pages);
	tcase_add_test(tc_page, test_page_sparse_allocation);
	suite_add_tcase(s, tc_page);

	TCase *tc_lookup = tcase_create("lookup_page_updates");
	tcase_add_checked_fixture(tc_lookup, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_lookup, 10);
	tcase_add_test(tc_lookup, test_lookup_page_set_on_first_set);
	tcase_add_test(tc_lookup, test_lookup_page_clear_on_empty_page);
	tcase_add_test(tc_lookup, test_lookup_page_stays_set_with_remaining_bits);
	tcase_add_test(tc_lookup, test_lookup_page_multiple_pages);
	suite_add_tcase(s, tc_lookup);

	TCase *tc_first_last = tcase_create("first_last_set_tracking");
	tcase_add_checked_fixture(tc_first_last, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_first_last, 10);
	tcase_add_test(tc_first_last, test_first_set_initialized_max);
	tcase_add_test(tc_first_last, test_last_set_initialized_zero);
	tcase_add_test(tc_first_last, test_first_set_tracks_minimum_word);
	tcase_add_test(tc_first_last, test_last_set_tracks_maximum_word);
	tcase_add_test(tc_first_last, test_first_last_set_same_word);
	tcase_add_test(tc_first_last, test_first_last_reset_on_page_clear);
	suite_add_tcase(s, tc_first_last);

	TCase *tc_stress = tcase_create("stress");
	tcase_add_checked_fixture(tc_stress, sparse_bitset_setup, sparse_bitset_teardown);
	tcase_set_timeout(tc_stress, 30);
	tcase_add_test(tc_stress, test_stress_random_bits);
	tcase_add_test(tc_stress, test_stress_set_clear_cycle);
	tcase_add_test(tc_stress, test_stress_large_index);
	suite_add_tcase(s, tc_stress);

	return s;
}

// test runner
int main()
{
	Suite *s = whisker_sparse_bitset_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
