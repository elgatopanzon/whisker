/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_command_buffer
 * @created     : Thursday Mar 05, 2026 15:09:51 CST
 * @description : tests for whisker_command_buffer
 */

#include "whisker_std.h"

#include "whisker_command_buffer.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

// test fixtures
static struct w_command_buffer buf;

static void whisker_command_buffer_setup()
{
	w_command_buffer_init(&buf);
}
static void whisker_command_buffer_teardown()
{
	w_command_buffer_free(&buf);
}

// basic tests

START_TEST(test_command_buffer_init_free_nulls_pointers)
{
	// init already called by fixture; free and verify nulls
	w_command_buffer_free(&buf);
	ck_assert_ptr_null(buf.commands);
	ck_assert_ptr_null(buf.payload_data);
	// re-init so teardown doesn't double-free
	w_command_buffer_init(&buf);
}
END_TEST

static void cmd_noop(void *ctx, void *payload)
{
	(void)ctx;
	(void)payload;
}

static void cmd_set_int(void *ctx, void *payload)
{
	int *out = (int *)ctx;
	*out = *(int *)payload;
}

START_TEST(test_command_buffer_queue_single)
{
	int val = 42;
	w_command_buffer_queue(&buf, cmd_set_int, NULL, &val, sizeof(int));
	ck_assert_int_eq(buf.commands_length, 1);
}
END_TEST

START_TEST(test_command_buffer_flush_executes_command)
{
	int result = 0;
	int val = 99;
	w_command_buffer_queue(&buf, cmd_set_int, &result, &val, sizeof(int));
	w_command_buffer_flush(&buf);
	ck_assert_int_eq(result, 99);
}
END_TEST

START_TEST(test_command_buffer_flush_clears_buffer)
{
	int val = 1;
	w_command_buffer_queue(&buf, cmd_noop, NULL, &val, sizeof(int));
	w_command_buffer_flush(&buf);
	ck_assert_int_eq(buf.commands_length, 0);
}
END_TEST

// multi-command tests

#define ORDER_TRACK_MAX 8
static int order_track[ORDER_TRACK_MAX];
static int order_idx;

static void cmd_record_order(void *ctx, void *payload)
{
	(void)ctx;
	int id = *(int *)payload;
	if (order_idx < ORDER_TRACK_MAX)
		order_track[order_idx++] = id;
}

START_TEST(test_command_buffer_queue_multiple)
{
	int v0 = 0, v1 = 1, v2 = 2;
	w_command_buffer_queue(&buf, cmd_noop, NULL, &v0, sizeof(int));
	w_command_buffer_queue(&buf, cmd_noop, NULL, &v1, sizeof(int));
	w_command_buffer_queue(&buf, cmd_noop, NULL, &v2, sizeof(int));
	ck_assert_int_eq(buf.commands_length, 3);
}
END_TEST

static void whisker_command_buffer_order_setup()
{
	w_command_buffer_init(&buf);
	order_idx = 0;
	for (int i = 0; i < ORDER_TRACK_MAX; i++)
		order_track[i] = -1;
}

START_TEST(test_command_buffer_flush_preserves_order)
{
	for (int i = 0; i < 5; i++)
		w_command_buffer_queue(&buf, cmd_record_order, NULL, &i, sizeof(int));
	w_command_buffer_flush(&buf);
	for (int i = 0; i < 5; i++)
		ck_assert_int_eq(order_track[i], i);
}
END_TEST

// stress tests

START_TEST(test_command_buffer_stress_large_count)
{
	int dummy = 0;
	int val = 7;
	for (int i = 0; i < 1000; i++)
		w_command_buffer_queue(&buf, cmd_set_int, &dummy, &val, sizeof(int));
	ck_assert_int_eq(buf.commands_length, 1000);
	w_command_buffer_flush(&buf);
	ck_assert_int_eq(dummy, 7);
	ck_assert_int_eq(buf.commands_length, 0);
}
END_TEST

START_TEST(test_command_buffer_stress_varying_payload_sizes)
{
	// payloads of 1 to 256 bytes
	unsigned char payloads[256];
	for (int i = 0; i < 256; i++)
		payloads[i] = (unsigned char)i;

	// queue commands with varying sizes (1..256)
	for (int sz = 1; sz <= 256; sz++)
		w_command_buffer_queue(&buf, cmd_noop, NULL, payloads, sz);

	ck_assert_int_eq(buf.commands_length, 256);
	// flush must not crash
	w_command_buffer_flush(&buf);
	ck_assert_int_eq(buf.commands_length, 0);
}
END_TEST

// test suite
Suite* whisker_command_buffer_suite(void)
{
	Suite *s = suite_create("whisker_command_buffer");

	// basic tcase
	TCase *tc_basic = tcase_create("basic");
	tcase_add_checked_fixture(tc_basic, whisker_command_buffer_setup, whisker_command_buffer_teardown);
	tcase_set_timeout(tc_basic, 10);
	tcase_add_test(tc_basic, test_command_buffer_init_free_nulls_pointers);
	tcase_add_test(tc_basic, test_command_buffer_queue_single);
	tcase_add_test(tc_basic, test_command_buffer_flush_executes_command);
	tcase_add_test(tc_basic, test_command_buffer_flush_clears_buffer);
	suite_add_tcase(s, tc_basic);

	// multi-command tcase
	TCase *tc_multi = tcase_create("multi");
	tcase_add_checked_fixture(tc_multi, whisker_command_buffer_order_setup, whisker_command_buffer_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_command_buffer_queue_multiple);
	tcase_add_test(tc_multi, test_command_buffer_flush_preserves_order);
	suite_add_tcase(s, tc_multi);

	// stress tcase
	TCase *tc_stress = tcase_create("stress");
	tcase_add_checked_fixture(tc_stress, whisker_command_buffer_setup, whisker_command_buffer_teardown);
	tcase_set_timeout(tc_stress, 10);
	tcase_add_test(tc_stress, test_command_buffer_stress_large_count);
	tcase_add_test(tc_stress, test_command_buffer_stress_varying_payload_sizes);
	suite_add_tcase(s, tc_stress);

	return s;
}

// test runner
int main()
{
	Suite *s = whisker_command_buffer_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
