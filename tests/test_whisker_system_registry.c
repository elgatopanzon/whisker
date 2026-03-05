/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_system_registry
 * @created     : Wednesday Mar 04, 2026 20:12:11 CST
 * @description : tests for whisker_system_registry.h system registry
 */

#include "whisker_std.h"
#include "whisker_system_registry.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_system_registry g_registry;

static void system_registry_setup(void)
{
	w_system_registry_init(&g_registry);
}

static void system_registry_teardown(void)
{
	w_system_registry_free(&g_registry);
}


/*****************************
*  test update function      *
*****************************/

static int g_update_call_count;
static double g_last_delta_time;

static void test_update_func(void *ctx, double delta_time)
{
	(void)ctx;
	g_update_call_count++;
	g_last_delta_time = delta_time;
}


/*****************************
*  init and free             *
*****************************/

START_TEST(test_init_systems_empty)
{
	ck_assert_int_eq(g_registry.systems_length, 0);
}
END_TEST

START_TEST(test_init_systems_allocated)
{
	ck_assert_ptr_nonnull(g_registry.systems);
}
END_TEST

START_TEST(test_free_nulls_pointers)
{
	struct w_system_registry reg;
	w_system_registry_init(&reg);
	w_system_registry_free(&reg);
	ck_assert_ptr_null(reg.systems);
	ck_assert_int_eq(reg.systems_length, 0);
}
END_TEST

START_TEST(test_free_empty_registry)
{
	struct w_system_registry reg;
	w_system_registry_init(&reg);
	// should not crash on empty registry
	w_system_registry_free(&reg);
}
END_TEST


/*****************************
*  system register           *
*****************************/

START_TEST(test_register_returns_id_zero)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	ck_assert_int_eq(id, 0);
}
END_TEST

START_TEST(test_register_increments_ids)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	size_t id1 = w_system_register_system(&g_registry, &sys);
	size_t id2 = w_system_register_system(&g_registry, &sys);
	size_t id3 = w_system_register_system(&g_registry, &sys);
	ck_assert_int_eq(id1, 0);
	ck_assert_int_eq(id2, 1);
	ck_assert_int_eq(id3, 2);
}
END_TEST

START_TEST(test_register_increments_length)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	ck_assert_int_eq(g_registry.systems_length, 0);
	w_system_register_system(&g_registry, &sys);
	ck_assert_int_eq(g_registry.systems_length, 1);
	w_system_register_system(&g_registry, &sys);
	ck_assert_int_eq(g_registry.systems_length, 2);
}
END_TEST

START_TEST(test_register_sets_enabled_true)
{
	struct w_system sys = {.phase_id = 0, .enabled = false, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert(got->enabled);
}
END_TEST

START_TEST(test_register_resets_last_update_ticks)
{
	struct w_system sys = {.phase_id = 0, .last_update_ticks = 12345, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert_int_eq(got->last_update_ticks, 0);
}
END_TEST

START_TEST(test_register_preserves_phase_id)
{
	struct w_system sys = {.phase_id = 42, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert_int_eq(got->phase_id, 42);
}
END_TEST

START_TEST(test_register_preserves_update_function)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert_ptr_eq(got->update, test_update_func);
}
END_TEST

START_TEST(test_register_preserves_update_frequency)
{
	struct w_system sys = {.phase_id = 0, .update_frequency = 60, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert_int_eq(got->update_frequency, 60);
}
END_TEST


/*****************************
*  system get                *
*****************************/

START_TEST(test_get_returns_registered)
{
	struct w_system sys = {.phase_id = 99, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(got->phase_id, 99);
}
END_TEST

START_TEST(test_get_invalid_returns_null)
{
	struct w_system *got = w_system_get_system_entry(&g_registry, 999);
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_get_empty_registry_returns_null)
{
	struct w_system *got = w_system_get_system_entry(&g_registry, 0);
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_get_boundary_valid)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	w_system_register_system(&g_registry, &sys);
	w_system_register_system(&g_registry, &sys);
	w_system_register_system(&g_registry, &sys);

	// ID 2 is valid (last element)
	struct w_system *got = w_system_get_system_entry(&g_registry, 2);
	ck_assert_ptr_nonnull(got);
}
END_TEST

START_TEST(test_get_boundary_invalid)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	w_system_register_system(&g_registry, &sys);
	w_system_register_system(&g_registry, &sys);
	w_system_register_system(&g_registry, &sys);

	// ID 3 is invalid (one past last)
	struct w_system *got = w_system_get_system_entry(&g_registry, 3);
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_get_returns_correct_system)
{
	struct w_system sys1 = {.phase_id = 10, .update = test_update_func};
	struct w_system sys2 = {.phase_id = 20, .update = test_update_func};
	struct w_system sys3 = {.phase_id = 30, .update = test_update_func};

	size_t id1 = w_system_register_system(&g_registry, &sys1);
	size_t id2 = w_system_register_system(&g_registry, &sys2);
	size_t id3 = w_system_register_system(&g_registry, &sys3);

	ck_assert_int_eq(w_system_get_system_entry(&g_registry, id1)->phase_id, 10);
	ck_assert_int_eq(w_system_get_system_entry(&g_registry, id2)->phase_id, 20);
	ck_assert_int_eq(w_system_get_system_entry(&g_registry, id3)->phase_id, 30);
}
END_TEST


/*****************************
*  system set_state          *
*****************************/

START_TEST(test_set_state_disable)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert(got->enabled);

	w_system_set_system_state(&g_registry, id, false);
	ck_assert(!got->enabled);
}
END_TEST

START_TEST(test_set_state_enable)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);

	// first disable
	w_system_set_system_state(&g_registry, id, false);
	ck_assert(!got->enabled);

	// then enable
	w_system_set_system_state(&g_registry, id, true);
	ck_assert(got->enabled);
}
END_TEST

START_TEST(test_set_state_invalid_noop)
{
	// should not crash on invalid ID
	w_system_set_system_state(&g_registry, 999, true);
	w_system_set_system_state(&g_registry, 999, false);
}
END_TEST

START_TEST(test_set_state_empty_registry_noop)
{
	// should not crash on empty registry
	w_system_set_system_state(&g_registry, 0, true);
}
END_TEST

START_TEST(test_set_state_multiple_systems)
{
	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	size_t id1 = w_system_register_system(&g_registry, &sys);
	size_t id2 = w_system_register_system(&g_registry, &sys);
	size_t id3 = w_system_register_system(&g_registry, &sys);

	// all start enabled
	ck_assert(w_system_get_system_entry(&g_registry, id1)->enabled);
	ck_assert(w_system_get_system_entry(&g_registry, id2)->enabled);
	ck_assert(w_system_get_system_entry(&g_registry, id3)->enabled);

	// disable id2 only
	w_system_set_system_state(&g_registry, id2, false);
	ck_assert(w_system_get_system_entry(&g_registry, id1)->enabled);
	ck_assert(!w_system_get_system_entry(&g_registry, id2)->enabled);
	ck_assert(w_system_get_system_entry(&g_registry, id3)->enabled);
}
END_TEST


/*****************************
*  edge cases                *
*****************************/

START_TEST(test_many_systems_stress)
{
	const int count = 100;
	size_t system_ids[100];

	for (int i = 0; i < count; i++) {
		struct w_system sys = {.phase_id = (size_t)i, .update = test_update_func};
		system_ids[i] = w_system_register_system(&g_registry, &sys);
	}

	ck_assert_int_eq(g_registry.systems_length, count);

	// verify all systems retrievable with correct data
	for (int i = 0; i < count; i++) {
		struct w_system *s = w_system_get_system_entry(&g_registry, system_ids[i]);
		ck_assert_ptr_nonnull(s);
		ck_assert_int_eq(s->phase_id, (size_t)i);
	}
}
END_TEST

START_TEST(test_systems_beyond_initial_allocation)
{
	// initial allocation is 16, register more than that
	const int count = 32;

	for (int i = 0; i < count; i++) {
		struct w_system sys = {.phase_id = (size_t)i, .update = test_update_func};
		w_system_register_system(&g_registry, &sys);
	}

	ck_assert_int_eq(g_registry.systems_length, count);

	// verify all systems valid
	for (int i = 0; i < count; i++) {
		struct w_system *s = w_system_get_system_entry(&g_registry, i);
		ck_assert_ptr_nonnull(s);
		ck_assert_int_eq(s->phase_id, (size_t)i);
	}
}
END_TEST

START_TEST(test_system_update_function_callable)
{
	g_update_call_count = 0;
	g_last_delta_time = 0.0;

	struct w_system sys = {.phase_id = 0, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);

	// call the update function through the pointer
	got->update(NULL, 0.016);

	ck_assert_int_eq(g_update_call_count, 1);
	ck_assert_double_eq_tol(g_last_delta_time, 0.016, 0.0001);
}
END_TEST

START_TEST(test_system_null_update_function)
{
	struct w_system sys = {.phase_id = 0, .update = NULL};
	size_t id = w_system_register_system(&g_registry, &sys);
	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert_ptr_null(got->update);
}
END_TEST

START_TEST(test_modify_system_via_pointer)
{
	struct w_system sys = {.phase_id = 0, .update_frequency = 30, .update = test_update_func};
	size_t id = w_system_register_system(&g_registry, &sys);

	struct w_system *got = w_system_get_system_entry(&g_registry, id);
	ck_assert_int_eq(got->update_frequency, 30);

	// modify through pointer
	got->update_frequency = 60;
	got->last_update_ticks = 1000;

	// verify changes persist
	struct w_system *got2 = w_system_get_system_entry(&g_registry, id);
	ck_assert_int_eq(got2->update_frequency, 60);
	ck_assert_int_eq(got2->last_update_ticks, 1000);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_system_registry_suite(void)
{
	Suite *s = suite_create("whisker_system_registry");

	TCase *tc_init = tcase_create("init_free");
	tcase_add_checked_fixture(tc_init, system_registry_setup, system_registry_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_systems_empty);
	tcase_add_test(tc_init, test_init_systems_allocated);
	tcase_add_test(tc_init, test_free_nulls_pointers);
	tcase_add_test(tc_init, test_free_empty_registry);
	suite_add_tcase(s, tc_init);

	TCase *tc_register = tcase_create("system_register");
	tcase_add_checked_fixture(tc_register, system_registry_setup, system_registry_teardown);
	tcase_set_timeout(tc_register, 10);
	tcase_add_test(tc_register, test_register_returns_id_zero);
	tcase_add_test(tc_register, test_register_increments_ids);
	tcase_add_test(tc_register, test_register_increments_length);
	tcase_add_test(tc_register, test_register_sets_enabled_true);
	tcase_add_test(tc_register, test_register_resets_last_update_ticks);
	tcase_add_test(tc_register, test_register_preserves_phase_id);
	tcase_add_test(tc_register, test_register_preserves_update_function);
	tcase_add_test(tc_register, test_register_preserves_update_frequency);
	suite_add_tcase(s, tc_register);

	TCase *tc_get = tcase_create("system_get");
	tcase_add_checked_fixture(tc_get, system_registry_setup, system_registry_teardown);
	tcase_set_timeout(tc_get, 10);
	tcase_add_test(tc_get, test_get_returns_registered);
	tcase_add_test(tc_get, test_get_invalid_returns_null);
	tcase_add_test(tc_get, test_get_empty_registry_returns_null);
	tcase_add_test(tc_get, test_get_boundary_valid);
	tcase_add_test(tc_get, test_get_boundary_invalid);
	tcase_add_test(tc_get, test_get_returns_correct_system);
	suite_add_tcase(s, tc_get);

	TCase *tc_state = tcase_create("system_state");
	tcase_add_checked_fixture(tc_state, system_registry_setup, system_registry_teardown);
	tcase_set_timeout(tc_state, 10);
	tcase_add_test(tc_state, test_set_state_disable);
	tcase_add_test(tc_state, test_set_state_enable);
	tcase_add_test(tc_state, test_set_state_invalid_noop);
	tcase_add_test(tc_state, test_set_state_empty_registry_noop);
	tcase_add_test(tc_state, test_set_state_multiple_systems);
	suite_add_tcase(s, tc_state);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_add_checked_fixture(tc_edge, system_registry_setup, system_registry_teardown);
	tcase_set_timeout(tc_edge, 10);
	tcase_add_test(tc_edge, test_many_systems_stress);
	tcase_add_test(tc_edge, test_systems_beyond_initial_allocation);
	tcase_add_test(tc_edge, test_system_update_function_callable);
	tcase_add_test(tc_edge, test_system_null_update_function);
	tcase_add_test(tc_edge, test_modify_system_via_pointer);
	suite_add_tcase(s, tc_edge);

	return s;
}

int main(void)
{
	Suite *s = whisker_system_registry_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
