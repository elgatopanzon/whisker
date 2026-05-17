/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_slab_arena
 * @created     : Saturday May 16, 2026 18:37:29 CST
 * @description : tests for whisker_slab_arena slab allocator
 */

#include "whisker_std.h"
#include "whisker_slab_arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_slab_arena g_slab;

static void slab_setup(void)
{
	w_slab_arena_init(&g_slab);
}

static void slab_teardown(void)
{
	w_slab_arena_free(&g_slab);
}


/*****************************
*  init / free               *
*****************************/

START_TEST(test_init_slabs_null)
{
	ck_assert_int_eq(g_slab.slabs_length, 0);
}
END_TEST

START_TEST(test_init_handle_pool_zero)
{
	ck_assert_int_eq(g_slab.handle_pool.next_id, 0);
}
END_TEST

START_TEST(test_free_cleans_up)
{
	struct w_slab_arena sa;
	w_slab_arena_init(&sa);
	size_t h = w_slab_arena_malloc(&sa, 64);
	(void)h;
	w_slab_arena_free(&sa);
	/* just verify it does not crash */
	ck_assert(1);
}
END_TEST


/*****************************
*  alloc / free              *
*****************************/

START_TEST(test_malloc_returns_valid_handle)
{
	size_t h = w_slab_arena_malloc(&g_slab, 64);
	ck_assert_int_ne(h, W_ID_POOL_INVALID);
}
END_TEST

START_TEST(test_malloc_ptr_nonnull)
{
	size_t h = w_slab_arena_malloc(&g_slab, 64);
	void *ptr = w_slab_arena_resolve_handle(&g_slab, h);
	ck_assert_ptr_nonnull(ptr);
}
END_TEST

START_TEST(test_malloc_data_writable)
{
	size_t h = w_slab_arena_malloc(&g_slab, 64);
	char *ptr = w_slab_arena_resolve_handle(&g_slab, h);
	memset(ptr, 0xAB, 64);
	ck_assert_int_eq((unsigned char)ptr[0], 0xAB);
	ck_assert_int_eq((unsigned char)ptr[63], 0xAB);
}
END_TEST

START_TEST(test_free_handle_clears_ptr)
{
	size_t h = w_slab_arena_malloc(&g_slab, 64);
	w_slab_arena_free_handle(&g_slab, h);
	void *ptr = w_slab_arena_resolve_safe_handle(&g_slab, h);
	ck_assert_ptr_null(ptr);
}
END_TEST

START_TEST(test_free_recycles_handle)
{
	size_t h1 = w_slab_arena_malloc(&g_slab, 64);
	w_slab_arena_free_handle(&g_slab, h1);
	size_t h2 = w_slab_arena_malloc(&g_slab, 64);
	ck_assert_int_eq(h1, h2);
}
END_TEST

START_TEST(test_free_recycles_slab_slot)
{
	size_t h1 = w_slab_arena_malloc(&g_slab, 64);
	void *ptr1 = w_slab_arena_resolve_handle(&g_slab, h1);
	w_slab_arena_free_handle(&g_slab, h1);
	size_t h2 = w_slab_arena_malloc(&g_slab, 64);
	void *ptr2 = w_slab_arena_resolve_handle(&g_slab, h2);
	/* same slab slot -> same pointer */
	ck_assert_ptr_eq(ptr1, ptr2);
}
END_TEST


/*****************************
*  realloc                   *
*****************************/

START_TEST(test_realloc_same_size_same_handle)
{
	size_t h1 = w_slab_arena_malloc(&g_slab, 64);
	size_t h2 = w_slab_arena_realloc(&g_slab, h1, 64);
	ck_assert_int_eq(h1, h2);
}
END_TEST

START_TEST(test_realloc_shrink_same_handle)
{
	size_t h1 = w_slab_arena_malloc(&g_slab, 128);
	size_t h2 = w_slab_arena_realloc(&g_slab, h1, 64);
	ck_assert_int_eq(h1, h2);
}
END_TEST

START_TEST(test_realloc_grow_data_preserved)
{
	size_t h = w_slab_arena_malloc(&g_slab, 16);
	char *ptr = w_slab_arena_resolve_handle(&g_slab, h);
	memset(ptr, 0x42, 16);

	h = w_slab_arena_realloc(&g_slab, h, 128);
	char *new_ptr = w_slab_arena_resolve_handle(&g_slab, h);

	for (int i = 0; i < 16; i++) {
		ck_assert_int_eq((unsigned char)new_ptr[i], 0x42);
	}
}
END_TEST

START_TEST(test_realloc_grow_new_ptr_nonnull)
{
	size_t h = w_slab_arena_malloc(&g_slab, 16);
	h = w_slab_arena_realloc(&g_slab, h, 512);
	void *ptr = w_slab_arena_resolve_handle(&g_slab, h);
	ck_assert_ptr_nonnull(ptr);
}
END_TEST


/*****************************
*  multiple size classes     *
*****************************/

START_TEST(test_multiple_size_classes_independent)
{
	size_t h8   = w_slab_arena_malloc(&g_slab, 8);
	size_t h64  = w_slab_arena_malloc(&g_slab, 64);
	size_t h512 = w_slab_arena_malloc(&g_slab, 512);

	char *p8   = w_slab_arena_resolve_handle(&g_slab, h8);
	char *p64  = w_slab_arena_resolve_handle(&g_slab, h64);
	char *p512 = w_slab_arena_resolve_handle(&g_slab, h512);

	ck_assert_ptr_nonnull(p8);
	ck_assert_ptr_nonnull(p64);
	ck_assert_ptr_nonnull(p512);
	ck_assert_ptr_ne(p8, p64);
	ck_assert_ptr_ne(p64, p512);
}
END_TEST

START_TEST(test_many_allocs_same_class)
{
	const int count = 64;
	size_t handles[64];

	for (int i = 0; i < count; i++) {
		handles[i] = w_slab_arena_malloc(&g_slab, 64);
		ck_assert_int_ne(handles[i], W_ID_POOL_INVALID);
		char *p = w_slab_arena_resolve_handle(&g_slab, handles[i]);
		memset(p, (unsigned char)i, 64);
	}

	/* verify data integrity */
	for (int i = 0; i < count; i++) {
		char *p = w_slab_arena_resolve_handle(&g_slab, handles[i]);
		ck_assert_int_eq((unsigned char)p[0], (unsigned char)i);
	}
}
END_TEST

START_TEST(test_alloc_free_alloc_different_classes)
{
	size_t ha = w_slab_arena_malloc(&g_slab, 32);
	size_t hb = w_slab_arena_malloc(&g_slab, 256);
	w_slab_arena_free_handle(&g_slab, ha);
	size_t hc = w_slab_arena_malloc(&g_slab, 32);
	ck_assert_int_eq(ha, hc);
	(void)hb;
}
END_TEST


/*****************************
*  stress tests              *
*****************************/

START_TEST(test_stress_alloc_free_same_size_reuses_slot)
{
	// alloc/free same size 1000x, verify same pointer every time
	const int iterations = 1000;
	size_t h = w_slab_arena_malloc(&g_slab, 64);
	void *first_ptr = w_slab_arena_resolve_handle(&g_slab, h);
	w_slab_arena_free_handle(&g_slab, h);

	for (int i = 0; i < iterations; i++) {
		h = w_slab_arena_malloc(&g_slab, 64);
		void *ptr = w_slab_arena_resolve_handle(&g_slab, h);
		ck_assert_ptr_eq(ptr, first_ptr);
		w_slab_arena_free_handle(&g_slab, h);
	}
}
END_TEST

START_TEST(test_stress_many_alloc_free_cycles)
{
	// hammer allocator with many cycles across multiple sizes
	const int cycles = 500;
	const size_t sizes[] = {8, 32, 64, 128, 256, 512, 1024};
	const int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

	for (int c = 0; c < cycles; c++) {
		for (int s = 0; s < num_sizes; s++) {
			size_t h = w_slab_arena_malloc(&g_slab, sizes[s]);
			ck_assert_int_ne(h, W_ID_POOL_INVALID);
			char *ptr = w_slab_arena_resolve_handle(&g_slab, h);
			ck_assert_ptr_nonnull(ptr);
			memset(ptr, 0xCC, sizes[s]);
			w_slab_arena_free_handle(&g_slab, h);
		}
	}
}
END_TEST

START_TEST(test_stress_interleaved_alloc_free)
{
	// allocate batch, free some, allocate more, verify reuse
	const int batch = 100;
	size_t handles[100];
	void *ptrs[100];

	// alloc all
	for (int i = 0; i < batch; i++) {
		handles[i] = w_slab_arena_malloc(&g_slab, 64);
		ptrs[i] = w_slab_arena_resolve_handle(&g_slab, handles[i]);
	}

	// free even indices
	for (int i = 0; i < batch; i += 2) {
		w_slab_arena_free_handle(&g_slab, handles[i]);
	}

	// re-alloc, should reuse freed slots
	for (int i = 0; i < batch; i += 2) {
		size_t h = w_slab_arena_malloc(&g_slab, 64);
		void *ptr = w_slab_arena_resolve_handle(&g_slab, h);
		// ptr should be one of the previously freed ptrs
		int found = 0;
		for (int j = 0; j < batch; j += 2) {
			if (ptr == ptrs[j]) {
				found = 1;
				break;
			}
		}
		ck_assert_int_eq(found, 1);
		handles[i] = h;
	}

	// free all
	for (int i = 0; i < batch; i++) {
		w_slab_arena_free_handle(&g_slab, handles[i]);
	}
}
END_TEST

START_TEST(test_stress_no_unbounded_growth)
{
	// alloc/free same size many times, verify slab count stays bounded
	const int iterations = 10000;

	for (int i = 0; i < iterations; i++) {
		size_t h = w_slab_arena_malloc(&g_slab, 256);
		w_slab_arena_free_handle(&g_slab, h);
	}

	// should only have 1 slab for size class 256
	// slabs_length should be minimal, not growing with iterations
	ck_assert_int_le(g_slab.slabs_length, 64);
}
END_TEST

START_TEST(test_stress_mixed_sizes_high_volume)
{
	// high volume with mixed sizes, verify no leaks via ASAN
	const int iterations = 2000;
	size_t handles[16];
	int active = 0;

	for (int i = 0; i < iterations; i++) {
		// randomly decide to alloc or free
		if (active < 16 && (active == 0 || (i % 3) != 0)) {
			size_t sz = 8 + (i % 512);
			handles[active] = w_slab_arena_malloc(&g_slab, sz);
			ck_assert_int_ne(handles[active], W_ID_POOL_INVALID);
			char *p = w_slab_arena_resolve_handle(&g_slab, handles[active]);
			memset(p, (unsigned char)i, sz);
			active++;
		} else if (active > 0) {
			active--;
			w_slab_arena_free_handle(&g_slab, handles[active]);
		}
	}

	// cleanup remaining
	while (active > 0) {
		active--;
		w_slab_arena_free_handle(&g_slab, handles[active]);
	}
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_slab_arena_suite(void)
{
	Suite *s = suite_create("whisker_slab_arena");

	TCase *tc_init = tcase_create("init_free");
	tcase_add_checked_fixture(tc_init, slab_setup, slab_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_slabs_null);
	tcase_add_test(tc_init, test_init_handle_pool_zero);
	tcase_add_test(tc_init, test_free_cleans_up);
	suite_add_tcase(s, tc_init);

	TCase *tc_alloc = tcase_create("alloc_free");
	tcase_add_checked_fixture(tc_alloc, slab_setup, slab_teardown);
	tcase_set_timeout(tc_alloc, 10);
	tcase_add_test(tc_alloc, test_malloc_returns_valid_handle);
	tcase_add_test(tc_alloc, test_malloc_ptr_nonnull);
	tcase_add_test(tc_alloc, test_malloc_data_writable);
	tcase_add_test(tc_alloc, test_free_handle_clears_ptr);
	tcase_add_test(tc_alloc, test_free_recycles_handle);
	tcase_add_test(tc_alloc, test_free_recycles_slab_slot);
	suite_add_tcase(s, tc_alloc);

	TCase *tc_realloc = tcase_create("realloc");
	tcase_add_checked_fixture(tc_realloc, slab_setup, slab_teardown);
	tcase_set_timeout(tc_realloc, 10);
	tcase_add_test(tc_realloc, test_realloc_same_size_same_handle);
	tcase_add_test(tc_realloc, test_realloc_shrink_same_handle);
	tcase_add_test(tc_realloc, test_realloc_grow_data_preserved);
	tcase_add_test(tc_realloc, test_realloc_grow_new_ptr_nonnull);
	suite_add_tcase(s, tc_realloc);

	TCase *tc_classes = tcase_create("size_classes");
	tcase_add_checked_fixture(tc_classes, slab_setup, slab_teardown);
	tcase_set_timeout(tc_classes, 10);
	tcase_add_test(tc_classes, test_multiple_size_classes_independent);
	tcase_add_test(tc_classes, test_many_allocs_same_class);
	tcase_add_test(tc_classes, test_alloc_free_alloc_different_classes);
	suite_add_tcase(s, tc_classes);

	TCase *tc_stress = tcase_create("stress");
	tcase_add_checked_fixture(tc_stress, slab_setup, slab_teardown);
	tcase_set_timeout(tc_stress, 30);
	tcase_add_test(tc_stress, test_stress_alloc_free_same_size_reuses_slot);
	tcase_add_test(tc_stress, test_stress_many_alloc_free_cycles);
	tcase_add_test(tc_stress, test_stress_interleaved_alloc_free);
	tcase_add_test(tc_stress, test_stress_no_unbounded_growth);
	tcase_add_test(tc_stress, test_stress_mixed_sizes_high_volume);
	suite_add_tcase(s, tc_stress);

	return s;
}

int main(void)
{
	Suite *s = whisker_slab_arena_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
