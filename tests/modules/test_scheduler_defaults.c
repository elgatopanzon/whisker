/**
 * @author      : ElGatoPanzon
 * @file        : test_scheduler_defaults
 * @created     : Saturday Mar 07, 2026 11:52:28 CST
 * @description : tests for whisker_scheduler_defaults module
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "whisker_scheduler_defaults.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;

static void defaults_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_scheduler_defaults_init(&g_world, 60.0);
}

static void defaults_teardown(void)
{
	wm_scheduler_defaults_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  timestep count            *
*****************************/

START_TEST(test_timestep_count)
{
	ck_assert_int_eq(g_world.scheduler.time_steps_length, 7);
}
END_TEST


/*****************************
*  phase count               *
*****************************/

START_TEST(test_phase_count)
{
	ck_assert_int_eq(g_world.scheduler.phases_length, 20);
}
END_TEST


/*****************************
*  timestep IDs valid        *
*****************************/

START_TEST(test_timestep_ids_valid)
{
	ck_assert_ptr_nonnull(w_ecs_get_system_time_step(&g_world, WM_TIMESTEP_PRE));
	ck_assert_ptr_nonnull(w_ecs_get_system_time_step(&g_world, WM_TIMESTEP_DEFAULT));
	ck_assert_ptr_nonnull(w_ecs_get_system_time_step(&g_world, WM_TIMESTEP_DEFAULT_FIXED));
	ck_assert_ptr_nonnull(w_ecs_get_system_time_step(&g_world, WM_TIMESTEP_DEFAULT_POST));
	ck_assert_ptr_nonnull(w_ecs_get_system_time_step(&g_world, WM_TIMESTEP_DEFAULT_RENDER));
	ck_assert_ptr_nonnull(w_ecs_get_system_time_step(&g_world, WM_TIMESTEP_RESERVED));
	ck_assert_ptr_nonnull(w_ecs_get_system_time_step(&g_world, WM_TIMESTEP_POST));
}
END_TEST


/*****************************
*  phase IDs valid           *
*****************************/

START_TEST(test_phase_ids_valid)
{
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_STARTUP));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE_LOAD));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_LOAD));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_LOAD));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE_UPDATE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_UPDATE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_UPDATE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE_FIXED_UPDATE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_FIXED_UPDATE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_FIXED_UPDATE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_FINAL_FIXED));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_FIXED));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_FINAL));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE_RENDER));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_RENDER));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_RENDER));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_FINAL_RENDER));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_RESERVED));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_PHASE_POST));
}
END_TEST


/*****************************
*  timestep ordering         *
*****************************/

START_TEST(test_timestep_ordering)
{
	// verify timestep order: PRE, DEFAULT, FIXED, POST, RENDER, RESERVED, POST_FINAL
	size_t *order = g_world.scheduler.time_steps_order;
	ck_assert_int_eq(order[0], WM_TIMESTEP_PRE);
	ck_assert_int_eq(order[1], WM_TIMESTEP_DEFAULT);
	ck_assert_int_eq(order[2], WM_TIMESTEP_DEFAULT_FIXED);
	ck_assert_int_eq(order[3], WM_TIMESTEP_DEFAULT_POST);
	ck_assert_int_eq(order[4], WM_TIMESTEP_DEFAULT_RENDER);
	ck_assert_int_eq(order[5], WM_TIMESTEP_RESERVED);
	ck_assert_int_eq(order[6], WM_TIMESTEP_POST);
}
END_TEST


/*****************************
*  phase-timestep assignment *
*****************************/

START_TEST(test_phase_timestep_assignments)
{
	// PRE timestep phases
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE)->time_step_id, WM_TIMESTEP_PRE);

	// DEFAULT timestep phases
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_STARTUP)->time_step_id, WM_TIMESTEP_DEFAULT);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE_LOAD)->time_step_id, WM_TIMESTEP_DEFAULT);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_LOAD)->time_step_id, WM_TIMESTEP_DEFAULT);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_LOAD)->time_step_id, WM_TIMESTEP_DEFAULT);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE_UPDATE)->time_step_id, WM_TIMESTEP_DEFAULT);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_UPDATE)->time_step_id, WM_TIMESTEP_DEFAULT);

	// DEFAULT_POST timestep phases
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_UPDATE)->time_step_id, WM_TIMESTEP_DEFAULT_POST);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_FINAL)->time_step_id, WM_TIMESTEP_DEFAULT_POST);

	// DEFAULT_FIXED timestep phases
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE_FIXED_UPDATE)->time_step_id, WM_TIMESTEP_DEFAULT_FIXED);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_FIXED_UPDATE)->time_step_id, WM_TIMESTEP_DEFAULT_FIXED);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_FIXED_UPDATE)->time_step_id, WM_TIMESTEP_DEFAULT_FIXED);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_FINAL_FIXED)->time_step_id, WM_TIMESTEP_DEFAULT_FIXED);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_FIXED)->time_step_id, WM_TIMESTEP_DEFAULT_FIXED);

	// DEFAULT_RENDER timestep phases
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_PRE_RENDER)->time_step_id, WM_TIMESTEP_DEFAULT_RENDER);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_ON_RENDER)->time_step_id, WM_TIMESTEP_DEFAULT_RENDER);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_POST_RENDER)->time_step_id, WM_TIMESTEP_DEFAULT_RENDER);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_FINAL_RENDER)->time_step_id, WM_TIMESTEP_DEFAULT_RENDER);

	// RESERVED and POST timestep phases
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_RESERVED)->time_step_id, WM_TIMESTEP_RESERVED);
	ck_assert_int_eq(w_ecs_get_system_phase(&g_world, WM_PHASE_POST)->time_step_id, WM_TIMESTEP_POST);
}
END_TEST


/*****************************
*  phase ordering            *
*****************************/

START_TEST(test_phase_ordering_default_timestep)
{
	// within DEFAULT timestep: ON_STARTUP < PRE_LOAD < ON_LOAD < POST_LOAD < PRE_UPDATE < ON_UPDATE
	size_t *order = g_world.scheduler.phases_order;

	// find positions of DEFAULT timestep phases in the order array
	size_t pos_startup = 0, pos_pre_load = 0, pos_on_load = 0;
	size_t pos_post_load = 0, pos_pre_update = 0, pos_on_update = 0;

	for (size_t i = 0; i < g_world.scheduler.phases_order_length; i++) {
		if (order[i] == WM_PHASE_ON_STARTUP)  pos_startup = i;
		if (order[i] == WM_PHASE_PRE_LOAD)    pos_pre_load = i;
		if (order[i] == WM_PHASE_ON_LOAD)     pos_on_load = i;
		if (order[i] == WM_PHASE_POST_LOAD)   pos_post_load = i;
		if (order[i] == WM_PHASE_PRE_UPDATE)  pos_pre_update = i;
		if (order[i] == WM_PHASE_ON_UPDATE)   pos_on_update = i;
	}

	ck_assert_uint_lt(pos_startup, pos_pre_load);
	ck_assert_uint_lt(pos_pre_load, pos_on_load);
	ck_assert_uint_lt(pos_on_load, pos_post_load);
	ck_assert_uint_lt(pos_post_load, pos_pre_update);
	ck_assert_uint_lt(pos_pre_update, pos_on_update);
}
END_TEST

START_TEST(test_phase_ordering_fixed_timestep)
{
	// within FIXED timestep: PRE_FIXED < ON_FIXED < POST_FIXED_UPDATE < FINAL_FIXED < POST_FIXED
	size_t *order = g_world.scheduler.phases_order;

	size_t pos_pre = 0, pos_on = 0, pos_post = 0, pos_final = 0, pos_post_fixed = 0;

	for (size_t i = 0; i < g_world.scheduler.phases_order_length; i++) {
		if (order[i] == WM_PHASE_PRE_FIXED_UPDATE)  pos_pre = i;
		if (order[i] == WM_PHASE_ON_FIXED_UPDATE)   pos_on = i;
		if (order[i] == WM_PHASE_POST_FIXED_UPDATE) pos_post = i;
		if (order[i] == WM_PHASE_FINAL_FIXED)       pos_final = i;
		if (order[i] == WM_PHASE_POST_FIXED)        pos_post_fixed = i;
	}

	ck_assert_uint_lt(pos_pre, pos_on);
	ck_assert_uint_lt(pos_on, pos_post);
	ck_assert_uint_lt(pos_post, pos_final);
	ck_assert_uint_lt(pos_final, pos_post_fixed);
}
END_TEST

START_TEST(test_phase_ordering_render_timestep)
{
	// within RENDER timestep: PRE_RENDER < ON_RENDER < POST_RENDER < FINAL_RENDER
	size_t *order = g_world.scheduler.phases_order;

	size_t pos_pre = 0, pos_on = 0, pos_post = 0, pos_final = 0;

	for (size_t i = 0; i < g_world.scheduler.phases_order_length; i++) {
		if (order[i] == WM_PHASE_PRE_RENDER)  pos_pre = i;
		if (order[i] == WM_PHASE_ON_RENDER)   pos_on = i;
		if (order[i] == WM_PHASE_POST_RENDER) pos_post = i;
		if (order[i] == WM_PHASE_FINAL_RENDER) pos_final = i;
	}

	ck_assert_uint_lt(pos_pre, pos_on);
	ck_assert_uint_lt(pos_on, pos_post);
	ck_assert_uint_lt(pos_post, pos_final);
}
END_TEST

START_TEST(test_phase_ordering_post_timestep)
{
	// within DEFAULT_POST timestep: POST_UPDATE < FINAL
	size_t *order = g_world.scheduler.phases_order;

	size_t pos_post_update = 0, pos_final = 0;

	for (size_t i = 0; i < g_world.scheduler.phases_order_length; i++) {
		if (order[i] == WM_PHASE_POST_UPDATE) pos_post_update = i;
		if (order[i] == WM_PHASE_FINAL)       pos_final = i;
	}

	ck_assert_uint_lt(pos_post_update, pos_final);
}
END_TEST


/*****************************
*  all phases enabled        *
*****************************/

START_TEST(test_all_phases_enabled)
{
	for (size_t i = 0; i <= WM_PHASE_POST; i++) {
		struct w_scheduler_phase *p = w_ecs_get_system_phase(&g_world, i);
		ck_assert_ptr_nonnull(p);
		ck_assert_msg(p->enabled, "phase %zu should be enabled", i);
	}
}
END_TEST


/*****************************
*  all timesteps enabled     *
*****************************/

START_TEST(test_all_timesteps_enabled)
{
	for (size_t i = 0; i <= WM_TIMESTEP_POST; i++) {
		struct w_scheduler_time_step *ts = w_ecs_get_system_time_step(&g_world, i);
		ck_assert_ptr_nonnull(ts);
		ck_assert_msg(ts->enabled, "timestep %zu should be enabled", i);
	}
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *scheduler_defaults_suite(void)
{
	Suite *s = suite_create("scheduler_defaults");

	TCase *tc_counts = tcase_create("counts");
	tcase_add_checked_fixture(tc_counts, defaults_setup, defaults_teardown);
	tcase_set_timeout(tc_counts, 10);
	tcase_add_test(tc_counts, test_timestep_count);
	tcase_add_test(tc_counts, test_phase_count);
	suite_add_tcase(s, tc_counts);

	TCase *tc_ids = tcase_create("ids_valid");
	tcase_add_checked_fixture(tc_ids, defaults_setup, defaults_teardown);
	tcase_set_timeout(tc_ids, 10);
	tcase_add_test(tc_ids, test_timestep_ids_valid);
	tcase_add_test(tc_ids, test_phase_ids_valid);
	suite_add_tcase(s, tc_ids);

	TCase *tc_ts_order = tcase_create("timestep_ordering");
	tcase_add_checked_fixture(tc_ts_order, defaults_setup, defaults_teardown);
	tcase_set_timeout(tc_ts_order, 10);
	tcase_add_test(tc_ts_order, test_timestep_ordering);
	suite_add_tcase(s, tc_ts_order);

	TCase *tc_assign = tcase_create("phase_timestep_assignments");
	tcase_add_checked_fixture(tc_assign, defaults_setup, defaults_teardown);
	tcase_set_timeout(tc_assign, 10);
	tcase_add_test(tc_assign, test_phase_timestep_assignments);
	suite_add_tcase(s, tc_assign);

	TCase *tc_phase_order = tcase_create("phase_ordering");
	tcase_add_checked_fixture(tc_phase_order, defaults_setup, defaults_teardown);
	tcase_set_timeout(tc_phase_order, 10);
	tcase_add_test(tc_phase_order, test_phase_ordering_default_timestep);
	tcase_add_test(tc_phase_order, test_phase_ordering_fixed_timestep);
	tcase_add_test(tc_phase_order, test_phase_ordering_render_timestep);
	tcase_add_test(tc_phase_order, test_phase_ordering_post_timestep);
	suite_add_tcase(s, tc_phase_order);

	TCase *tc_state = tcase_create("enabled_state");
	tcase_add_checked_fixture(tc_state, defaults_setup, defaults_teardown);
	tcase_set_timeout(tc_state, 10);
	tcase_add_test(tc_state, test_all_phases_enabled);
	tcase_add_test(tc_state, test_all_timesteps_enabled);
	suite_add_tcase(s, tc_state);

	return s;
}

int main(void)
{
	Suite *s = scheduler_defaults_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
