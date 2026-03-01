/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_hashmap
 * @created     : Saturday Feb 28, 2026 19:45:56 CST
 * @description : tests for whisker_hashmap.h hashmap implementation
 */

#include "whisker_std.h"
#include "whisker_hashmap.h"
#include "whisker_arena.h"
#include "whisker_hash_xxhash64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  fixture (16 buckets)      *
*****************************/

static struct w_arena g_arena;
static struct w_hashmap g_map;

static void hashmap_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_hashmap_init(&g_map, &g_arena, 16, sizeof(int), w_xxhash64_hash, NULL);
}

static void hashmap_teardown(void)
{
	w_hashmap_free(&g_map);
	w_arena_free(&g_arena);
}


/*****************************
*  hashmap_init tcase        *
*****************************/

START_TEST(test_init_buckets_allocated)
{
	ck_assert_ptr_nonnull(g_map.buckets);
}
END_TEST

START_TEST(test_init_bucket_count_stored)
{
	ck_assert_int_eq(g_map.buckets_length, 16);
}
END_TEST

START_TEST(test_init_total_entries_zero)
{
	ck_assert_int_eq(g_map.total_entries, 0);
}
END_TEST

START_TEST(test_init_value_size_stored)
{
	ck_assert_int_eq(g_map.value_size, sizeof(int));
}
END_TEST

START_TEST(test_init_hash_fn_stored)
{
	ck_assert_ptr_eq(g_map.hash_fn, w_xxhash64_hash);
}
END_TEST

START_TEST(test_init_default_equality_fn)
{
	ck_assert_ptr_eq(g_map.equality_fn, w_hashmap_eq_default);
}
END_TEST


/*****************************
*  hashmap_set tcase         *
*****************************/

START_TEST(test_set_increments_total_entries)
{
	int key = 42;
	int val = 100;
	w_hashmap_set(&g_map, &key, sizeof(key), &val);
	ck_assert_int_eq(g_map.total_entries, 1);
}
END_TEST

START_TEST(test_set_multiple_keys)
{
	for (int i = 0; i < 10; i++) {
		int val = i * 10;
		w_hashmap_set(&g_map, &i, sizeof(i), &val);
	}
	ck_assert_int_eq(g_map.total_entries, 10);
}
END_TEST

START_TEST(test_set_overwrite_same_key)
{
	int key = 5;
	int val1 = 100;
	int val2 = 200;
	w_hashmap_set(&g_map, &key, sizeof(key), &val1);
	w_hashmap_set(&g_map, &key, sizeof(key), &val2);
	// should not increment, key already exists
	ck_assert_int_eq(g_map.total_entries, 1);
}
END_TEST

START_TEST(test_set_string_key)
{
	const char *key = "hello";
	int val = 999;
	w_hashmap_set(&g_map, key, strlen(key), &val);
	ck_assert_int_eq(g_map.total_entries, 1);
}
END_TEST

START_TEST(test_set_many_keys_forces_bucket_growth)
{
	// insert enough keys to cause bucket entry array growth
	for (int i = 0; i < 100; i++) {
		int val = i;
		w_hashmap_set(&g_map, &i, sizeof(i), &val);
	}
	ck_assert_int_eq(g_map.total_entries, 100);
}
END_TEST


/*****************************
*  hashmap_get tcase         *
*****************************/

START_TEST(test_get_returns_correct_value)
{
	int key = 42;
	int val = 123;
	w_hashmap_set(&g_map, &key, sizeof(key), &val);
	int *result = w_hashmap_get(&g_map, &key, sizeof(key));
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 123);
}
END_TEST

START_TEST(test_get_nonexistent_returns_null)
{
	int key = 999;
	int *result = w_hashmap_get(&g_map, &key, sizeof(key));
	ck_assert_ptr_null(result);
}
END_TEST

START_TEST(test_get_after_overwrite)
{
	int key = 5;
	int val1 = 100;
	int val2 = 200;
	w_hashmap_set(&g_map, &key, sizeof(key), &val1);
	w_hashmap_set(&g_map, &key, sizeof(key), &val2);
	int *result = w_hashmap_get(&g_map, &key, sizeof(key));
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 200);
}
END_TEST

START_TEST(test_get_multiple_keys)
{
	for (int i = 0; i < 10; i++) {
		int val = i * 10;
		w_hashmap_set(&g_map, &i, sizeof(i), &val);
	}
	for (int i = 0; i < 10; i++) {
		int *result = w_hashmap_get(&g_map, &i, sizeof(i));
		ck_assert_ptr_nonnull(result);
		ck_assert_int_eq(*result, i * 10);
	}
}
END_TEST

START_TEST(test_get_string_key)
{
	const char *key = "world";
	int val = 777;
	w_hashmap_set(&g_map, key, strlen(key), &val);
	int *result = w_hashmap_get(&g_map, key, strlen(key));
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 777);
}
END_TEST

START_TEST(test_get_similar_string_keys)
{
	const char *key1 = "abc";
	const char *key2 = "abd";
	int val1 = 111;
	int val2 = 222;
	w_hashmap_set(&g_map, key1, strlen(key1), &val1);
	w_hashmap_set(&g_map, key2, strlen(key2), &val2);
	int *r1 = w_hashmap_get(&g_map, key1, strlen(key1));
	int *r2 = w_hashmap_get(&g_map, key2, strlen(key2));
	ck_assert_int_eq(*r1, 111);
	ck_assert_int_eq(*r2, 222);
}
END_TEST


/*****************************
*  hashmap_remove tcase      *
*****************************/

START_TEST(test_remove_returns_true)
{
	int key = 42;
	int val = 100;
	w_hashmap_set(&g_map, &key, sizeof(key), &val);
	bool removed = w_hashmap_remove(&g_map, &key, sizeof(key));
	ck_assert(removed);
}
END_TEST

START_TEST(test_remove_decrements_total_entries)
{
	int key = 42;
	int val = 100;
	w_hashmap_set(&g_map, &key, sizeof(key), &val);
	w_hashmap_remove(&g_map, &key, sizeof(key));
	ck_assert_int_eq(g_map.total_entries, 0);
}
END_TEST

START_TEST(test_remove_nonexistent_returns_false)
{
	int key = 999;
	bool removed = w_hashmap_remove(&g_map, &key, sizeof(key));
	ck_assert(!removed);
}
END_TEST

START_TEST(test_remove_get_returns_null)
{
	int key = 42;
	int val = 100;
	w_hashmap_set(&g_map, &key, sizeof(key), &val);
	w_hashmap_remove(&g_map, &key, sizeof(key));
	int *result = w_hashmap_get(&g_map, &key, sizeof(key));
	ck_assert_ptr_null(result);
}
END_TEST

START_TEST(test_remove_middle_entry)
{
	// insert 3 entries, remove middle, verify others still accessible
	int keys[] = {1, 2, 3};
	int vals[] = {10, 20, 30};
	for (int i = 0; i < 3; i++) {
		w_hashmap_set(&g_map, &keys[i], sizeof(keys[i]), &vals[i]);
	}
	w_hashmap_remove(&g_map, &keys[1], sizeof(keys[1]));
	ck_assert_int_eq(g_map.total_entries, 2);
	ck_assert_int_eq(*(int *)w_hashmap_get(&g_map, &keys[0], sizeof(keys[0])), 10);
	ck_assert_int_eq(*(int *)w_hashmap_get(&g_map, &keys[2], sizeof(keys[2])), 30);
	ck_assert_ptr_null(w_hashmap_get(&g_map, &keys[1], sizeof(keys[1])));
}
END_TEST

START_TEST(test_remove_and_reinsert)
{
	int key = 42;
	int val1 = 100;
	int val2 = 200;
	w_hashmap_set(&g_map, &key, sizeof(key), &val1);
	w_hashmap_remove(&g_map, &key, sizeof(key));
	w_hashmap_set(&g_map, &key, sizeof(key), &val2);
	int *result = w_hashmap_get(&g_map, &key, sizeof(key));
	ck_assert_int_eq(*result, 200);
	ck_assert_int_eq(g_map.total_entries, 1);
}
END_TEST


/*****************************
*  hashmap_total_entries     *
*****************************/

START_TEST(test_total_entries_empty)
{
	ck_assert_int_eq(w_hashmap_total_entries(&g_map), 0);
}
END_TEST

START_TEST(test_total_entries_after_inserts)
{
	for (int i = 0; i < 5; i++) {
		w_hashmap_set(&g_map, &i, sizeof(i), &i);
	}
	ck_assert_int_eq(w_hashmap_total_entries(&g_map), 5);
}
END_TEST

START_TEST(test_total_entries_after_removes)
{
	for (int i = 0; i < 5; i++) {
		w_hashmap_set(&g_map, &i, sizeof(i), &i);
	}
	int key = 2;
	w_hashmap_remove(&g_map, &key, sizeof(key));
	ck_assert_int_eq(w_hashmap_total_entries(&g_map), 4);
}
END_TEST


/*****************************
*  collision handling        *
*****************************/

START_TEST(test_collision_many_same_bucket)
{
	// with 16 buckets, keys 0, 16, 32, 48 will hash to same bucket (mod 16)
	// test that chaining works
	int keys[] = {0, 16, 32, 48};
	int vals[] = {100, 200, 300, 400};
	for (int i = 0; i < 4; i++) {
		w_hashmap_set(&g_map, &keys[i], sizeof(keys[i]), &vals[i]);
	}
	ck_assert_int_eq(g_map.total_entries, 4);
	for (int i = 0; i < 4; i++) {
		int *result = w_hashmap_get(&g_map, &keys[i], sizeof(keys[i]));
		ck_assert_ptr_nonnull(result);
		ck_assert_int_eq(*result, vals[i]);
	}
}
END_TEST

START_TEST(test_collision_remove_first)
{
	int keys[] = {0, 16, 32};
	int vals[] = {100, 200, 300};
	for (int i = 0; i < 3; i++) {
		w_hashmap_set(&g_map, &keys[i], sizeof(keys[i]), &vals[i]);
	}
	w_hashmap_remove(&g_map, &keys[0], sizeof(keys[0]));
	ck_assert_ptr_null(w_hashmap_get(&g_map, &keys[0], sizeof(keys[0])));
	ck_assert_int_eq(*(int *)w_hashmap_get(&g_map, &keys[1], sizeof(keys[1])), 200);
	ck_assert_int_eq(*(int *)w_hashmap_get(&g_map, &keys[2], sizeof(keys[2])), 300);
}
END_TEST

START_TEST(test_collision_remove_middle)
{
	int keys[] = {0, 16, 32};
	int vals[] = {100, 200, 300};
	for (int i = 0; i < 3; i++) {
		w_hashmap_set(&g_map, &keys[i], sizeof(keys[i]), &vals[i]);
	}
	w_hashmap_remove(&g_map, &keys[1], sizeof(keys[1]));
	ck_assert_int_eq(*(int *)w_hashmap_get(&g_map, &keys[0], sizeof(keys[0])), 100);
	ck_assert_ptr_null(w_hashmap_get(&g_map, &keys[1], sizeof(keys[1])));
	ck_assert_int_eq(*(int *)w_hashmap_get(&g_map, &keys[2], sizeof(keys[2])), 300);
}
END_TEST

START_TEST(test_collision_remove_last)
{
	int keys[] = {0, 16, 32};
	int vals[] = {100, 200, 300};
	for (int i = 0; i < 3; i++) {
		w_hashmap_set(&g_map, &keys[i], sizeof(keys[i]), &vals[i]);
	}
	w_hashmap_remove(&g_map, &keys[2], sizeof(keys[2]));
	ck_assert_int_eq(*(int *)w_hashmap_get(&g_map, &keys[0], sizeof(keys[0])), 100);
	ck_assert_int_eq(*(int *)w_hashmap_get(&g_map, &keys[1], sizeof(keys[1])), 200);
	ck_assert_ptr_null(w_hashmap_get(&g_map, &keys[2], sizeof(keys[2])));
}
END_TEST


/*****************************
*  hashmap_free tcase        *
*  (no fixture - self-managed)
*****************************/

START_TEST(test_free_empty_map)
{
	struct w_arena a;
	struct w_hashmap m;
	w_arena_init(&a, 1024);
	w_hashmap_init(&m, &a, 8, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_free(&m);
	w_arena_free(&a);
	// no crash, buckets should be null
	ck_assert_ptr_null(m.buckets);
}
END_TEST

START_TEST(test_free_with_entries)
{
	struct w_arena a;
	struct w_hashmap m;
	w_arena_init(&a, 1024);
	w_hashmap_init(&m, &a, 8, sizeof(int), w_xxhash64_hash, NULL);
	for (int i = 0; i < 20; i++) {
		w_hashmap_set(&m, &i, sizeof(i), &i);
	}
	w_hashmap_free(&m);
	w_arena_free(&a);
	ck_assert_ptr_null(m.buckets);
	ck_assert_int_eq(m.total_entries, 0);
}
END_TEST


/*****************************
*  edge cases tcase          *
*****************************/

START_TEST(test_zero_length_key)
{
	// zero-length key should work (edge case)
	int val = 42;
	w_hashmap_set(&g_map, "", 0, &val);
	int *result = w_hashmap_get(&g_map, "", 0);
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 42);
}
END_TEST

START_TEST(test_large_value_size)
{
	// test with larger value type
	struct w_arena a;
	struct w_hashmap m;
	w_arena_init(&a, 4096);

	struct big_value { char data[256]; };
	w_hashmap_init(&m, &a, 8, sizeof(struct big_value), w_xxhash64_hash, NULL);

	int key = 1;
	struct big_value val;
	memset(val.data, 'A', 255);
	val.data[255] = '\0';

	w_hashmap_set(&m, &key, sizeof(key), &val);
	struct big_value *result = w_hashmap_get(&m, &key, sizeof(key));
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(result->data[0], 'A');
	ck_assert_int_eq(result->data[254], 'A');

	w_hashmap_free(&m);
	w_arena_free(&a);
}
END_TEST

START_TEST(test_single_bucket)
{
	// edge case: map with only 1 bucket
	struct w_arena a;
	struct w_hashmap m;
	w_arena_init(&a, 1024);
	w_hashmap_init(&m, &a, 1, sizeof(int), w_xxhash64_hash, NULL);

	for (int i = 0; i < 10; i++) {
		w_hashmap_set(&m, &i, sizeof(i), &i);
	}
	ck_assert_int_eq(m.total_entries, 10);

	for (int i = 0; i < 10; i++) {
		int *result = w_hashmap_get(&m, &i, sizeof(i));
		ck_assert_int_eq(*result, i);
	}

	w_hashmap_free(&m);
	w_arena_free(&a);
}
END_TEST


/*****************************
*  macro hashmap tests       *
*****************************/

// declare typed hashmap for int->int
w_hashmap_t_declare(int, int, int_hashmap);

static struct w_arena g_macro_arena;
static struct int_hashmap g_macro_map;

static void macro_hashmap_setup(void)
{
	w_arena_init(&g_macro_arena, 4096);
	w_hashmap_t_init(&g_macro_map, &g_macro_arena, 16, w_xxhash64_hash, NULL);
}

static void macro_hashmap_teardown(void)
{
	w_hashmap_t_free(&g_macro_map);
	w_arena_free(&g_macro_arena);
}

START_TEST(macro_test_init_buckets_allocated)
{
	ck_assert_ptr_nonnull(g_macro_map.buckets);
}
END_TEST

START_TEST(macro_test_init_bucket_count)
{
	ck_assert_int_eq(g_macro_map.buckets_length, 16);
}
END_TEST

START_TEST(macro_test_init_total_entries_zero)
{
	ck_assert_int_eq(g_macro_map.total_entries, 0);
}
END_TEST

START_TEST(macro_test_set_increments_entries)
{
	w_hashmap_t_set(&g_macro_map, 42, 100);
	ck_assert_int_eq(g_macro_map.total_entries, 1);
}
END_TEST

START_TEST(macro_test_set_multiple_keys)
{
	for (int i = 0; i < 10; i++) {
		w_hashmap_t_set(&g_macro_map, i, i * 10);
	}
	ck_assert_int_eq(g_macro_map.total_entries, 10);
}
END_TEST

START_TEST(macro_test_set_overwrite)
{
	w_hashmap_t_set(&g_macro_map, 5, 100);
	w_hashmap_t_set(&g_macro_map, 5, 200);
	ck_assert_int_eq(g_macro_map.total_entries, 1);
}
END_TEST

START_TEST(macro_test_get_returns_value)
{
	w_hashmap_t_set(&g_macro_map, 42, 123);
	int *result;
	w_hashmap_t_get(&g_macro_map, 42, result);
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 123);
}
END_TEST

START_TEST(macro_test_get_nonexistent)
{
	int *result;
	w_hashmap_t_get(&g_macro_map, 999, result);
	ck_assert_ptr_null(result);
}
END_TEST

START_TEST(macro_test_get_after_overwrite)
{
	w_hashmap_t_set(&g_macro_map, 5, 100);
	w_hashmap_t_set(&g_macro_map, 5, 200);
	int *result;
	w_hashmap_t_get(&g_macro_map, 5, result);
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 200);
}
END_TEST

START_TEST(macro_test_get_multiple_keys)
{
	for (int i = 0; i < 10; i++) {
		w_hashmap_t_set(&g_macro_map, i, i * 10);
	}
	for (int i = 0; i < 10; i++) {
		int *result;
		w_hashmap_t_get(&g_macro_map, i, result);
		ck_assert_ptr_nonnull(result);
		ck_assert_int_eq(*result, i * 10);
	}
}
END_TEST

START_TEST(macro_test_remove_returns_true)
{
	w_hashmap_t_set(&g_macro_map, 42, 100);
	bool removed;
	w_hashmap_t_remove(&g_macro_map, 42, removed);
	ck_assert(removed);
}
END_TEST

START_TEST(macro_test_remove_decrements_entries)
{
	w_hashmap_t_set(&g_macro_map, 42, 100);
	bool removed;
	w_hashmap_t_remove(&g_macro_map, 42, removed);
	ck_assert_int_eq(g_macro_map.total_entries, 0);
}
END_TEST

START_TEST(macro_test_remove_nonexistent)
{
	bool removed;
	w_hashmap_t_remove(&g_macro_map, 999, removed);
	ck_assert(!removed);
}
END_TEST

START_TEST(macro_test_remove_get_null)
{
	w_hashmap_t_set(&g_macro_map, 42, 100);
	bool removed;
	w_hashmap_t_remove(&g_macro_map, 42, removed);
	int *result;
	w_hashmap_t_get(&g_macro_map, 42, result);
	ck_assert_ptr_null(result);
}
END_TEST

START_TEST(macro_test_collision_handling)
{
	// keys 0, 16, 32, 48 hash to same bucket (mod 16)
	int keys[] = {0, 16, 32, 48};
	int vals[] = {100, 200, 300, 400};
	for (int i = 0; i < 4; i++) {
		w_hashmap_t_set(&g_macro_map, keys[i], vals[i]);
	}
	ck_assert_int_eq(g_macro_map.total_entries, 4);
	for (int i = 0; i < 4; i++) {
		int *result;
		w_hashmap_t_get(&g_macro_map, keys[i], result);
		ck_assert_ptr_nonnull(result);
		ck_assert_int_eq(*result, vals[i]);
	}
}
END_TEST

START_TEST(macro_test_total_entries)
{
	ck_assert_int_eq(w_hashmap_t_total_entries(&g_macro_map), 0);
	for (int i = 0; i < 5; i++) {
		w_hashmap_t_set(&g_macro_map, i, i);
	}
	ck_assert_int_eq(w_hashmap_t_total_entries(&g_macro_map), 5);
}
END_TEST

START_TEST(macro_test_many_keys_bucket_growth)
{
	for (int i = 0; i < 100; i++) {
		w_hashmap_t_set(&g_macro_map, i, i);
	}
	ck_assert_int_eq(g_macro_map.total_entries, 100);
	for (int i = 0; i < 100; i++) {
		int *result;
		w_hashmap_t_get(&g_macro_map, i, result);
		ck_assert_ptr_nonnull(result);
		ck_assert_int_eq(*result, i);
	}
}
END_TEST

// test with different types: uint64_t -> double
w_hashmap_t_declare(uint64_t, double, u64_double_hashmap);

// declare typed hashmap for string keys (const char* -> int)
w_hashmap_t_declare(const char *, int, str_hashmap);

START_TEST(macro_test_different_types)
{
	struct w_arena a;
	struct u64_double_hashmap m;
	w_arena_init(&a, 4096);
	w_hashmap_t_init(&m, &a, 8, w_xxhash64_hash, NULL);

	w_hashmap_t_set(&m, (uint64_t)12345, 3.14159);
	w_hashmap_t_set(&m, (uint64_t)67890, 2.71828);

	double *r1;
	w_hashmap_t_get(&m, (uint64_t)12345, r1);
	ck_assert_ptr_nonnull(r1);
	ck_assert_double_eq_tol(*r1, 3.14159, 0.00001);

	double *r2;
	w_hashmap_t_get(&m, (uint64_t)67890, r2);
	ck_assert_ptr_nonnull(r2);
	ck_assert_double_eq_tol(*r2, 2.71828, 0.00001);

	w_hashmap_t_free(&m);
	w_arena_free(&a);
}
END_TEST


/*****************************
*  void* string key tests    *
*****************************/

START_TEST(test_void_string_literal_key)
{
	// string literal passed directly
	int val = 42;
	w_hashmap_set(&g_map, "literal_key", strlen("literal_key"), &val);
	int *result = w_hashmap_get(&g_map, "literal_key", strlen("literal_key"));
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 42);
}
END_TEST

START_TEST(test_void_char_ptr_key)
{
	// char* variable as key
	char *key = "char_ptr_key";
	int val = 123;
	w_hashmap_set(&g_map, key, strlen(key), &val);
	int *result = w_hashmap_get(&g_map, key, strlen(key));
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 123);
}
END_TEST

START_TEST(test_void_const_char_ptr_key)
{
	// const char* variable as key
	const char *key = "const_char_ptr_key";
	int val = 456;
	w_hashmap_set(&g_map, key, strlen(key), &val);
	int *result = w_hashmap_get(&g_map, key, strlen(key));
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 456);
}
END_TEST

START_TEST(test_void_string_key_overwrite)
{
	const char *key = "overwrite_key";
	int val1 = 100;
	int val2 = 200;
	w_hashmap_set(&g_map, key, strlen(key), &val1);
	w_hashmap_set(&g_map, key, strlen(key), &val2);
	int *result = w_hashmap_get(&g_map, key, strlen(key));
	ck_assert_int_eq(*result, 200);
	ck_assert_int_eq(g_map.total_entries, 1);
}
END_TEST

START_TEST(test_void_string_key_remove)
{
	const char *key = "remove_key";
	int val = 999;
	w_hashmap_set(&g_map, key, strlen(key), &val);
	bool removed = w_hashmap_remove(&g_map, key, strlen(key));
	ck_assert(removed);
	ck_assert_ptr_null(w_hashmap_get(&g_map, key, strlen(key)));
}
END_TEST

START_TEST(test_void_multiple_string_keys)
{
	const char *keys[] = {"apple", "banana", "cherry", "date"};
	int vals[] = {1, 2, 3, 4};
	for (int i = 0; i < 4; i++) {
		w_hashmap_set(&g_map, keys[i], strlen(keys[i]), &vals[i]);
	}
	ck_assert_int_eq(g_map.total_entries, 4);
	for (int i = 0; i < 4; i++) {
		int *result = w_hashmap_get(&g_map, keys[i], strlen(keys[i]));
		ck_assert_int_eq(*result, vals[i]);
	}
}
END_TEST


/*****************************
*  macro string key tests    *
*****************************/

START_TEST(macro_test_string_literal_key)
{
	struct w_arena a;
	struct str_hashmap m;
	w_arena_init(&a, 4096);
	w_hashmap_t_init(&m, &a, 16, w_hashmap_hash_str, w_hashmap_eq_str);

	// string literal
	w_hashmap_t_set(&m, "hello", 100);
	int *result;
	w_hashmap_t_get(&m, "hello", result);
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 100);

	w_hashmap_t_free(&m);
	w_arena_free(&a);
}
END_TEST

START_TEST(macro_test_char_ptr_key)
{
	struct w_arena a;
	struct str_hashmap m;
	w_arena_init(&a, 4096);
	w_hashmap_t_init(&m, &a, 16, w_hashmap_hash_str, w_hashmap_eq_str);

	// char* variable
	char *key = "char_ptr";
	w_hashmap_t_set(&m, key, 200);
	int *result;
	w_hashmap_t_get(&m, key, result);
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 200);

	w_hashmap_t_free(&m);
	w_arena_free(&a);
}
END_TEST

START_TEST(macro_test_const_char_ptr_key)
{
	struct w_arena a;
	struct str_hashmap m;
	w_arena_init(&a, 4096);
	w_hashmap_t_init(&m, &a, 16, w_hashmap_hash_str, w_hashmap_eq_str);

	// const char* variable
	const char *key = "const_char_ptr";
	w_hashmap_t_set(&m, key, 300);
	int *result;
	w_hashmap_t_get(&m, key, result);
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 300);

	w_hashmap_t_free(&m);
	w_arena_free(&a);
}
END_TEST

START_TEST(macro_test_string_key_overwrite)
{
	struct w_arena a;
	struct str_hashmap m;
	w_arena_init(&a, 4096);
	w_hashmap_t_init(&m, &a, 16, w_hashmap_hash_str, w_hashmap_eq_str);

	w_hashmap_t_set(&m, "key", 100);
	w_hashmap_t_set(&m, "key", 200);
	ck_assert_int_eq(m.total_entries, 1);
	int *result;
	w_hashmap_t_get(&m, "key", result);
	ck_assert_int_eq(*result, 200);

	w_hashmap_t_free(&m);
	w_arena_free(&a);
}
END_TEST

START_TEST(macro_test_string_key_remove)
{
	struct w_arena a;
	struct str_hashmap m;
	w_arena_init(&a, 4096);
	w_hashmap_t_init(&m, &a, 16, w_hashmap_hash_str, w_hashmap_eq_str);

	w_hashmap_t_set(&m, "remove_me", 999);
	bool removed;
	w_hashmap_t_remove(&m, "remove_me", removed);
	ck_assert(removed);
	int *result;
	w_hashmap_t_get(&m, "remove_me", result);
	ck_assert_ptr_null(result);

	w_hashmap_t_free(&m);
	w_arena_free(&a);
}
END_TEST

START_TEST(macro_test_multiple_string_keys)
{
	struct w_arena a;
	struct str_hashmap m;
	w_arena_init(&a, 4096);
	w_hashmap_t_init(&m, &a, 16, w_hashmap_hash_str, w_hashmap_eq_str);

	const char *keys[] = {"one", "two", "three", "four", "five"};
	for (int i = 0; i < 5; i++) {
		w_hashmap_t_set(&m, keys[i], i * 10);
	}
	ck_assert_int_eq(m.total_entries, 5);
	for (int i = 0; i < 5; i++) {
		int *result;
		w_hashmap_t_get(&m, keys[i], result);
		ck_assert_ptr_nonnull(result);
		ck_assert_int_eq(*result, i * 10);
	}

	w_hashmap_t_free(&m);
	w_arena_free(&a);
}
END_TEST

START_TEST(macro_test_string_key_different_pointers_same_content)
{
	// Test that two different char* pointing to same content work
	struct w_arena a;
	struct str_hashmap m;
	w_arena_init(&a, 4096);
	w_hashmap_t_init(&m, &a, 16, w_hashmap_hash_str, w_hashmap_eq_str);

	char key1[16];
	char key2[16];
	strcpy(key1, "same_content");
	strcpy(key2, "same_content");

	// key1 and key2 are different pointers but same content
	ck_assert_ptr_ne(key1, key2);

	w_hashmap_t_set(&m, (const char *)key1, 111);
	int *result;
	w_hashmap_t_get(&m, (const char *)key2, result);
	ck_assert_ptr_nonnull(result);
	ck_assert_int_eq(*result, 111);

	// overwrite using key2
	w_hashmap_t_set(&m, (const char *)key2, 222);
	ck_assert_int_eq(m.total_entries, 1);
	w_hashmap_t_get(&m, (const char *)key1, result);
	ck_assert_int_eq(*result, 222);

	w_hashmap_t_free(&m);
	w_arena_free(&a);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_hashmap_suite(void)
{
	Suite *s = suite_create("whisker_hashmap");

	TCase *tc_init = tcase_create("hashmap_init");
	tcase_add_checked_fixture(tc_init, hashmap_setup, hashmap_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_buckets_allocated);
	tcase_add_test(tc_init, test_init_bucket_count_stored);
	tcase_add_test(tc_init, test_init_total_entries_zero);
	tcase_add_test(tc_init, test_init_value_size_stored);
	tcase_add_test(tc_init, test_init_hash_fn_stored);
	tcase_add_test(tc_init, test_init_default_equality_fn);
	suite_add_tcase(s, tc_init);

	TCase *tc_set = tcase_create("hashmap_set");
	tcase_add_checked_fixture(tc_set, hashmap_setup, hashmap_teardown);
	tcase_set_timeout(tc_set, 10);
	tcase_add_test(tc_set, test_set_increments_total_entries);
	tcase_add_test(tc_set, test_set_multiple_keys);
	tcase_add_test(tc_set, test_set_overwrite_same_key);
	tcase_add_test(tc_set, test_set_string_key);
	tcase_add_test(tc_set, test_set_many_keys_forces_bucket_growth);
	suite_add_tcase(s, tc_set);

	TCase *tc_get = tcase_create("hashmap_get");
	tcase_add_checked_fixture(tc_get, hashmap_setup, hashmap_teardown);
	tcase_set_timeout(tc_get, 10);
	tcase_add_test(tc_get, test_get_returns_correct_value);
	tcase_add_test(tc_get, test_get_nonexistent_returns_null);
	tcase_add_test(tc_get, test_get_after_overwrite);
	tcase_add_test(tc_get, test_get_multiple_keys);
	tcase_add_test(tc_get, test_get_string_key);
	tcase_add_test(tc_get, test_get_similar_string_keys);
	suite_add_tcase(s, tc_get);

	TCase *tc_remove = tcase_create("hashmap_remove");
	tcase_add_checked_fixture(tc_remove, hashmap_setup, hashmap_teardown);
	tcase_set_timeout(tc_remove, 10);
	tcase_add_test(tc_remove, test_remove_returns_true);
	tcase_add_test(tc_remove, test_remove_decrements_total_entries);
	tcase_add_test(tc_remove, test_remove_nonexistent_returns_false);
	tcase_add_test(tc_remove, test_remove_get_returns_null);
	tcase_add_test(tc_remove, test_remove_middle_entry);
	tcase_add_test(tc_remove, test_remove_and_reinsert);
	suite_add_tcase(s, tc_remove);

	TCase *tc_total = tcase_create("hashmap_total_entries");
	tcase_add_checked_fixture(tc_total, hashmap_setup, hashmap_teardown);
	tcase_set_timeout(tc_total, 10);
	tcase_add_test(tc_total, test_total_entries_empty);
	tcase_add_test(tc_total, test_total_entries_after_inserts);
	tcase_add_test(tc_total, test_total_entries_after_removes);
	suite_add_tcase(s, tc_total);

	TCase *tc_collision = tcase_create("collision_handling");
	tcase_add_checked_fixture(tc_collision, hashmap_setup, hashmap_teardown);
	tcase_set_timeout(tc_collision, 10);
	tcase_add_test(tc_collision, test_collision_many_same_bucket);
	tcase_add_test(tc_collision, test_collision_remove_first);
	tcase_add_test(tc_collision, test_collision_remove_middle);
	tcase_add_test(tc_collision, test_collision_remove_last);
	suite_add_tcase(s, tc_collision);

	TCase *tc_free = tcase_create("hashmap_free");
	tcase_set_timeout(tc_free, 10);
	tcase_add_test(tc_free, test_free_empty_map);
	tcase_add_test(tc_free, test_free_with_entries);
	suite_add_tcase(s, tc_free);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_add_checked_fixture(tc_edge, hashmap_setup, hashmap_teardown);
	tcase_set_timeout(tc_edge, 10);
	tcase_add_test(tc_edge, test_zero_length_key);
	tcase_add_test(tc_edge, test_large_value_size);
	tcase_add_test(tc_edge, test_single_bucket);
	suite_add_tcase(s, tc_edge);

	TCase *tc_macro = tcase_create("macro_hashmap");
	tcase_add_checked_fixture(tc_macro, macro_hashmap_setup, macro_hashmap_teardown);
	tcase_set_timeout(tc_macro, 10);
	tcase_add_test(tc_macro, macro_test_init_buckets_allocated);
	tcase_add_test(tc_macro, macro_test_init_bucket_count);
	tcase_add_test(tc_macro, macro_test_init_total_entries_zero);
	tcase_add_test(tc_macro, macro_test_set_increments_entries);
	tcase_add_test(tc_macro, macro_test_set_multiple_keys);
	tcase_add_test(tc_macro, macro_test_set_overwrite);
	tcase_add_test(tc_macro, macro_test_get_returns_value);
	tcase_add_test(tc_macro, macro_test_get_nonexistent);
	tcase_add_test(tc_macro, macro_test_get_after_overwrite);
	tcase_add_test(tc_macro, macro_test_get_multiple_keys);
	tcase_add_test(tc_macro, macro_test_remove_returns_true);
	tcase_add_test(tc_macro, macro_test_remove_decrements_entries);
	tcase_add_test(tc_macro, macro_test_remove_nonexistent);
	tcase_add_test(tc_macro, macro_test_remove_get_null);
	tcase_add_test(tc_macro, macro_test_collision_handling);
	tcase_add_test(tc_macro, macro_test_total_entries);
	tcase_add_test(tc_macro, macro_test_many_keys_bucket_growth);
	tcase_add_test(tc_macro, macro_test_different_types);
	suite_add_tcase(s, tc_macro);

	TCase *tc_void_str = tcase_create("void_string_keys");
	tcase_add_checked_fixture(tc_void_str, hashmap_setup, hashmap_teardown);
	tcase_set_timeout(tc_void_str, 10);
	tcase_add_test(tc_void_str, test_void_string_literal_key);
	tcase_add_test(tc_void_str, test_void_char_ptr_key);
	tcase_add_test(tc_void_str, test_void_const_char_ptr_key);
	tcase_add_test(tc_void_str, test_void_string_key_overwrite);
	tcase_add_test(tc_void_str, test_void_string_key_remove);
	tcase_add_test(tc_void_str, test_void_multiple_string_keys);
	suite_add_tcase(s, tc_void_str);

	TCase *tc_macro_str = tcase_create("macro_string_keys");
	tcase_set_timeout(tc_macro_str, 10);
	tcase_add_test(tc_macro_str, macro_test_string_literal_key);
	tcase_add_test(tc_macro_str, macro_test_char_ptr_key);
	tcase_add_test(tc_macro_str, macro_test_const_char_ptr_key);
	tcase_add_test(tc_macro_str, macro_test_string_key_overwrite);
	tcase_add_test(tc_macro_str, macro_test_string_key_remove);
	tcase_add_test(tc_macro_str, macro_test_multiple_string_keys);
	tcase_add_test(tc_macro_str, macro_test_string_key_different_pointers_same_content);
	suite_add_tcase(s, tc_macro_str);

	return s;
}

int main(void)
{
	Suite *s = whisker_hashmap_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
