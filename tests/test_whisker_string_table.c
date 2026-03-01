/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_string_table
 * @created     : Sunday Mar 01, 2026 16:11:25 CST
 * @description : tests for whisker_string_table.h string interning
 */

#include "whisker_std.h"
#include "whisker_string_table.h"
#include "whisker_arena.h"
#include "whisker_hash_xxhash64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_arena g_arena;
static struct w_string_table g_table;

static void string_table_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_table, &g_arena, WHISKER_STRING_TABLE_REALLOC_SIZE, WHISKER_STRING_TABLE_BUCKETS_SIZE, w_xxhash64_hash);
}

static void string_table_teardown(void)
{
	w_string_table_free(&g_table);
	w_arena_free(&g_arena);
}


/*****************************
*  string_table_init tcase   *
*****************************/

START_TEST(test_init_arena_stored)
{
	ck_assert_ptr_eq(g_table.arena, &g_arena);
}
END_TEST

START_TEST(test_init_entries_null_at_start)
{
	// entries array starts empty
	ck_assert_int_eq(g_table.entries_length, 0);
}
END_TEST

START_TEST(test_init_realloc_block_size_stored)
{
	ck_assert_int_eq(g_table.entries_realloc_block_size, WHISKER_STRING_TABLE_REALLOC_SIZE);
}
END_TEST

START_TEST(test_init_hashmap_initialized)
{
	ck_assert_ptr_nonnull(g_table.reverse_map.buckets);
	ck_assert_int_eq(g_table.reverse_map.buckets_length, WHISKER_STRING_TABLE_BUCKETS_SIZE);
}
END_TEST


/*****************************
*  string_table_intern_str   *
*****************************/

START_TEST(test_intern_str_returns_zero_first)
{
	w_string_table_id id = w_string_table_intern_str(&g_table, "hello");
	ck_assert_int_eq(id, 0);
}
END_TEST

START_TEST(test_intern_str_increments_id)
{
	w_string_table_id id1 = w_string_table_intern_str(&g_table, "hello");
	w_string_table_id id2 = w_string_table_intern_str(&g_table, "world");
	ck_assert_int_eq(id1, 0);
	ck_assert_int_eq(id2, 1);
}
END_TEST

START_TEST(test_intern_str_duplicate_returns_same_id)
{
	w_string_table_id id1 = w_string_table_intern_str(&g_table, "duplicate");
	w_string_table_id id2 = w_string_table_intern_str(&g_table, "duplicate");
	ck_assert_int_eq(id1, id2);
}
END_TEST

START_TEST(test_intern_str_duplicate_no_new_entry)
{
	w_string_table_intern_str(&g_table, "test");
	w_string_table_intern_str(&g_table, "test");
	ck_assert_int_eq(g_table.entries_length, 1);
}
END_TEST

START_TEST(test_intern_str_multiple_strings)
{
	const char *strings[] = {"one", "two", "three", "four", "five"};
	for (int i = 0; i < 5; i++) {
		w_string_table_id id = w_string_table_intern_str(&g_table, strings[i]);
		ck_assert_int_eq(id, i);
	}
	ck_assert_int_eq(g_table.entries_length, 5);
}
END_TEST

START_TEST(test_intern_str_empty_string)
{
	w_string_table_id id = w_string_table_intern_str(&g_table, "");
	ck_assert_int_eq(id, 0);
	char *str = w_string_table_lookup(&g_table, id);
	ck_assert_ptr_nonnull(str);
	ck_assert_str_eq(str, "");
}
END_TEST

START_TEST(test_intern_str_similar_strings)
{
	w_string_table_id id1 = w_string_table_intern_str(&g_table, "abc");
	w_string_table_id id2 = w_string_table_intern_str(&g_table, "abd");
	w_string_table_id id3 = w_string_table_intern_str(&g_table, "ab");
	ck_assert_int_ne(id1, id2);
	ck_assert_int_ne(id1, id3);
	ck_assert_int_ne(id2, id3);
}
END_TEST


/*****************************
*  string_table_intern_strn  *
*****************************/

START_TEST(test_intern_strn_with_length)
{
	// intern only "hello" from "hello world"
	w_string_table_id id = w_string_table_intern_strn(&g_table, "hello world", 5);
	char *str = w_string_table_lookup(&g_table, id);
	ck_assert_ptr_nonnull(str);
	ck_assert_str_eq(str, "hello");
}
END_TEST

START_TEST(test_intern_strn_partial_match)
{
	// "hello" should not match "hello world" when interned separately
	w_string_table_id id1 = w_string_table_intern_str(&g_table, "hello");
	w_string_table_id id2 = w_string_table_intern_str(&g_table, "hello world");
	ck_assert_int_ne(id1, id2);
}
END_TEST

START_TEST(test_intern_strn_duplicate_detection)
{
	w_string_table_id id1 = w_string_table_intern_strn(&g_table, "test string", 4);
	w_string_table_id id2 = w_string_table_intern_str(&g_table, "test");
	ck_assert_int_eq(id1, id2);
}
END_TEST

START_TEST(test_intern_strn_zero_length)
{
	w_string_table_id id = w_string_table_intern_strn(&g_table, "anything", 0);
	char *str = w_string_table_lookup(&g_table, id);
	ck_assert_ptr_nonnull(str);
	ck_assert_str_eq(str, "");
}
END_TEST

START_TEST(test_intern_strn_full_length)
{
	const char *s = "complete";
	w_string_table_id id = w_string_table_intern_strn(&g_table, s, strlen(s));
	char *str = w_string_table_lookup(&g_table, id);
	ck_assert_str_eq(str, "complete");
}
END_TEST


/*****************************
*  string_table_lookup       *
*****************************/

START_TEST(test_lookup_returns_string)
{
	w_string_table_id id = w_string_table_intern_str(&g_table, "lookup_test");
	char *str = w_string_table_lookup(&g_table, id);
	ck_assert_ptr_nonnull(str);
	ck_assert_str_eq(str, "lookup_test");
}
END_TEST

START_TEST(test_lookup_invalid_id)
{
	char *str = w_string_table_lookup(&g_table, W_STRING_TABLE_INVALID_ID);
	ck_assert_ptr_null(str);
}
END_TEST

START_TEST(test_lookup_out_of_range_id)
{
	w_string_table_intern_str(&g_table, "only_one");
	char *str = w_string_table_lookup(&g_table, 999);
	ck_assert_ptr_null(str);
}
END_TEST

START_TEST(test_lookup_multiple_strings)
{
	const char *strings[] = {"alpha", "beta", "gamma"};
	w_string_table_id ids[3];
	for (int i = 0; i < 3; i++) {
		ids[i] = w_string_table_intern_str(&g_table, strings[i]);
	}
	for (int i = 0; i < 3; i++) {
		char *str = w_string_table_lookup(&g_table, ids[i]);
		ck_assert_str_eq(str, strings[i]);
	}
}
END_TEST

START_TEST(test_lookup_string_is_copy)
{
	// interned string should be a copy, not the original pointer
	char buf[32];
	strcpy(buf, "original");
	w_string_table_id id = w_string_table_intern_str(&g_table, buf);
	char *str = w_string_table_lookup(&g_table, id);
	ck_assert_ptr_ne(str, buf);
	ck_assert_str_eq(str, "original");
	// modifying buf should not affect interned string
	strcpy(buf, "modified");
	ck_assert_str_eq(str, "original");
}
END_TEST


/*****************************
*  string_table_lookup_entry *
*****************************/

START_TEST(test_lookup_entry_returns_entry)
{
	w_string_table_id id = w_string_table_intern_str(&g_table, "entry_test");
	struct w_string_table_entry *entry = w_string_table_lookup_entry(&g_table, id);
	ck_assert_ptr_nonnull(entry);
	ck_assert_str_eq(entry->string, "entry_test");
	ck_assert_int_eq(entry->length, strlen("entry_test"));
}
END_TEST

START_TEST(test_lookup_entry_invalid_id)
{
	struct w_string_table_entry *entry = w_string_table_lookup_entry(&g_table, W_STRING_TABLE_INVALID_ID);
	ck_assert_ptr_null(entry);
}
END_TEST

START_TEST(test_lookup_entry_out_of_range)
{
	w_string_table_intern_str(&g_table, "single");
	struct w_string_table_entry *entry = w_string_table_lookup_entry(&g_table, 100);
	ck_assert_ptr_null(entry);
}
END_TEST

START_TEST(test_lookup_entry_length_correct)
{
	w_string_table_id id = w_string_table_intern_strn(&g_table, "hello world", 5);
	struct w_string_table_entry *entry = w_string_table_lookup_entry(&g_table, id);
	ck_assert_int_eq(entry->length, 5);
}
END_TEST


/*****************************
*  lookup_entry_str tcase    *
*****************************/

START_TEST(test_lookup_entry_str_found)
{
	w_string_table_intern_str(&g_table, "findme");
	struct w_string_table_entry *entry = w_string_table_lookup_entry_str(&g_table, "findme");
	ck_assert_ptr_nonnull(entry);
	ck_assert_str_eq(entry->string, "findme");
}
END_TEST

START_TEST(test_lookup_entry_str_not_found)
{
	w_string_table_intern_str(&g_table, "exists");
	struct w_string_table_entry *entry = w_string_table_lookup_entry_str(&g_table, "notexists");
	ck_assert_ptr_null(entry);
}
END_TEST

START_TEST(test_lookup_entry_str_empty_table)
{
	struct w_string_table_entry *entry = w_string_table_lookup_entry_str(&g_table, "anything");
	ck_assert_ptr_null(entry);
}
END_TEST

START_TEST(test_lookup_entry_str_multiple)
{
	const char *strings[] = {"one", "two", "three"};
	for (int i = 0; i < 3; i++) {
		w_string_table_intern_str(&g_table, strings[i]);
	}
	for (int i = 0; i < 3; i++) {
		struct w_string_table_entry *entry = w_string_table_lookup_entry_str(&g_table, strings[i]);
		ck_assert_ptr_nonnull(entry);
		ck_assert_str_eq(entry->string, strings[i]);
	}
}
END_TEST


/*****************************
*  string_table_free tcase   *
*  (no fixture)              *
*****************************/

START_TEST(test_free_empty_table)
{
	struct w_arena a;
	struct w_string_table t;
	w_arena_init(&a, 1024);
	w_string_table_init(&t, &a, 64, 8, w_xxhash64_hash);
	w_string_table_free(&t);
	w_arena_free(&a);
	// no crash, entries should be null
	ck_assert_ptr_null(t.entries);
}
END_TEST

START_TEST(test_free_with_entries)
{
	struct w_arena a;
	struct w_string_table t;
	w_arena_init(&a, 4096);
	w_string_table_init(&t, &a, 64, 8, w_xxhash64_hash);
	for (int i = 0; i < 10; i++) {
		char buf[16];
		snprintf(buf, sizeof(buf), "str%d", i);
		w_string_table_intern_str(&t, buf);
	}
	w_string_table_free(&t);
	w_arena_free(&a);
	ck_assert_ptr_null(t.entries);
}
END_TEST


/*****************************
*  edge cases tcase          *
*****************************/

START_TEST(test_many_strings)
{
	// test with many strings to ensure growth works
	for (int i = 0; i < 1000; i++) {
		char buf[32];
		snprintf(buf, sizeof(buf), "string_%d", i);
		w_string_table_id id = w_string_table_intern_str(&g_table, buf);
		ck_assert_int_eq(id, i);
	}
	ck_assert_int_eq(g_table.entries_length, 1000);
	// verify all strings are retrievable
	for (int i = 0; i < 1000; i++) {
		char buf[32];
		snprintf(buf, sizeof(buf), "string_%d", i);
		char *str = w_string_table_lookup(&g_table, i);
		ck_assert_str_eq(str, buf);
	}
}
END_TEST

START_TEST(test_long_strings)
{
	// test with a long string
	char long_str[1024];
	memset(long_str, 'x', 1023);
	long_str[1023] = '\0';
	w_string_table_id id = w_string_table_intern_str(&g_table, long_str);
	char *str = w_string_table_lookup(&g_table, id);
	ck_assert_int_eq(strlen(str), 1023);
	ck_assert_str_eq(str, long_str);
}
END_TEST

START_TEST(test_null_terminated)
{
	// verify interned strings are null-terminated
	w_string_table_id id = w_string_table_intern_strn(&g_table, "ABCDEFGH", 4);
	char *str = w_string_table_lookup(&g_table, id);
	ck_assert_int_eq(strlen(str), 4);
	ck_assert_str_eq(str, "ABCD");
}
END_TEST

START_TEST(test_different_pointer_same_content)
{
	// two different pointers with same content should return same ID
	char buf1[32];
	char buf2[32];
	strcpy(buf1, "same_content");
	strcpy(buf2, "same_content");
	ck_assert_ptr_ne(buf1, buf2);
	w_string_table_id id1 = w_string_table_intern_str(&g_table, buf1);
	w_string_table_id id2 = w_string_table_intern_str(&g_table, buf2);
	ck_assert_int_eq(id1, id2);
	ck_assert_int_eq(g_table.entries_length, 1);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_string_table_suite(void)
{
	Suite *s = suite_create("whisker_string_table");

	TCase *tc_init = tcase_create("string_table_init");
	tcase_add_checked_fixture(tc_init, string_table_setup, string_table_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_arena_stored);
	tcase_add_test(tc_init, test_init_entries_null_at_start);
	tcase_add_test(tc_init, test_init_realloc_block_size_stored);
	tcase_add_test(tc_init, test_init_hashmap_initialized);
	suite_add_tcase(s, tc_init);

	TCase *tc_intern_str = tcase_create("string_table_intern_str");
	tcase_add_checked_fixture(tc_intern_str, string_table_setup, string_table_teardown);
	tcase_set_timeout(tc_intern_str, 10);
	tcase_add_test(tc_intern_str, test_intern_str_returns_zero_first);
	tcase_add_test(tc_intern_str, test_intern_str_increments_id);
	tcase_add_test(tc_intern_str, test_intern_str_duplicate_returns_same_id);
	tcase_add_test(tc_intern_str, test_intern_str_duplicate_no_new_entry);
	tcase_add_test(tc_intern_str, test_intern_str_multiple_strings);
	tcase_add_test(tc_intern_str, test_intern_str_empty_string);
	tcase_add_test(tc_intern_str, test_intern_str_similar_strings);
	suite_add_tcase(s, tc_intern_str);

	TCase *tc_intern_strn = tcase_create("string_table_intern_strn");
	tcase_add_checked_fixture(tc_intern_strn, string_table_setup, string_table_teardown);
	tcase_set_timeout(tc_intern_strn, 10);
	tcase_add_test(tc_intern_strn, test_intern_strn_with_length);
	tcase_add_test(tc_intern_strn, test_intern_strn_partial_match);
	tcase_add_test(tc_intern_strn, test_intern_strn_duplicate_detection);
	tcase_add_test(tc_intern_strn, test_intern_strn_zero_length);
	tcase_add_test(tc_intern_strn, test_intern_strn_full_length);
	suite_add_tcase(s, tc_intern_strn);

	TCase *tc_lookup = tcase_create("string_table_lookup");
	tcase_add_checked_fixture(tc_lookup, string_table_setup, string_table_teardown);
	tcase_set_timeout(tc_lookup, 10);
	tcase_add_test(tc_lookup, test_lookup_returns_string);
	tcase_add_test(tc_lookup, test_lookup_invalid_id);
	tcase_add_test(tc_lookup, test_lookup_out_of_range_id);
	tcase_add_test(tc_lookup, test_lookup_multiple_strings);
	tcase_add_test(tc_lookup, test_lookup_string_is_copy);
	suite_add_tcase(s, tc_lookup);

	TCase *tc_lookup_entry = tcase_create("string_table_lookup_entry");
	tcase_add_checked_fixture(tc_lookup_entry, string_table_setup, string_table_teardown);
	tcase_set_timeout(tc_lookup_entry, 10);
	tcase_add_test(tc_lookup_entry, test_lookup_entry_returns_entry);
	tcase_add_test(tc_lookup_entry, test_lookup_entry_invalid_id);
	tcase_add_test(tc_lookup_entry, test_lookup_entry_out_of_range);
	tcase_add_test(tc_lookup_entry, test_lookup_entry_length_correct);
	suite_add_tcase(s, tc_lookup_entry);

	TCase *tc_lookup_entry_str = tcase_create("string_table_lookup_entry_str");
	tcase_add_checked_fixture(tc_lookup_entry_str, string_table_setup, string_table_teardown);
	tcase_set_timeout(tc_lookup_entry_str, 10);
	tcase_add_test(tc_lookup_entry_str, test_lookup_entry_str_found);
	tcase_add_test(tc_lookup_entry_str, test_lookup_entry_str_not_found);
	tcase_add_test(tc_lookup_entry_str, test_lookup_entry_str_empty_table);
	tcase_add_test(tc_lookup_entry_str, test_lookup_entry_str_multiple);
	suite_add_tcase(s, tc_lookup_entry_str);

	TCase *tc_free = tcase_create("string_table_free");
	tcase_set_timeout(tc_free, 10);
	tcase_add_test(tc_free, test_free_empty_table);
	tcase_add_test(tc_free, test_free_with_entries);
	suite_add_tcase(s, tc_free);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_add_checked_fixture(tc_edge, string_table_setup, string_table_teardown);
	tcase_set_timeout(tc_edge, 10);
	tcase_add_test(tc_edge, test_many_strings);
	tcase_add_test(tc_edge, test_long_strings);
	tcase_add_test(tc_edge, test_null_terminated);
	tcase_add_test(tc_edge, test_different_pointer_same_content);
	suite_add_tcase(s, tc_edge);

	return s;
}

int main(void)
{
	Suite *s = whisker_string_table_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
