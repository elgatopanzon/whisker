/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_arena
 * @created     : Friday Feb 27, 2026 20:23:17 CST
 * @description : tests for whisker_arena.h block-based arena allocator
 */

#include "whisker_std.h"
#include "whisker_arena.h"
#include "whisker_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>

// helper to get aligned data start
#define ALIGNED_DATA(b) ((unsigned char *)ALIGN_UP((uintptr_t)(b)->data, alignof(max_align_t)))


/*****************************
*  fixture (64-byte blocks)  *
*****************************/

static struct w_arena g_arena;

static void arena_setup(void)
{
	w_arena_init(&g_arena, 64);
}

static void arena_teardown(void)
{
	w_arena_free(&g_arena);
}


/*****************************
*  arena_init tcase          *
*****************************/

START_TEST(test_init_first_block_allocated)
{
	ck_assert_ptr_nonnull(g_arena.first);
}
END_TEST

START_TEST(test_init_current_equals_first)
{
	ck_assert_ptr_eq(g_arena.current, g_arena.first);
}
END_TEST

START_TEST(test_init_block_size_stored)
{
	ck_assert_int_eq(g_arena.block_size, 64);
}
END_TEST

START_TEST(test_init_first_block_ptr_at_aligned_data)
{
	ck_assert_ptr_eq(g_arena.first->ptr, ALIGNED_DATA(g_arena.first));
}
END_TEST

START_TEST(test_init_first_block_next_null)
{
	ck_assert_ptr_null(g_arena.first->next);
}
END_TEST


/*****************************
*  arena_malloc tcase        *
*****************************/

START_TEST(test_malloc_returns_nonnull)
{
	void *p = w_arena_malloc(&g_arena, 16);
	ck_assert_ptr_nonnull(p);
}
END_TEST

START_TEST(test_malloc_pointer_in_data_region)
{
	void *p = w_arena_malloc(&g_arena, 16);
	ck_assert_ptr_eq(p, ALIGNED_DATA(g_arena.first));
}
END_TEST

START_TEST(test_malloc_updates_ptr)
{
	w_arena_malloc(&g_arena, 16);
	ck_assert_int_eq(g_arena.first->ptr - ALIGNED_DATA(g_arena.first), 16);
}
END_TEST

START_TEST(test_malloc_multiple_fit_in_block)
{
	w_arena_malloc(&g_arena, 16);
	w_arena_malloc(&g_arena, 16);
	w_arena_malloc(&g_arena, 16);
	// 48 bytes used, still in first block
	ck_assert_ptr_eq(g_arena.current, g_arena.first);
	ck_assert_int_eq(g_arena.first->ptr - ALIGNED_DATA(g_arena.first), 48);
}
END_TEST

START_TEST(test_malloc_triggers_new_block)
{
	w_arena_malloc(&g_arena, 32);
	w_arena_malloc(&g_arena, 48);
	// second alloc exceeds remaining 32 bytes, triggers new block
	ck_assert_ptr_ne(g_arena.current, g_arena.first);
}
END_TEST

START_TEST(test_malloc_new_block_becomes_current)
{
	w_arena_malloc(&g_arena, 32);
	w_arena_malloc(&g_arena, 48);
	// 48 doesn't fit in remaining 32 bytes, new block created
	ck_assert_ptr_nonnull(g_arena.current);
	ck_assert_ptr_ne(g_arena.current, g_arena.first);
	ck_assert_int_eq(g_arena.current->ptr - ALIGNED_DATA(g_arena.current), 48);
}
END_TEST

START_TEST(test_malloc_links_blocks)
{
	w_arena_malloc(&g_arena, 64);
	w_arena_malloc(&g_arena, 8);
	// second alloc triggers new block, first->next should point to it
	ck_assert_ptr_eq(g_arena.first->next, g_arena.current);
}
END_TEST

START_TEST(test_malloc_exact_boundary)
{
	w_arena_malloc(&g_arena, 64);
	// exactly fills block
	ck_assert_int_eq(g_arena.first->ptr - ALIGNED_DATA(g_arena.first), 64);
	ck_assert_ptr_eq(g_arena.current, g_arena.first);
}
END_TEST

START_TEST(test_malloc_exceeds_remaining)
{
	w_arena_malloc(&g_arena, 60);
	w_arena_malloc(&g_arena, 8);
	// 8 bytes doesn't fit in remaining 4 bytes
	ck_assert_ptr_ne(g_arena.current, g_arena.first);
	ck_assert_int_eq(g_arena.current->ptr - ALIGNED_DATA(g_arena.current), 8);
}
END_TEST

START_TEST(test_malloc_memory_writable)
{
	unsigned char *p = w_arena_malloc(&g_arena, 32);
	for (int i = 0; i < 32; i++) {
		p[i] = (unsigned char)i;
	}
	ck_assert_int_eq(p[31], 31);
}
END_TEST

START_TEST(test_malloc_sequential_fills_many_blocks)
{
	// allocate 10 blocks worth of data
	for (int i = 0; i < 10; i++) {
		w_arena_malloc(&g_arena, 64);
	}
	// count blocks
	int count = 0;
	struct w_arena_block *b = g_arena.first;
	while (b) {
		count++;
		b = b->next;
	}
	ck_assert_int_eq(count, 10);
}
END_TEST


/*****************************
*  arena_calloc tcase        *
*****************************/

START_TEST(test_calloc_returns_nonnull)
{
	void *p = w_arena_calloc(&g_arena, 16);
	ck_assert_ptr_nonnull(p);
}
END_TEST

START_TEST(test_calloc_memory_zeroed)
{
	unsigned char *p = w_arena_calloc(&g_arena, 32);
	for (int i = 0; i < 32; i++) {
		ck_assert_int_eq(p[i], 0);
	}
}
END_TEST

START_TEST(test_calloc_updates_ptr)
{
	w_arena_calloc(&g_arena, 24);
	ck_assert_int_eq(g_arena.first->ptr - ALIGNED_DATA(g_arena.first), 24);
}
END_TEST

START_TEST(test_calloc_new_block_zeroed)
{
	w_arena_calloc(&g_arena, 64);
	// trigger new block
	unsigned char *p = w_arena_calloc(&g_arena, 32);
	for (int i = 0; i < 32; i++) {
		ck_assert_int_eq(p[i], 0);
	}
}
END_TEST


/*****************************
*  arena_clear tcase         *
*****************************/

START_TEST(test_clear_resets_first_ptr)
{
	w_arena_malloc(&g_arena, 32);
	w_arena_clear(&g_arena);
	ck_assert_ptr_eq(g_arena.first->ptr, ALIGNED_DATA(g_arena.first));
}
END_TEST

START_TEST(test_clear_current_equals_first)
{
	w_arena_malloc(&g_arena, 64);
	w_arena_malloc(&g_arena, 64);
	w_arena_clear(&g_arena);
	ck_assert_ptr_eq(g_arena.current, g_arena.first);
}
END_TEST

START_TEST(test_clear_first_next_null)
{
	w_arena_malloc(&g_arena, 64);
	w_arena_malloc(&g_arena, 64);
	w_arena_clear(&g_arena);
	ck_assert_ptr_null(g_arena.first->next);
}
END_TEST

START_TEST(test_clear_single_block_works)
{
	w_arena_malloc(&g_arena, 16);
	w_arena_clear(&g_arena);
	ck_assert_ptr_eq(g_arena.first->ptr, ALIGNED_DATA(g_arena.first));
	ck_assert_ptr_eq(g_arena.current, g_arena.first);
}
END_TEST

START_TEST(test_clear_multi_block_works)
{
	// fill 5 blocks
	for (int i = 0; i < 5; i++) {
		w_arena_malloc(&g_arena, 64);
	}
	w_arena_clear(&g_arena);
	ck_assert_ptr_eq(g_arena.current, g_arena.first);
	ck_assert_ptr_null(g_arena.first->next);
	ck_assert_ptr_eq(g_arena.first->ptr, ALIGNED_DATA(g_arena.first));
}
END_TEST

START_TEST(test_clear_allows_reuse)
{
	w_arena_malloc(&g_arena, 32);
	w_arena_clear(&g_arena);
	void *p = w_arena_malloc(&g_arena, 16);
	// should reuse first block's data region (at aligned position)
	ck_assert_ptr_eq(p, ALIGNED_DATA(g_arena.first));
	ck_assert_int_eq(g_arena.first->ptr - ALIGNED_DATA(g_arena.first), 16);
}
END_TEST

START_TEST(test_clear_double_clear_safe)
{
	w_arena_malloc(&g_arena, 32);
	w_arena_clear(&g_arena);
	w_arena_clear(&g_arena);
	ck_assert_ptr_eq(g_arena.current, g_arena.first);
	ck_assert_ptr_eq(g_arena.first->ptr, ALIGNED_DATA(g_arena.first));
}
END_TEST


/*****************************
*  arena_free tcase          *
*  (no fixture - self-managed)
*****************************/

START_TEST(test_free_single_block_no_crash)
{
	struct w_arena a;
	w_arena_init(&a, 64);
	w_arena_malloc(&a, 16);
	w_arena_free(&a);
	// no crash, first should be null after free
	ck_assert_ptr_null(a.first);
}
END_TEST

START_TEST(test_free_multi_block_no_crash)
{
	struct w_arena a;
	w_arena_init(&a, 64);
	for (int i = 0; i < 5; i++) {
		w_arena_malloc(&a, 64);
	}
	w_arena_free(&a);
	// no crash, first should be null after free
	ck_assert_ptr_null(a.first);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_arena_suite(void)
{
	Suite *s = suite_create("whisker_arena");

	TCase *tc_init = tcase_create("arena_init");
	tcase_add_checked_fixture(tc_init, arena_setup, arena_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_first_block_allocated);
	tcase_add_test(tc_init, test_init_current_equals_first);
	tcase_add_test(tc_init, test_init_block_size_stored);
	tcase_add_test(tc_init, test_init_first_block_ptr_at_aligned_data);
	tcase_add_test(tc_init, test_init_first_block_next_null);
	suite_add_tcase(s, tc_init);

	TCase *tc_malloc = tcase_create("arena_malloc");
	tcase_add_checked_fixture(tc_malloc, arena_setup, arena_teardown);
	tcase_set_timeout(tc_malloc, 10);
	tcase_add_test(tc_malloc, test_malloc_returns_nonnull);
	tcase_add_test(tc_malloc, test_malloc_pointer_in_data_region);
	tcase_add_test(tc_malloc, test_malloc_updates_ptr);
	tcase_add_test(tc_malloc, test_malloc_multiple_fit_in_block);
	tcase_add_test(tc_malloc, test_malloc_triggers_new_block);
	tcase_add_test(tc_malloc, test_malloc_new_block_becomes_current);
	tcase_add_test(tc_malloc, test_malloc_links_blocks);
	tcase_add_test(tc_malloc, test_malloc_exact_boundary);
	tcase_add_test(tc_malloc, test_malloc_exceeds_remaining);
	tcase_add_test(tc_malloc, test_malloc_memory_writable);
	tcase_add_test(tc_malloc, test_malloc_sequential_fills_many_blocks);
	suite_add_tcase(s, tc_malloc);

	TCase *tc_calloc = tcase_create("arena_calloc");
	tcase_add_checked_fixture(tc_calloc, arena_setup, arena_teardown);
	tcase_set_timeout(tc_calloc, 10);
	tcase_add_test(tc_calloc, test_calloc_returns_nonnull);
	tcase_add_test(tc_calloc, test_calloc_memory_zeroed);
	tcase_add_test(tc_calloc, test_calloc_updates_ptr);
	tcase_add_test(tc_calloc, test_calloc_new_block_zeroed);
	suite_add_tcase(s, tc_calloc);

	TCase *tc_clear = tcase_create("arena_clear");
	tcase_add_checked_fixture(tc_clear, arena_setup, arena_teardown);
	tcase_set_timeout(tc_clear, 10);
	tcase_add_test(tc_clear, test_clear_resets_first_ptr);
	tcase_add_test(tc_clear, test_clear_current_equals_first);
	tcase_add_test(tc_clear, test_clear_first_next_null);
	tcase_add_test(tc_clear, test_clear_single_block_works);
	tcase_add_test(tc_clear, test_clear_multi_block_works);
	tcase_add_test(tc_clear, test_clear_allows_reuse);
	tcase_add_test(tc_clear, test_clear_double_clear_safe);
	suite_add_tcase(s, tc_clear);

	TCase *tc_free = tcase_create("arena_free");
	tcase_set_timeout(tc_free, 10);
	// no fixture - tests manage their own arena lifecycle
	tcase_add_test(tc_free, test_free_single_block_no_crash);
	tcase_add_test(tc_free, test_free_multi_block_no_crash);
	suite_add_tcase(s, tc_free);

	return s;
}

int main(void)
{
	Suite *s = whisker_arena_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
