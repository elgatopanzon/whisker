/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_ecs_world
 * @created     : Wednesday Mar 04, 2026 20:39:19 CST
 * @description : tests for ECS world update loop and system scheduling
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_arena g_arena;
static struct w_string_table g_string_table;

static void world_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
}

static void world_teardown(void)
{
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  test system functions     *
*****************************/

static int g_sys_a_call_count;
static int g_sys_b_call_count;
static int g_sys_c_call_count;
static double g_sys_a_last_delta;
static double g_sys_b_last_delta;
static double g_sys_c_last_delta;

static void reset_test_globals(void)
{
	g_sys_a_call_count = 0;
	g_sys_b_call_count = 0;
	g_sys_c_call_count = 0;
	g_sys_a_last_delta = 0.0;
	g_sys_b_last_delta = 0.0;
	g_sys_c_last_delta = 0.0;
}

static void test_system_a(void *ctx, double delta_time)
{
	(void)ctx;
	g_sys_a_call_count++;
	g_sys_a_last_delta = delta_time;
}

static void test_system_b(void *ctx, double delta_time)
{
	(void)ctx;
	g_sys_b_call_count++;
	g_sys_b_last_delta = delta_time;
}

static void test_system_c(void *ctx, double delta_time)
{
	(void)ctx;
	g_sys_c_call_count++;
	g_sys_c_last_delta = delta_time;
}


/*****************************
*  init and free             *
*****************************/

START_TEST(test_init_scheduler_empty)
{
	ck_assert_int_eq(g_world.scheduler.time_steps_length, 0);
	ck_assert_int_eq(g_world.scheduler.phases_length, 0);
}
END_TEST

START_TEST(test_init_systems_empty)
{
	ck_assert_int_eq(g_world.systems.systems_length, 0);
}
END_TEST

START_TEST(test_init_scheduler_jobs_dirty)
{
	ck_assert(g_world.scheduler_jobs_dirty);
}
END_TEST

START_TEST(test_init_update_result_continue)
{
	ck_assert_int_eq(g_world.update_result, W_WORLD_UPDATE_RESULT_CONTINUE);
}
END_TEST


/*****************************
*  timestep registration     *
*****************************/

START_TEST(test_register_timestep_via_scheduler)
{
	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {
			.delta_time_fixed = 0.016,
			.tick_count = 0,
		}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	ck_assert_int_eq(ts_id, 0);
	ck_assert_int_eq(g_world.scheduler.time_steps_length, 1);
}
END_TEST

START_TEST(test_register_multiple_timesteps)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts1 = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	size_t ts2 = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	ck_assert_int_eq(ts1, 0);
	ck_assert_int_eq(ts2, 1);
	ck_assert_int_eq(g_world.scheduler.time_steps_length, 2);
}
END_TEST


/*****************************
*  phase registration        *
*****************************/

START_TEST(test_register_phase_via_scheduler)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);
	ck_assert_int_eq(phase_id, 0);
	ck_assert_int_eq(g_world.scheduler.phases_length, 1);
}
END_TEST

START_TEST(test_register_multiple_phases)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t p1 = w_scheduler_register_phase(&g_world.scheduler, &phase);
	size_t p2 = w_scheduler_register_phase(&g_world.scheduler, &phase);
	size_t p3 = w_scheduler_register_phase(&g_world.scheduler, &phase);
	ck_assert_int_eq(p1, 0);
	ck_assert_int_eq(p2, 1);
	ck_assert_int_eq(p3, 2);
}
END_TEST


/*****************************
*  system registration       *
*****************************/

START_TEST(test_register_system)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 0,
	};
	size_t sys_id = w_ecs_register_system(&g_world, &sys);
	ck_assert_int_eq(sys_id, 0);
	ck_assert_int_eq(g_world.systems.systems_length, 1);
}
END_TEST

START_TEST(test_register_system_marks_jobs_dirty)
{
	g_world.scheduler_jobs_dirty = false;

	struct w_system sys = {.phase_id = 1, .update = test_system_a};
	w_ecs_register_system(&g_world, &sys);

	ck_assert(g_world.scheduler_jobs_dirty);
}
END_TEST

START_TEST(test_register_multiple_systems)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a};
	size_t id1 = w_ecs_register_system(&g_world, &sys);
	size_t id2 = w_ecs_register_system(&g_world, &sys);
	size_t id3 = w_ecs_register_system(&g_world, &sys);

	ck_assert_int_eq(id1, 0);
	ck_assert_int_eq(id2, 1);
	ck_assert_int_eq(id3, 2);
	ck_assert_int_eq(g_world.systems.systems_length, 3);
}
END_TEST


/*****************************
*  basic update execution    *
*****************************/

START_TEST(test_update_calls_system)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016, .tick_count = 0}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 0,
	};
	w_ecs_register_system(&g_world, &sys);

	w_ecs_update(&g_world);

	ck_assert_int_eq(g_sys_a_call_count, 1);
}
END_TEST

START_TEST(test_update_multiple_times)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016, .tick_count = 0}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 0,
	};
	w_ecs_register_system(&g_world, &sys);

	w_ecs_update(&g_world);
	w_ecs_update(&g_world);
	w_ecs_update(&g_world);

	ck_assert_int_eq(g_sys_a_call_count, 3);
}
END_TEST

START_TEST(test_update_returns_result)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	w_scheduler_register_time_step(&g_world.scheduler, &ts);

	enum W_WORLD_UPDATE_RESULT result = w_ecs_update(&g_world);
	ck_assert_int_eq(result, W_WORLD_UPDATE_RESULT_CONTINUE);

	g_world.update_result = W_WORLD_UPDATE_RESULT_SHUTDOWN;
	result = w_ecs_update(&g_world);
	ck_assert_int_eq(result, W_WORLD_UPDATE_RESULT_SHUTDOWN);
}
END_TEST

START_TEST(test_update_multiple_systems_same_phase)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys_a = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	struct w_system sys_b = {.phase_id = phase_id, .update = test_system_b, .update_frequency = 0};
	struct w_system sys_c = {.phase_id = phase_id, .update = test_system_c, .update_frequency = 0};
	w_ecs_register_system(&g_world, &sys_a);
	w_ecs_register_system(&g_world, &sys_b);
	w_ecs_register_system(&g_world, &sys_c);

	w_ecs_update(&g_world);

	ck_assert_int_eq(g_sys_a_call_count, 1);
	ck_assert_int_eq(g_sys_b_call_count, 1);
	ck_assert_int_eq(g_sys_c_call_count, 1);
}
END_TEST


/*****************************
*  delta time (frequency=0)  *
*****************************/

START_TEST(test_frequency_zero_uses_timestep_delta)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.025, .tick_count = 0}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 0,
	};
	w_ecs_register_system(&g_world, &sys);

	w_ecs_update(&g_world);

	ck_assert_double_eq_tol(g_sys_a_last_delta, 0.025, 0.0001);
}
END_TEST

START_TEST(test_frequency_zero_different_timestep_deltas)
{
	reset_test_globals();

	struct w_scheduler_time_step ts1 = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016}
	};
	struct w_scheduler_time_step ts2 = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.033}
	};
	size_t ts1_id = w_scheduler_register_time_step(&g_world.scheduler, &ts1);
	size_t ts2_id = w_scheduler_register_time_step(&g_world.scheduler, &ts2);

	struct w_scheduler_phase phase1 = {.enabled = true, .time_step_id = ts1_id};
	struct w_scheduler_phase phase2 = {.enabled = true, .time_step_id = ts2_id};
	size_t phase1_id = w_scheduler_register_phase(&g_world.scheduler, &phase1);
	size_t phase2_id = w_scheduler_register_phase(&g_world.scheduler, &phase2);

	struct w_system sys_a = {.phase_id = phase1_id, .update = test_system_a, .update_frequency = 0};
	struct w_system sys_b = {.phase_id = phase2_id, .update = test_system_b, .update_frequency = 0};
	w_ecs_register_system(&g_world, &sys_a);
	w_ecs_register_system(&g_world, &sys_b);

	w_ecs_update(&g_world);

	ck_assert_double_eq_tol(g_sys_a_last_delta, 0.016, 0.0001);
	ck_assert_double_eq_tol(g_sys_b_last_delta, 0.033, 0.0001);
}
END_TEST


/*****************************
*  per-system frequency      *
*****************************/

START_TEST(test_frequency_skips_until_enough_ticks)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016, .tick_count = 0}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// system runs every 3 ticks
	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 3,
	};
	w_ecs_register_system(&g_world, &sys);

	// tick 0: ticks_elapsed = 0 - 0 = 0 < 3, skip
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_sys_a_call_count, 0);

	// tick 1: ticks_elapsed = 1 - 0 = 1 < 3, skip
	g_world.scheduler.time_steps[ts_id].time_step.tick_count = 1;
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_sys_a_call_count, 0);

	// tick 2: ticks_elapsed = 2 - 0 = 2 < 3, skip
	g_world.scheduler.time_steps[ts_id].time_step.tick_count = 2;
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_sys_a_call_count, 0);

	// tick 3: ticks_elapsed = 3 - 0 = 3 >= 3, run
	g_world.scheduler.time_steps[ts_id].time_step.tick_count = 3;
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_sys_a_call_count, 1);
}
END_TEST

START_TEST(test_frequency_updates_last_update_ticks)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016, .tick_count = 0}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 2,
	};
	size_t sys_id = w_ecs_register_system(&g_world, &sys);

	// initial last_update_ticks should be 0
	struct w_system *sys_entry = w_ecs_get_system_entry(&g_world, sys_id);
	ck_assert_int_eq(sys_entry->last_update_ticks, 0);

	// tick 2: should run and update last_update_ticks to 2
	g_world.scheduler.time_steps[ts_id].time_step.tick_count = 2;
	w_ecs_update(&g_world);
	ck_assert_int_eq(sys_entry->last_update_ticks, 2);

	// tick 4: should run and update last_update_ticks to 4
	g_world.scheduler.time_steps[ts_id].time_step.tick_count = 4;
	w_ecs_update(&g_world);
	ck_assert_int_eq(sys_entry->last_update_ticks, 4);
}
END_TEST

START_TEST(test_frequency_accumulated_delta_time)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.010, .tick_count = 0}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// system runs every 5 ticks
	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 5,
	};
	w_ecs_register_system(&g_world, &sys);

	// tick 5: 5 ticks elapsed * 0.010 = 0.050 delta
	g_world.scheduler.time_steps[ts_id].time_step.tick_count = 5;
	w_ecs_update(&g_world);

	ck_assert_int_eq(g_sys_a_call_count, 1);
	ck_assert_double_eq_tol(g_sys_a_last_delta, 0.050, 0.0001);
}
END_TEST

START_TEST(test_frequency_mixed_systems)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016, .tick_count = 0}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// system A: every tick (frequency=0)
	struct w_system sys_a = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	// system B: every 3 ticks
	struct w_system sys_b = {.phase_id = phase_id, .update = test_system_b, .update_frequency = 3};
	w_ecs_register_system(&g_world, &sys_a);
	w_ecs_register_system(&g_world, &sys_b);

	// run updates for ticks 0, 1, 2, 3
	for (int i = 0; i <= 3; i++) {
		g_world.scheduler.time_steps[ts_id].time_step.tick_count = i;
		w_ecs_update(&g_world);
	}

	// system A should have been called 4 times (ticks 0, 1, 2, 3)
	ck_assert_int_eq(g_sys_a_call_count, 4);
	// system B should have been called 1 time (tick 3)
	ck_assert_int_eq(g_sys_b_call_count, 1);
}
END_TEST

START_TEST(test_frequency_delta_on_late_run)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.010, .tick_count = 0}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// system runs every 2 ticks
	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 2,
	};
	w_ecs_register_system(&g_world, &sys);

	// first run at tick 2: 2 ticks * 0.010 = 0.020
	g_world.scheduler.time_steps[ts_id].time_step.tick_count = 2;
	w_ecs_update(&g_world);
	ck_assert_double_eq_tol(g_sys_a_last_delta, 0.020, 0.0001);

	// late run at tick 7: 7 - 2 = 5 ticks * 0.010 = 0.050
	g_world.scheduler.time_steps[ts_id].time_step.tick_count = 7;
	w_ecs_update(&g_world);
	ck_assert_double_eq_tol(g_sys_a_last_delta, 0.050, 0.0001);
}
END_TEST


/*****************************
*  disabled systems          *
*****************************/

START_TEST(test_disabled_system_not_called)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	size_t sys_id = w_ecs_register_system(&g_world, &sys);

	// disable the system
	w_ecs_set_system_state(&g_world, sys_id, false);

	w_ecs_update(&g_world);

	ck_assert_int_eq(g_sys_a_call_count, 0);
}
END_TEST

START_TEST(test_reenable_system_called)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	size_t sys_id = w_ecs_register_system(&g_world, &sys);

	// disable
	w_ecs_set_system_state(&g_world, sys_id, false);
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_sys_a_call_count, 0);

	// reenable
	w_ecs_set_system_state(&g_world, sys_id, true);
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_sys_a_call_count, 1);
}
END_TEST


/*****************************
*  phase ordering            *
*****************************/

static int g_execution_order[10];
static int g_execution_order_index;

static void order_system_1(void *ctx, double delta_time)
{
	(void)ctx; (void)delta_time;
	g_execution_order[g_execution_order_index++] = 1;
}

static void order_system_2(void *ctx, double delta_time)
{
	(void)ctx; (void)delta_time;
	g_execution_order[g_execution_order_index++] = 2;
}

static void order_system_3(void *ctx, double delta_time)
{
	(void)ctx; (void)delta_time;
	g_execution_order[g_execution_order_index++] = 3;
}

START_TEST(test_systems_in_different_phases_order)
{
	g_execution_order_index = 0;
	memset(g_execution_order, 0, sizeof(g_execution_order));

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = {.delta_time_fixed = 0.016}
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase1 = {.enabled = true, .time_step_id = ts_id};
	struct w_scheduler_phase phase2 = {.enabled = true, .time_step_id = ts_id};
	struct w_scheduler_phase phase3 = {.enabled = true, .time_step_id = ts_id};
	size_t p1 = w_scheduler_register_phase(&g_world.scheduler, &phase1);
	size_t p2 = w_scheduler_register_phase(&g_world.scheduler, &phase2);
	size_t p3 = w_scheduler_register_phase(&g_world.scheduler, &phase3);

	// register systems in reverse order to prove phase determines execution order
	struct w_system sys3 = {.phase_id = p3, .update = order_system_3, .update_frequency = 0};
	struct w_system sys1 = {.phase_id = p1, .update = order_system_1, .update_frequency = 0};
	struct w_system sys2 = {.phase_id = p2, .update = order_system_2, .update_frequency = 0};
	w_ecs_register_system(&g_world, &sys3);
	w_ecs_register_system(&g_world, &sys1);
	w_ecs_register_system(&g_world, &sys2);

	w_ecs_update(&g_world);

	// phases execute in registration order: p1, p2, p3
	ck_assert_int_eq(g_execution_order[0], 1);
	ck_assert_int_eq(g_execution_order[1], 2);
	ck_assert_int_eq(g_execution_order[2], 3);
}
END_TEST


/*****************************
*  empty world update        *
*****************************/

START_TEST(test_update_empty_world_no_crash)
{
	// just ensure no crash on empty world
	enum W_WORLD_UPDATE_RESULT result = w_ecs_update(&g_world);
	ck_assert_int_eq(result, W_WORLD_UPDATE_RESULT_CONTINUE);
}
END_TEST

START_TEST(test_update_no_systems_no_crash)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_world.scheduler, &phase);

	// no systems registered
	enum W_WORLD_UPDATE_RESULT result = w_ecs_update(&g_world);
	ck_assert_int_eq(result, W_WORLD_UPDATE_RESULT_CONTINUE);
}
END_TEST


/*****************************
*  schedule rebuild_count    *
*****************************/

START_TEST(test_rebuild_count_after_first_update)
{
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = {.delta_time_fixed = 0.016}};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	w_ecs_register_system(&g_world, &sys);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 1);
}
END_TEST

START_TEST(test_rebuild_count_subsequent_updates_no_rebuild)
{
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = {.delta_time_fixed = 0.016}};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	w_ecs_register_system(&g_world, &sys);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 1);

	w_ecs_update(&g_world);
	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 1);
}
END_TEST

START_TEST(test_rebuild_count_register_system_triggers_rebuild)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {.enabled = true, .time_step = {.delta_time_fixed = 0.016}};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	w_ecs_register_system(&g_world, &sys);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 1);

	struct w_system sys2 = {.phase_id = phase_id, .update = test_system_b, .update_frequency = 0};
	w_ecs_register_system(&g_world, &sys2);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 2);
}
END_TEST

START_TEST(test_rebuild_count_disable_system_triggers_rebuild)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {.enabled = true, .time_step = {.delta_time_fixed = 0.016}};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	size_t sys_id = w_ecs_register_system(&g_world, &sys);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 1);

	w_ecs_set_system_state(&g_world, sys_id, false);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 2);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_ecs_world_suite(void)
{
	Suite *s = suite_create("whisker_ecs_world");

	TCase *tc_init = tcase_create("init_free");
	tcase_add_checked_fixture(tc_init, world_setup, world_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_scheduler_empty);
	tcase_add_test(tc_init, test_init_systems_empty);
	tcase_add_test(tc_init, test_init_scheduler_jobs_dirty);
	tcase_add_test(tc_init, test_init_update_result_continue);
	suite_add_tcase(s, tc_init);

	TCase *tc_timestep = tcase_create("timestep_registration");
	tcase_add_checked_fixture(tc_timestep, world_setup, world_teardown);
	tcase_set_timeout(tc_timestep, 10);
	tcase_add_test(tc_timestep, test_register_timestep_via_scheduler);
	tcase_add_test(tc_timestep, test_register_multiple_timesteps);
	suite_add_tcase(s, tc_timestep);

	TCase *tc_phase = tcase_create("phase_registration");
	tcase_add_checked_fixture(tc_phase, world_setup, world_teardown);
	tcase_set_timeout(tc_phase, 10);
	tcase_add_test(tc_phase, test_register_phase_via_scheduler);
	tcase_add_test(tc_phase, test_register_multiple_phases);
	suite_add_tcase(s, tc_phase);

	TCase *tc_system = tcase_create("system_registration");
	tcase_add_checked_fixture(tc_system, world_setup, world_teardown);
	tcase_set_timeout(tc_system, 10);
	tcase_add_test(tc_system, test_register_system);
	tcase_add_test(tc_system, test_register_system_marks_jobs_dirty);
	tcase_add_test(tc_system, test_register_multiple_systems);
	suite_add_tcase(s, tc_system);

	TCase *tc_update = tcase_create("basic_update");
	tcase_add_checked_fixture(tc_update, world_setup, world_teardown);
	tcase_set_timeout(tc_update, 10);
	tcase_add_test(tc_update, test_update_calls_system);
	tcase_add_test(tc_update, test_update_multiple_times);
	tcase_add_test(tc_update, test_update_returns_result);
	tcase_add_test(tc_update, test_update_multiple_systems_same_phase);
	suite_add_tcase(s, tc_update);

	TCase *tc_delta = tcase_create("delta_time_frequency_zero");
	tcase_add_checked_fixture(tc_delta, world_setup, world_teardown);
	tcase_set_timeout(tc_delta, 10);
	tcase_add_test(tc_delta, test_frequency_zero_uses_timestep_delta);
	tcase_add_test(tc_delta, test_frequency_zero_different_timestep_deltas);
	suite_add_tcase(s, tc_delta);

	TCase *tc_freq = tcase_create("per_system_frequency");
	tcase_add_checked_fixture(tc_freq, world_setup, world_teardown);
	tcase_set_timeout(tc_freq, 10);
	tcase_add_test(tc_freq, test_frequency_skips_until_enough_ticks);
	tcase_add_test(tc_freq, test_frequency_updates_last_update_ticks);
	tcase_add_test(tc_freq, test_frequency_accumulated_delta_time);
	tcase_add_test(tc_freq, test_frequency_mixed_systems);
	tcase_add_test(tc_freq, test_frequency_delta_on_late_run);
	suite_add_tcase(s, tc_freq);

	TCase *tc_disabled = tcase_create("disabled_systems");
	tcase_add_checked_fixture(tc_disabled, world_setup, world_teardown);
	tcase_set_timeout(tc_disabled, 10);
	tcase_add_test(tc_disabled, test_disabled_system_not_called);
	tcase_add_test(tc_disabled, test_reenable_system_called);
	suite_add_tcase(s, tc_disabled);

	TCase *tc_order = tcase_create("phase_ordering");
	tcase_add_checked_fixture(tc_order, world_setup, world_teardown);
	tcase_set_timeout(tc_order, 10);
	tcase_add_test(tc_order, test_systems_in_different_phases_order);
	suite_add_tcase(s, tc_order);

	TCase *tc_empty = tcase_create("empty_world");
	tcase_add_checked_fixture(tc_empty, world_setup, world_teardown);
	tcase_set_timeout(tc_empty, 10);
	tcase_add_test(tc_empty, test_update_empty_world_no_crash);
	tcase_add_test(tc_empty, test_update_no_systems_no_crash);
	suite_add_tcase(s, tc_empty);

	TCase *tc_rebuild = tcase_create("schedule_rebuild_count");
	tcase_add_checked_fixture(tc_rebuild, world_setup, world_teardown);
	tcase_set_timeout(tc_rebuild, 10);
	tcase_add_test(tc_rebuild, test_rebuild_count_after_first_update);
	tcase_add_test(tc_rebuild, test_rebuild_count_subsequent_updates_no_rebuild);
	tcase_add_test(tc_rebuild, test_rebuild_count_register_system_triggers_rebuild);
	tcase_add_test(tc_rebuild, test_rebuild_count_disable_system_triggers_rebuild);
	suite_add_tcase(s, tc_rebuild);

	return s;
}

int main(void)
{
	Suite *s = whisker_ecs_world_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
