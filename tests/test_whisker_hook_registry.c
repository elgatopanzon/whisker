/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_hook_registry
 * @created     : Thursday Mar 05, 2026 16:22:00 CST
 * @description : tests for whisker_hook_registry
 */

#include "whisker_std.h"

#include <check.h>
#include "whisker_hook_registry.h"

// shared test state
static struct w_hook_registry registry;
static int call_count;
static void *last_ctx;
static void *last_data;

static void hook_registry_setup()
{
	w_hook_registry_init(&registry);
	call_count = 0;
	last_ctx = NULL;
	last_data = NULL;
}

static void hook_registry_teardown()
{
	w_hook_registry_free(&registry);
}

// hook functions for testing
static void hook_fn_counter(void *ctx, void *data)
{
	(void)ctx;
	(void)data;
	call_count++;
}

static void hook_fn_record(void *ctx, void *data)
{
	last_ctx = ctx;
	last_data = data;
}

// ---- register tcase ----

START_TEST(test_register_returns_hook_id)
{
	size_t id = w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	ck_assert_int_eq(id, 0);
}
END_TEST

START_TEST(test_register_multiple_same_group_returns_sequential_ids)
{
	size_t id0 = w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	size_t id1 = w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	size_t id2 = w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	ck_assert_int_eq(id0, 0);
	ck_assert_int_eq(id1, 1);
	ck_assert_int_eq(id2, 2);
}
END_TEST

START_TEST(test_register_updates_groups_length)
{
	w_hook_registry_register_hook(&registry, 3, hook_fn_counter);
	ck_assert_int_eq(registry.hook_groups_length, 4);
}
END_TEST

START_TEST(test_register_length_not_decreased_on_lower_group)
{
	w_hook_registry_register_hook(&registry, 5, hook_fn_counter);
	w_hook_registry_register_hook(&registry, 2, hook_fn_counter);
	ck_assert_int_eq(registry.hook_groups_length, 6);
}
END_TEST

START_TEST(test_register_hook_enabled_by_default)
{
	size_t id = w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	struct w_hook_entry *entry = w_hook_registry_get_hook_entry(&registry, 0, id);
	ck_assert_ptr_nonnull(entry);
	ck_assert_int_eq(entry->enabled, true);
}
END_TEST

START_TEST(test_register_hook_fn_stored)
{
	size_t id = w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	struct w_hook_entry *entry = w_hook_registry_get_hook_entry(&registry, 0, id);
	ck_assert_ptr_nonnull(entry);
	ck_assert_ptr_eq(entry->hook_fn, hook_fn_counter);
}
END_TEST

START_TEST(test_register_gap_group_initializes_intermediate)
{
	// register to group 3, then register to group 1
	// group 1 should start with hooks_length = 0 (no hooks from gap)
	w_hook_registry_register_hook(&registry, 3, hook_fn_counter);
	ck_assert_int_eq(registry.hook_groups[1].hooks_length, 0);
	ck_assert_ptr_null(registry.hook_groups[1].hooks);
}
END_TEST

// ---- get_hook_entry tcase ----

START_TEST(test_get_hook_entry_valid)
{
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	struct w_hook_entry *entry = w_hook_registry_get_hook_entry(&registry, 0, 0);
	ck_assert_ptr_nonnull(entry);
}
END_TEST

START_TEST(test_get_hook_entry_out_of_bounds_group)
{
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	struct w_hook_entry *entry = w_hook_registry_get_hook_entry(&registry, 99, 0);
	ck_assert_ptr_null(entry);
}
END_TEST

START_TEST(test_get_hook_entry_out_of_bounds_hook_id)
{
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	struct w_hook_entry *entry = w_hook_registry_get_hook_entry(&registry, 0, 99);
	ck_assert_ptr_null(entry);
}
END_TEST

// ---- run_hooks tcase ----

START_TEST(test_run_hooks_calls_enabled_hooks)
{
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	w_hook_registry_run_hooks(&registry, 0, NULL, NULL);
	ck_assert_int_eq(call_count, 2);
}
END_TEST

START_TEST(test_run_hooks_skips_disabled_hooks)
{
	size_t id = w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	struct w_hook_entry *entry = w_hook_registry_get_hook_entry(&registry, 0, id);
	entry->enabled = false;
	w_hook_registry_run_hooks(&registry, 0, NULL, NULL);
	ck_assert_int_eq(call_count, 1);
}
END_TEST

START_TEST(test_run_hooks_passes_ctx_and_data)
{
	int ctx_val = 42;
	int data_val = 99;
	w_hook_registry_register_hook(&registry, 0, hook_fn_record);
	w_hook_registry_run_hooks(&registry, 0, &ctx_val, &data_val);
	ck_assert_ptr_eq(last_ctx, &ctx_val);
	ck_assert_ptr_eq(last_data, &data_val);
}
END_TEST

START_TEST(test_run_hooks_noop_on_empty_group)
{
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	w_hook_registry_run_hooks(&registry, 1, NULL, NULL);
	ck_assert_int_eq(call_count, 0);
}
END_TEST

START_TEST(test_run_hooks_noop_on_out_of_bounds_group)
{
	w_hook_registry_run_hooks(&registry, 99, NULL, NULL);
	ck_assert_int_eq(call_count, 0);
}
END_TEST

START_TEST(test_run_hooks_multiple_groups_isolated)
{
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	w_hook_registry_register_hook(&registry, 1, hook_fn_counter);
	w_hook_registry_register_hook(&registry, 1, hook_fn_counter);
	w_hook_registry_run_hooks(&registry, 1, NULL, NULL);
	ck_assert_int_eq(call_count, 2);
}
END_TEST

// ---- free tcase ----

START_TEST(test_free_cleans_up)
{
	w_hook_registry_register_hook(&registry, 0, hook_fn_counter);
	w_hook_registry_register_hook(&registry, 1, hook_fn_counter);
	w_hook_registry_free(&registry);
	ck_assert_ptr_null(registry.hook_groups);
	ck_assert_int_eq(registry.hook_groups_length, 0);
	// re-init so teardown doesn't double-free
	w_hook_registry_init(&registry);
}
END_TEST

// test suite
Suite *whisker_hook_registry_suite(void)
{
	Suite *s = suite_create("whisker_hook_registry");

	TCase *tc_register = tcase_create("register");
	tcase_add_checked_fixture(tc_register, hook_registry_setup, hook_registry_teardown);
	tcase_set_timeout(tc_register, 10);
	tcase_add_test(tc_register, test_register_returns_hook_id);
	tcase_add_test(tc_register, test_register_multiple_same_group_returns_sequential_ids);
	tcase_add_test(tc_register, test_register_updates_groups_length);
	tcase_add_test(tc_register, test_register_length_not_decreased_on_lower_group);
	tcase_add_test(tc_register, test_register_hook_enabled_by_default);
	tcase_add_test(tc_register, test_register_hook_fn_stored);
	tcase_add_test(tc_register, test_register_gap_group_initializes_intermediate);
	suite_add_tcase(s, tc_register);

	TCase *tc_get = tcase_create("get_hook_entry");
	tcase_add_checked_fixture(tc_get, hook_registry_setup, hook_registry_teardown);
	tcase_set_timeout(tc_get, 10);
	tcase_add_test(tc_get, test_get_hook_entry_valid);
	tcase_add_test(tc_get, test_get_hook_entry_out_of_bounds_group);
	tcase_add_test(tc_get, test_get_hook_entry_out_of_bounds_hook_id);
	suite_add_tcase(s, tc_get);

	TCase *tc_run = tcase_create("run_hooks");
	tcase_add_checked_fixture(tc_run, hook_registry_setup, hook_registry_teardown);
	tcase_set_timeout(tc_run, 10);
	tcase_add_test(tc_run, test_run_hooks_calls_enabled_hooks);
	tcase_add_test(tc_run, test_run_hooks_skips_disabled_hooks);
	tcase_add_test(tc_run, test_run_hooks_passes_ctx_and_data);
	tcase_add_test(tc_run, test_run_hooks_noop_on_empty_group);
	tcase_add_test(tc_run, test_run_hooks_noop_on_out_of_bounds_group);
	tcase_add_test(tc_run, test_run_hooks_multiple_groups_isolated);
	suite_add_tcase(s, tc_run);

	TCase *tc_free = tcase_create("free");
	tcase_add_checked_fixture(tc_free, hook_registry_setup, hook_registry_teardown);
	tcase_set_timeout(tc_free, 10);
	tcase_add_test(tc_free, test_free_cleans_up);
	suite_add_tcase(s, tc_free);

	return s;
}

int main()
{
	Suite *s = whisker_hook_registry_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
