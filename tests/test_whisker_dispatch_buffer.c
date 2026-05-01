/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_dispatch_buffer
 * @created     : Friday May 01, 2026 00:27:23 CST
 * @description : tests for whisker_dispatch_buffer
 */

#include "whisker_std.h"

#include "whisker_dispatch_buffer.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

// test fixtures
static struct w_dispatch_buffer buf;

static void whisker_dispatch_buffer_setup()
{
	w_dispatch_buffer_init(&buf);
}
static void whisker_dispatch_buffer_teardown()
{
	w_dispatch_buffer_free(&buf);
}

// basic tests

START_TEST(test_dispatch_buffer_init_free_nulls_pointers)
{
	// init already called by fixture; free and verify nulls
	w_dispatch_buffer_free(&buf);
	ck_assert_ptr_null(buf.entries);
	ck_assert_ptr_null(buf.payload_data);
	ck_assert_int_eq(buf.read_index, 0);
	// re-init so teardown doesn't double-free
	w_dispatch_buffer_init(&buf);
}
END_TEST

START_TEST(test_dispatch_buffer_push_single)
{
	int val = 42;
	w_dispatch_buffer_push(&buf, 1, &val, sizeof(int));
	ck_assert_int_eq(buf.entries_length, 1);
	ck_assert(w_dispatch_buffer_has_entries(&buf));
	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 1);
}
END_TEST

START_TEST(test_dispatch_buffer_pop_returns_entry)
{
	int val = 99;
	w_dispatch_buffer_push(&buf, 5, &val, sizeof(int));

	void *payload = NULL;
	struct w_dispatch_entry *entry = w_dispatch_buffer_pop(&buf, &payload);

	ck_assert_ptr_nonnull(entry);
	ck_assert_int_eq(entry->type_id, 5);
	ck_assert_ptr_nonnull(payload);
	ck_assert_int_eq(*(int *)payload, 99);
}
END_TEST

START_TEST(test_dispatch_buffer_pop_advances_read_index)
{
	int v1 = 1, v2 = 2;
	w_dispatch_buffer_push(&buf, 10, &v1, sizeof(int));
	w_dispatch_buffer_push(&buf, 20, &v2, sizeof(int));

	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 2);

	void *payload = NULL;
	struct w_dispatch_entry *entry = w_dispatch_buffer_pop(&buf, &payload);
	ck_assert_int_eq(entry->type_id, 10);
	ck_assert_int_eq(*(int *)payload, 1);
	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 1);

	entry = w_dispatch_buffer_pop(&buf, &payload);
	ck_assert_int_eq(entry->type_id, 20);
	ck_assert_int_eq(*(int *)payload, 2);
	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 0);

	// pop on empty returns NULL
	entry = w_dispatch_buffer_pop(&buf, &payload);
	ck_assert_ptr_null(entry);
}
END_TEST

START_TEST(test_dispatch_buffer_pop_null_payload)
{
	int val = 123;
	w_dispatch_buffer_push(&buf, 7, &val, sizeof(int));

	// pass NULL for payload - should not crash
	struct w_dispatch_entry *entry = w_dispatch_buffer_pop(&buf, NULL);
	ck_assert_ptr_nonnull(entry);
	ck_assert_int_eq(entry->type_id, 7);
}
END_TEST

START_TEST(test_dispatch_buffer_clear_resets_buffer)
{
	int val = 1;
	w_dispatch_buffer_push(&buf, 1, &val, sizeof(int));
	w_dispatch_buffer_push(&buf, 2, &val, sizeof(int));

	w_dispatch_buffer_clear(&buf);

	ck_assert_int_eq(buf.entries_length, 0);
	ck_assert_int_eq(buf.payload_data_length, 0);
	ck_assert_int_eq(buf.read_index, 0);
	ck_assert(!w_dispatch_buffer_has_entries(&buf));
}
END_TEST

// peek tests

START_TEST(test_dispatch_buffer_peek_does_not_advance)
{
	int v1 = 10, v2 = 20;
	w_dispatch_buffer_push(&buf, 1, &v1, sizeof(int));
	w_dispatch_buffer_push(&buf, 2, &v2, sizeof(int));

	void *payload = NULL;
	struct w_dispatch_entry *entry = w_dispatch_buffer_peek(&buf, 0, &payload);
	ck_assert_int_eq(entry->type_id, 1);
	ck_assert_int_eq(*(int *)payload, 10);

	// read_index unchanged
	ck_assert_int_eq(buf.read_index, 0);
	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 2);

	// peek second entry
	entry = w_dispatch_buffer_peek(&buf, 1, &payload);
	ck_assert_int_eq(entry->type_id, 2);
	ck_assert_int_eq(*(int *)payload, 20);

	// still unchanged
	ck_assert_int_eq(buf.read_index, 0);
}
END_TEST

START_TEST(test_dispatch_buffer_peek_out_of_range)
{
	int val = 1;
	w_dispatch_buffer_push(&buf, 1, &val, sizeof(int));

	void *payload = NULL;
	struct w_dispatch_entry *entry = w_dispatch_buffer_peek(&buf, 5, &payload);
	ck_assert_ptr_null(entry);
}
END_TEST

// multi-entry tests

START_TEST(test_dispatch_buffer_push_multiple)
{
	int v0 = 0, v1 = 1, v2 = 2;
	w_dispatch_buffer_push(&buf, 100, &v0, sizeof(int));
	w_dispatch_buffer_push(&buf, 200, &v1, sizeof(int));
	w_dispatch_buffer_push(&buf, 300, &v2, sizeof(int));
	ck_assert_int_eq(buf.entries_length, 3);
	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 3);
}
END_TEST

START_TEST(test_dispatch_buffer_preserves_order)
{
	for (int i = 0; i < 5; i++)
		w_dispatch_buffer_push(&buf, i * 10, &i, sizeof(int));

	for (int i = 0; i < 5; i++)
	{
		void *payload = NULL;
		struct w_dispatch_entry *entry = w_dispatch_buffer_pop(&buf, &payload);
		ck_assert_int_eq(entry->type_id, i * 10);
		ck_assert_int_eq(*(int *)payload, i);
	}
}
END_TEST

// stress tests

START_TEST(test_dispatch_buffer_stress_large_count)
{
	int val = 7;
	for (int i = 0; i < 1000; i++)
		w_dispatch_buffer_push(&buf, i, &val, sizeof(int));

	ck_assert_int_eq(buf.entries_length, 1000);
	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 1000);

	// pop all
	for (int i = 0; i < 1000; i++)
	{
		void *payload = NULL;
		struct w_dispatch_entry *entry = w_dispatch_buffer_pop(&buf, &payload);
		ck_assert_ptr_nonnull(entry);
		ck_assert_int_eq(entry->type_id, i);
		ck_assert_int_eq(*(int *)payload, 7);
	}

	ck_assert(!w_dispatch_buffer_has_entries(&buf));
	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 0);
}
END_TEST

START_TEST(test_dispatch_buffer_stress_varying_payload_sizes)
{
	// payloads of 1 to 256 bytes
	unsigned char payloads[256];
	for (int i = 0; i < 256; i++)
		payloads[i] = (unsigned char)i;

	// push entries with varying sizes (1..256)
	for (int sz = 1; sz <= 256; sz++)
		w_dispatch_buffer_push(&buf, sz, payloads, sz);

	ck_assert_int_eq(buf.entries_length, 256);

	// pop must not crash and payload sizes must match
	for (int sz = 1; sz <= 256; sz++)
	{
		void *payload = NULL;
		struct w_dispatch_entry *entry = w_dispatch_buffer_pop(&buf, &payload);
		ck_assert_ptr_nonnull(entry);
		ck_assert_int_eq(entry->type_id, sz);
		ck_assert_int_eq(entry->payload_size, (size_t)sz);
	}

	ck_assert_int_eq(w_dispatch_buffer_count(&buf), 0);
}
END_TEST

// test suite
Suite* whisker_dispatch_buffer_suite(void)
{
	Suite *s = suite_create("whisker_dispatch_buffer");

	// basic tcase
	TCase *tc_basic = tcase_create("basic");
	tcase_add_checked_fixture(tc_basic, whisker_dispatch_buffer_setup, whisker_dispatch_buffer_teardown);
	tcase_set_timeout(tc_basic, 10);
	tcase_add_test(tc_basic, test_dispatch_buffer_init_free_nulls_pointers);
	tcase_add_test(tc_basic, test_dispatch_buffer_push_single);
	tcase_add_test(tc_basic, test_dispatch_buffer_pop_returns_entry);
	tcase_add_test(tc_basic, test_dispatch_buffer_pop_advances_read_index);
	tcase_add_test(tc_basic, test_dispatch_buffer_pop_null_payload);
	tcase_add_test(tc_basic, test_dispatch_buffer_clear_resets_buffer);
	suite_add_tcase(s, tc_basic);

	// peek tcase
	TCase *tc_peek = tcase_create("peek");
	tcase_add_checked_fixture(tc_peek, whisker_dispatch_buffer_setup, whisker_dispatch_buffer_teardown);
	tcase_set_timeout(tc_peek, 10);
	tcase_add_test(tc_peek, test_dispatch_buffer_peek_does_not_advance);
	tcase_add_test(tc_peek, test_dispatch_buffer_peek_out_of_range);
	suite_add_tcase(s, tc_peek);

	// multi-entry tcase
	TCase *tc_multi = tcase_create("multi");
	tcase_add_checked_fixture(tc_multi, whisker_dispatch_buffer_setup, whisker_dispatch_buffer_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_dispatch_buffer_push_multiple);
	tcase_add_test(tc_multi, test_dispatch_buffer_preserves_order);
	suite_add_tcase(s, tc_multi);

	// stress tcase
	TCase *tc_stress = tcase_create("stress");
	tcase_add_checked_fixture(tc_stress, whisker_dispatch_buffer_setup, whisker_dispatch_buffer_teardown);
	tcase_set_timeout(tc_stress, 10);
	tcase_add_test(tc_stress, test_dispatch_buffer_stress_large_count);
	tcase_add_test(tc_stress, test_dispatch_buffer_stress_varying_payload_sizes);
	suite_add_tcase(s, tc_stress);

	return s;
}

// test runner
int main()
{
	Suite *s = whisker_dispatch_buffer_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
