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

START_TEST(test_init_update_result_init)
{
	ck_assert_int_eq(g_world.update_result, W_WORLD_UPDATE_RESULT_INIT);
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
	size_t sys_id = w_ecs_register_system(&g_world, "test_system_a", &sys);
	ck_assert_int_eq(sys_id, 0);
	ck_assert_int_eq(g_world.systems.systems_length, 1);
}
END_TEST

START_TEST(test_register_system_marks_jobs_dirty)
{
	g_world.scheduler_jobs_dirty = false;

	struct w_system sys = {.phase_id = 1, .update = test_system_a};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

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
	size_t id1 = w_ecs_register_system(&g_world, "sys_a", &sys);
	size_t id2 = w_ecs_register_system(&g_world, "sys_b", &sys);
	size_t id3 = w_ecs_register_system(&g_world, "sys_c", &sys);

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
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 0,
	};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

	w_ecs_update(&g_world);

	ck_assert_int_eq(g_sys_a_call_count, 1);
}
END_TEST

START_TEST(test_update_multiple_times)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 0,
	};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

	w_ecs_update(&g_world);
	w_ecs_update(&g_world);
	w_ecs_update(&g_world);

	ck_assert_int_eq(g_sys_a_call_count, 3);
}
END_TEST

START_TEST(test_update_returns_result)
{
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
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
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys_a = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	struct w_system sys_b = {.phase_id = phase_id, .update = test_system_b, .update_frequency = 0};
	struct w_system sys_c = {.phase_id = phase_id, .update = test_system_c, .update_frequency = 0};
	w_ecs_register_system(&g_world, "test_system_a", &sys_a);
	w_ecs_register_system(&g_world, "test_system_b", &sys_b);
	w_ecs_register_system(&g_world, "test_system_c", &sys_c);

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
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 0,
	};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

	w_ecs_update(&g_world);

	// delta is real elapsed time from the uncapped timestep
	ck_assert(g_sys_a_last_delta > 0.0);
}
END_TEST

START_TEST(test_frequency_zero_different_timestep_deltas)
{
	reset_test_globals();

	struct w_scheduler_time_step ts1 = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	struct w_scheduler_time_step ts2 = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts1_id = w_scheduler_register_time_step(&g_world.scheduler, &ts1);
	size_t ts2_id = w_scheduler_register_time_step(&g_world.scheduler, &ts2);

	struct w_scheduler_phase phase1 = {.enabled = true, .time_step_id = ts1_id};
	struct w_scheduler_phase phase2 = {.enabled = true, .time_step_id = ts2_id};
	size_t phase1_id = w_scheduler_register_phase(&g_world.scheduler, &phase1);
	size_t phase2_id = w_scheduler_register_phase(&g_world.scheduler, &phase2);

	struct w_system sys_a = {.phase_id = phase1_id, .update = test_system_a, .update_frequency = 0};
	struct w_system sys_b = {.phase_id = phase2_id, .update = test_system_b, .update_frequency = 0};
	w_ecs_register_system(&g_world, "test_system_a", &sys_a);
	w_ecs_register_system(&g_world, "test_system_b", &sys_b);

	w_ecs_update(&g_world);

	// each timestep passes real elapsed time as delta
	ck_assert(g_sys_a_last_delta > 0.0);
	ck_assert(g_sys_b_last_delta > 0.0);
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
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// frequency of 5 seconds (5e12 ns) - won't trigger in a quick test call
	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = (uint64_t)5e12,
	};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

	// not enough time has passed - system should be skipped
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_sys_a_call_count, 0);
}
END_TEST

START_TEST(test_frequency_updates_last_update_ticks)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// frequency of 1 ns - always runs since real frame time > 1 ns
	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 1,
	};
	size_t sys_id = w_ecs_register_system(&g_world, "test_system_a", &sys);

	// initial last_update_ticks should be 0
	struct w_system *sys_entry = w_ecs_get_system_entry(&g_world, sys_id);
	ck_assert_int_eq(sys_entry->last_update_ticks, 0);

	// first run: last_update_ticks should be updated to tick_count
	w_ecs_update(&g_world);
	ck_assert_uint_gt(sys_entry->last_update_ticks, 0);

	// second run: last_update_ticks should increase further
	uint64_t after_first = sys_entry->last_update_ticks;
	w_ecs_update(&g_world);
	ck_assert_uint_gt(sys_entry->last_update_ticks, after_first);
}
END_TEST

START_TEST(test_frequency_accumulated_delta_time)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// frequency of 1 ns - always runs since real frame time > 1 ns
	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 1,
	};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

	w_ecs_update(&g_world);

	ck_assert_int_eq(g_sys_a_call_count, 1);
	ck_assert(g_sys_a_last_delta > 0.0);
}
END_TEST

START_TEST(test_frequency_mixed_systems)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// system A: every frame (frequency=0)
	struct w_system sys_a = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	// system B: 5 second frequency - won't trigger in quick test calls
	struct w_system sys_b = {.phase_id = phase_id, .update = test_system_b, .update_frequency = (uint64_t)5e12};
	w_ecs_register_system(&g_world, "test_system_a", &sys_a);
	w_ecs_register_system(&g_world, "test_system_b", &sys_b);

	// run 4 updates
	for (int i = 0; i < 4; i++) {
		w_ecs_update(&g_world);
	}

	// system A should have been called 4 times (every frame)
	ck_assert_int_eq(g_sys_a_call_count, 4);
	// system B should not have been called (frequency too large for quick test)
	ck_assert_int_eq(g_sys_b_call_count, 0);
}
END_TEST

START_TEST(test_frequency_delta_on_late_run)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// frequency of 1 ns - always runs since real frame time > 1 ns
	struct w_system sys = {
		.phase_id = phase_id,
		.update = test_system_a,
		.update_frequency = 1,
	};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

	// first run: delta is real elapsed time since last run
	w_ecs_update(&g_world);
	ck_assert(g_sys_a_last_delta > 0.0);

	// second run: delta is real elapsed time since previous run
	w_ecs_update(&g_world);
	ck_assert(g_sys_a_last_delta > 0.0);
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
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	size_t sys_id = w_ecs_register_system(&g_world, "test_system_a", &sys);

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
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	size_t sys_id = w_ecs_register_system(&g_world, "test_system_a", &sys);

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
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
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
	w_ecs_register_system(&g_world, "order_system_3", &sys3);
	w_ecs_register_system(&g_world, "order_system_1", &sys1);
	w_ecs_register_system(&g_world, "order_system_2", &sys2);

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
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_world.scheduler, &phase);

	// no systems registered
	enum W_WORLD_UPDATE_RESULT result = w_ecs_update(&g_world);
	ck_assert_int_eq(result, W_WORLD_UPDATE_RESULT_CONTINUE);
}
END_TEST


/*****************************
*  buffered entity commands  *
*****************************/

START_TEST(test_buffered_entity_name_via_update)
{
	// buffering disabled by default
	ck_assert(!g_world.buffering_enabled);

	// enable buffering manually
	g_world.buffering_enabled = true;

	// request entity with name (queued, not applied yet)
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "test_entity");
	ck_assert(entity != W_ENTITY_INVALID);

	// name should NOT be set yet (command is buffered)
	char *name_before = w_ecs_get_entity_name(&g_world, entity);
	ck_assert_ptr_null(name_before);
	w_entity_id lookup_before = w_ecs_get_entity_by_name(&g_world, "test_entity");
	ck_assert(lookup_before == W_ENTITY_INVALID);

	// setup minimal scheduler for update to run hooks
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_world.scheduler, &phase);

	// world update flushes command buffer via phase-end hook
	w_ecs_update(&g_world);

	// name should now be set
	char *name_after = w_ecs_get_entity_name(&g_world, entity);
	ck_assert_ptr_nonnull(name_after);
	ck_assert_str_eq(name_after, "test_entity");

	// lookup by name should work
	w_entity_id lookup_after = w_ecs_get_entity_by_name(&g_world, "test_entity");
	ck_assert(lookup_after == entity);
}
END_TEST


/*****************************
*  return entity buffering   *
*****************************/

START_TEST(test_unbuffered_return_entity)
{
	// buffering disabled by default
	ck_assert(!g_world.buffering_enabled);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	ck_assert(entity != W_ENTITY_INVALID);

	// verify entity is alive (next_id - recycled_stack_length)
	size_t alive_before = g_world.entities.next_id - g_world.entities.recycled_stack_length;
	ck_assert_uint_eq(alive_before, 1);

	// return immediately
	w_ecs_return_entity(&g_world, entity);

	// entity should be recycled immediately
	size_t alive_after = g_world.entities.next_id - g_world.entities.recycled_stack_length;
	ck_assert_uint_eq(alive_after, 0);
	ck_assert_uint_eq(g_world.entities.recycled_stack_length, 1);
}
END_TEST

START_TEST(test_buffered_return_entity)
{
	g_world.buffering_enabled = true;

	w_entity_id entity = w_ecs_request_entity(&g_world);
	ck_assert(entity != W_ENTITY_INVALID);

	size_t alive = g_world.entities.next_id - g_world.entities.recycled_stack_length;
	ck_assert_uint_eq(alive, 1);

	// queue return (buffered)
	w_ecs_return_entity(&g_world, entity);

	// entity should still be alive (command buffered)
	alive = g_world.entities.next_id - g_world.entities.recycled_stack_length;
	ck_assert_uint_eq(alive, 1);
	ck_assert_uint_eq(g_world.entities.recycled_stack_length, 0);

	// setup scheduler and flush via update
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_world.scheduler, &phase);

	w_ecs_update(&g_world);

	// entity should now be recycled
	alive = g_world.entities.next_id - g_world.entities.recycled_stack_length;
	ck_assert_uint_eq(alive, 0);
	ck_assert_uint_eq(g_world.entities.recycled_stack_length, 1);
}
END_TEST


/*****************************
*  clear entity name buffering *
*****************************/

START_TEST(test_unbuffered_clear_entity_name)
{
	ck_assert(!g_world.buffering_enabled);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_ecs_set_entity_name(&g_world, entity, "named_entity");

	// verify name is set
	char *name = w_ecs_get_entity_name(&g_world, entity);
	ck_assert_ptr_nonnull(name);
	ck_assert_str_eq(name, "named_entity");

	// clear name immediately
	w_ecs_clear_entity_name(&g_world, entity);

	// name should be cleared immediately
	char *name_after = w_ecs_get_entity_name(&g_world, entity);
	ck_assert_ptr_null(name_after);
	w_entity_id lookup = w_ecs_get_entity_by_name(&g_world, "named_entity");
	ck_assert(lookup == W_ENTITY_INVALID);
}
END_TEST

START_TEST(test_buffered_clear_entity_name)
{
	// set name unbuffered first
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_ecs_set_entity_name(&g_world, entity, "named_entity");

	char *name = w_ecs_get_entity_name(&g_world, entity);
	ck_assert_ptr_nonnull(name);
	ck_assert_str_eq(name, "named_entity");

	// enable buffering
	g_world.buffering_enabled = true;

	// queue clear name (buffered)
	w_ecs_clear_entity_name(&g_world, entity);

	// name should still be set (command buffered)
	char *name_before = w_ecs_get_entity_name(&g_world, entity);
	ck_assert_ptr_nonnull(name_before);
	ck_assert_str_eq(name_before, "named_entity");
	w_entity_id lookup_before = w_ecs_get_entity_by_name(&g_world, "named_entity");
	ck_assert(lookup_before == entity);

	// setup scheduler and flush via update
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_world.scheduler, &phase);

	w_ecs_update(&g_world);

	// name should now be cleared
	char *name_after = w_ecs_get_entity_name(&g_world, entity);
	ck_assert_ptr_null(name_after);
	w_entity_id lookup_after = w_ecs_get_entity_by_name(&g_world, "named_entity");
	ck_assert(lookup_after == W_ENTITY_INVALID);
}
END_TEST


/*****************************
*  set component buffering   *
*****************************/

struct test_component
{
	int value;
	float data;
};

static w_entity_id g_test_component_type_id = W_ENTITY_INVALID;


/*****************************
*  system execution test     *
*  global state              *
*****************************/

static int g_sysexec_counter = 0;
static double g_sysexec_last_dt = 0.0;

static void system_increment_counter_(void *ctx, double dt)
{
	(void)ctx;
	g_sysexec_counter++;
	g_sysexec_last_dt = dt;
}

START_TEST(test_unbuffered_set_component)
{
	ck_assert(!g_world.buffering_enabled);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	g_test_component_type_id = w_ecs_get_component_by_name(&g_world, "test_component");

	struct test_component comp = {.value = 42, .data = 3.14f};

	// set component immediately
	w_ecs_set_component_(&g_world, 0, g_test_component_type_id, entity, &comp, sizeof(comp));

	// component should exist immediately
	ck_assert(w_ecs_has_component_(&g_world, g_test_component_type_id, entity));

	struct test_component *retrieved = w_ecs_get_component_(&g_world, g_test_component_type_id, entity);
	ck_assert_ptr_nonnull(retrieved);
	ck_assert_int_eq(retrieved->value, 42);
	ck_assert_float_eq_tol(retrieved->data, 3.14f, 0.001f);
}
END_TEST

START_TEST(test_buffered_set_component)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	g_test_component_type_id = w_ecs_get_component_by_name(&g_world, "test_component_buffered");

	// enable buffering
	g_world.buffering_enabled = true;

	struct test_component comp = {.value = 99, .data = 2.71f};

	// queue set component (buffered)
	w_ecs_set_component_(&g_world, 0, g_test_component_type_id, entity, &comp, sizeof(comp));

	// component should NOT exist yet (command buffered)
	ck_assert(!w_ecs_has_component_(&g_world, g_test_component_type_id, entity));

	// setup scheduler and flush via update
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_world.scheduler, &phase);

	w_ecs_update(&g_world);

	// component should now exist with correct values
	ck_assert(w_ecs_has_component_(&g_world, g_test_component_type_id, entity));

	struct test_component *retrieved = w_ecs_get_component_(&g_world, g_test_component_type_id, entity);
	ck_assert_ptr_nonnull(retrieved);
	ck_assert_int_eq(retrieved->value, 99);
	ck_assert_float_eq_tol(retrieved->data, 2.71f, 0.001f);
}
END_TEST


/*****************************
*  remove component buffering *
*****************************/

START_TEST(test_unbuffered_remove_component)
{
	ck_assert(!g_world.buffering_enabled);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	g_test_component_type_id = w_ecs_get_component_by_name(&g_world, "test_component_remove");

	struct test_component comp = {.value = 123, .data = 1.0f};
	w_ecs_set_component_(&g_world, 0, g_test_component_type_id, entity, &comp, sizeof(comp));

	// verify component exists
	ck_assert(w_ecs_has_component_(&g_world, g_test_component_type_id, entity));

	// remove immediately
	w_ecs_remove_component_(&g_world, g_test_component_type_id, entity);

	// component should be removed immediately
	ck_assert(!w_ecs_has_component_(&g_world, g_test_component_type_id, entity));
}
END_TEST

START_TEST(test_buffered_remove_component)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	g_test_component_type_id = w_ecs_get_component_by_name(&g_world, "test_component_remove_buffered");

	struct test_component comp = {.value = 456, .data = 9.0f};
	w_ecs_set_component_(&g_world, 0, g_test_component_type_id, entity, &comp, sizeof(comp));

	// verify component exists
	ck_assert(w_ecs_has_component_(&g_world, g_test_component_type_id, entity));

	// enable buffering
	g_world.buffering_enabled = true;

	// queue remove component (buffered)
	w_ecs_remove_component_(&g_world, g_test_component_type_id, entity);

	// component should still exist (command buffered)
	ck_assert(w_ecs_has_component_(&g_world, g_test_component_type_id, entity));
	struct test_component *before = w_ecs_get_component_(&g_world, g_test_component_type_id, entity);
	ck_assert_int_eq(before->value, 456);

	// setup scheduler and flush via update
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_world.scheduler, &phase);

	w_ecs_update(&g_world);

	// component should now be removed
	ck_assert(!w_ecs_has_component_(&g_world, g_test_component_type_id, entity));
}
END_TEST


/*****************************
*  system execution verifies *
*  systems run during update *
*****************************/

START_TEST(test_system_executes_during_update)
{
	g_sysexec_counter = 0;
	g_sysexec_last_dt = 0.0;

	// setup timestep with fixed delta
	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// register system that increments counter
	struct w_system sys = {
		.phase_id = phase_id,
		.enabled = true,
		.update = system_increment_counter_,
		.update_frequency = 0
	};
	w_ecs_register_system(&g_world, "system_increment_counter", &sys);

	// counter should be zero before update
	ck_assert_int_eq(g_sysexec_counter, 0);

	// run world update
	w_ecs_update(&g_world);

	// system MUST have executed - counter should be 1
	ck_assert_msg(g_sysexec_counter == 1,
		"System did not execute during w_ecs_update(). Counter is %d, expected 1",
		g_sysexec_counter);

	// verify delta time was passed (real elapsed time from uncapped timestep)
	ck_assert(g_sysexec_last_dt > 0.0);
}
END_TEST

START_TEST(test_multiple_systems_execute_during_update)
{
	g_sysexec_counter = 0;

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	// register 5 systems in same phase
	char name_buf[32];
	for (int i = 0; i < 5; i++)
	{
		struct w_system sys = {
			.phase_id = phase_id,
			.enabled = true,
			.update = system_increment_counter_,
			.update_frequency = 0
		};
		snprintf(name_buf, sizeof(name_buf), "sys_%d", i);
		w_ecs_register_system(&g_world, name_buf, &sys);
	}

	w_ecs_update(&g_world);

	// all 5 systems should have executed
	ck_assert_msg(g_sysexec_counter == 5,
		"Not all systems executed. Counter is %d, expected 5",
		g_sysexec_counter);
}
END_TEST

START_TEST(test_system_executes_multiple_updates)
{
	g_sysexec_counter = 0;

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {
		.phase_id = phase_id,
		.enabled = true,
		.update = system_increment_counter_,
		.update_frequency = 0
	};
	w_ecs_register_system(&g_world, "system_increment_counter", &sys);

	// run 10 updates
	for (int i = 0; i < 10; i++)
	{
		w_ecs_update(&g_world);
	}

	// system should have executed 10 times
	ck_assert_msg(g_sysexec_counter == 10,
		"System did not execute on every update. Counter is %d, expected 10",
		g_sysexec_counter);
}
END_TEST


/*****************************
*  schedule rebuild_count    *
*****************************/

START_TEST(test_rebuild_count_after_first_update)
{
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 1);
}
END_TEST

START_TEST(test_rebuild_count_subsequent_updates_no_rebuild)
{
	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

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

	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	w_ecs_register_system(&g_world, "test_system_a", &sys);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 1);

	struct w_system sys2 = {.phase_id = phase_id, .update = test_system_b, .update_frequency = 0};
	w_ecs_register_system(&g_world, "test_system_b", &sys2);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 2);
}
END_TEST

START_TEST(test_rebuild_count_disable_system_triggers_rebuild)
{
	reset_test_globals();

	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_world.scheduler, &phase);

	struct w_system sys = {.phase_id = phase_id, .update = test_system_a, .update_frequency = 0};
	size_t sys_id = w_ecs_register_system(&g_world, "test_system_a", &sys);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 1);

	w_ecs_set_system_state(&g_world, sys_id, false);

	w_ecs_update(&g_world);
	ck_assert_uint_eq(g_world.scheduler.schedule.rebuild_count, 2);
}
END_TEST


/*****************************
*  remove all components     *
*****************************/

static int g_remove_all_hook_count = 0;

static void hook_count_removes_(void *ctx, void *data)
{
	(void)ctx;
	(void)data;
	g_remove_all_hook_count++;
}

START_TEST(test_remove_all_components_removes_all)
{
	ck_assert(!g_world.buffering_enabled);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp_a = w_ecs_get_component_by_name(&g_world, "rac_comp_a");
	w_entity_id comp_b = w_ecs_get_component_by_name(&g_world, "rac_comp_b");
	w_entity_id comp_c = w_ecs_get_component_by_name(&g_world, "rac_comp_c");

	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp_a, entity, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_b, entity, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_c, entity, &val, sizeof(val));

	ck_assert(w_ecs_has_component_(&g_world, comp_a, entity));
	ck_assert(w_ecs_has_component_(&g_world, comp_b, entity));
	ck_assert(w_ecs_has_component_(&g_world, comp_c, entity));

	w_ecs_remove_all_components_(&g_world, entity);

	ck_assert(!w_ecs_has_component_(&g_world, comp_a, entity));
	ck_assert(!w_ecs_has_component_(&g_world, comp_b, entity));
	ck_assert(!w_ecs_has_component_(&g_world, comp_c, entity));
}
END_TEST

START_TEST(test_remove_all_components_hooks_fired)
{
	ck_assert(!g_world.buffering_enabled);

	g_remove_all_hook_count = 0;
	w_ecs_register_component_remove_hook(&g_world, 0, hook_count_removes_);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp_a = w_ecs_get_component_by_name(&g_world, "rac_hook_a");
	w_entity_id comp_b = w_ecs_get_component_by_name(&g_world, "rac_hook_b");
	w_entity_id comp_c = w_ecs_get_component_by_name(&g_world, "rac_hook_c");

	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp_a, entity, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_b, entity, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_c, entity, &val, sizeof(val));

	w_ecs_remove_all_components_(&g_world, entity);

	ck_assert_int_eq(g_remove_all_hook_count, 3);
}
END_TEST

START_TEST(test_remove_all_components_no_components_noop)
{
	ck_assert(!g_world.buffering_enabled);

	g_remove_all_hook_count = 0;
	w_ecs_register_component_remove_hook(&g_world, 0, hook_count_removes_);

	w_entity_id entity = w_ecs_request_entity(&g_world);

	// entity has no components - should be a no-op
	w_ecs_remove_all_components_(&g_world, entity);

	ck_assert_int_eq(g_remove_all_hook_count, 0);
}
END_TEST

START_TEST(test_remove_all_components_leaves_other_entities)
{
	ck_assert(!g_world.buffering_enabled);

	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);
	w_entity_id comp_a = w_ecs_get_component_by_name(&g_world, "rac_multi_a");
	w_entity_id comp_b = w_ecs_get_component_by_name(&g_world, "rac_multi_b");

	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp_a, e1, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_b, e1, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_a, e2, &val, sizeof(val));

	// remove all from e1 only
	w_ecs_remove_all_components_(&g_world, e1);

	// e1 components should be gone
	ck_assert(!w_ecs_has_component_(&g_world, comp_a, e1));
	ck_assert(!w_ecs_has_component_(&g_world, comp_b, e1));

	// e2's component should remain
	ck_assert(w_ecs_has_component_(&g_world, comp_a, e2));
}
END_TEST


/*****************************
*  entity destroy hooks      *
*****************************/

static int g_destroy_hook_count = 0;
static w_entity_id g_destroy_hook_entity_id = W_ENTITY_INVALID;
static bool g_destroy_hook_had_components = false;

static void hook_count_destroys_(void *ctx, void *data)
{
	(void)ctx;
	w_entity_id entity = *(w_entity_id *)data;
	g_destroy_hook_count++;
	g_destroy_hook_entity_id = entity;
}

static void hook_check_components_before_destroy_(void *ctx, void *data)
{
	struct w_ecs_world *world = (struct w_ecs_world *)ctx;
	w_entity_id entity = *(w_entity_id *)data;
	w_entity_id comp = w_ecs_get_component_by_name(world, "edh_comp_check");
	g_destroy_hook_had_components = w_ecs_has_component_(world, comp, entity);
}

static int g_destroy_hook_multi_a = 0;
static int g_destroy_hook_multi_b = 0;

static void hook_multi_a_(void *ctx, void *data)
{
	(void)ctx; (void)data;
	g_destroy_hook_multi_a++;
}

static void hook_multi_b_(void *ctx, void *data)
{
	(void)ctx; (void)data;
	g_destroy_hook_multi_b++;
}

START_TEST(test_entity_destroy_hook_fires)
{
	g_destroy_hook_count = 0;
	w_ecs_register_entity_destroy_hook(&g_world, hook_count_destroys_);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_ecs_return_entity(&g_world, entity);

	ck_assert_int_eq(g_destroy_hook_count, 1);
}
END_TEST

START_TEST(test_entity_destroy_hook_before_component_removal)
{
	g_destroy_hook_had_components = false;
	w_ecs_register_entity_destroy_hook(&g_world, hook_check_components_before_destroy_);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp = w_ecs_get_component_by_name(&g_world, "edh_comp_check");
	int val = 42;
	w_ecs_set_component_(&g_world, 0, comp, entity, &val, sizeof(val));

	w_ecs_return_entity(&g_world, entity);

	// hook should have seen the component still present
	ck_assert(g_destroy_hook_had_components);
}
END_TEST

START_TEST(test_entity_destroy_hook_receives_correct_entity)
{
	g_destroy_hook_count = 0;
	g_destroy_hook_entity_id = W_ENTITY_INVALID;
	w_ecs_register_entity_destroy_hook(&g_world, hook_count_destroys_);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_ecs_return_entity(&g_world, entity);

	ck_assert_int_eq(g_destroy_hook_count, 1);
	ck_assert_uint_eq(g_destroy_hook_entity_id, entity);
}
END_TEST

START_TEST(test_entity_destroy_hook_multiple_hooks_fire)
{
	g_destroy_hook_multi_a = 0;
	g_destroy_hook_multi_b = 0;
	w_ecs_register_entity_destroy_hook(&g_world, hook_multi_a_);
	w_ecs_register_entity_destroy_hook(&g_world, hook_multi_b_);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_ecs_return_entity(&g_world, entity);

	ck_assert_int_eq(g_destroy_hook_multi_a, 1);
	ck_assert_int_eq(g_destroy_hook_multi_b, 1);
}
END_TEST

START_TEST(test_entity_destroy_hook_unregistered_not_fired)
{
	g_destroy_hook_count = 0;
	size_t hook_id = w_ecs_register_entity_destroy_hook(&g_world, hook_count_destroys_);

	// unregister before destroy
	w_ecs_unregister_entity_destroy_hook(&g_world, hook_id);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_ecs_return_entity(&g_world, entity);

	ck_assert_int_eq(g_destroy_hook_count, 0);
}
END_TEST


/*****************************
*  module resources          *
*****************************/

START_TEST(test_module_resource_set_get)
{
	int data = 42;
	w_ecs_set_module_resource(&g_world, 0, &data);

	void *got = w_ecs_get_module_resource(&g_world, 0);
	ck_assert_ptr_eq(got, &data);
	ck_assert_int_eq(*(int *)got, 42);
}
END_TEST

START_TEST(test_module_resource_multiple_slots)
{
	int a = 1, b = 2, c = 3;
	w_ecs_set_module_resource(&g_world, 0, &a);
	w_ecs_set_module_resource(&g_world, 1, &b);
	w_ecs_set_module_resource(&g_world, 5, &c);

	ck_assert_ptr_eq(w_ecs_get_module_resource(&g_world, 0), &a);
	ck_assert_ptr_eq(w_ecs_get_module_resource(&g_world, 1), &b);
	ck_assert_ptr_eq(w_ecs_get_module_resource(&g_world, 5), &c);
}
END_TEST

START_TEST(test_module_resource_auto_growth)
{
	// set beyond initial capacity (8)
	int data = 99;
	w_ecs_set_module_resource(&g_world, 20, &data);

	void *got = w_ecs_get_module_resource(&g_world, 20);
	ck_assert_ptr_eq(got, &data);
	ck_assert_int_eq(*(int *)got, 99);

	// intermediate slots should be NULL
	ck_assert_ptr_null(w_ecs_get_module_resource(&g_world, 10));
}
END_TEST

START_TEST(test_module_resource_out_of_bounds_returns_null)
{
	// no resources set, index 0 should return NULL
	ck_assert_ptr_null(w_ecs_get_module_resource(&g_world, 100));
	ck_assert_ptr_null(w_ecs_get_module_resource(&g_world, 1000));
}
END_TEST

START_TEST(test_module_resource_clear)
{
	int data = 42;
	w_ecs_set_module_resource(&g_world, 3, &data);
	ck_assert_ptr_eq(w_ecs_get_module_resource(&g_world, 3), &data);

	w_ecs_clear_module_resource(&g_world, 3);
	ck_assert_ptr_null(w_ecs_get_module_resource(&g_world, 3));
}
END_TEST

START_TEST(test_module_resource_overwrite)
{
	int a = 1, b = 2;
	w_ecs_set_module_resource(&g_world, 0, &a);
	ck_assert_ptr_eq(w_ecs_get_module_resource(&g_world, 0), &a);

	w_ecs_set_module_resource(&g_world, 0, &b);
	ck_assert_ptr_eq(w_ecs_get_module_resource(&g_world, 0), &b);
}
END_TEST


/*****************************
*  hook definition macros    *
*****************************/

static int g_hook_macro_counter = 0;
static w_entity_id g_hook_macro_entity_id = W_ENTITY_INVALID;

w_ecs_update_hook(hook_macro_update_begin, BEGIN, {
	g_hook_macro_counter++;
})

w_ecs_component_set_hook(hook_macro_comp_set, 0, {
	g_hook_macro_counter++;
})

w_ecs_component_remove_hook(hook_macro_comp_remove, 0, {
	g_hook_macro_counter++;
})

w_ecs_component_pre_set_hook(hook_macro_comp_pre_set, {
	g_hook_macro_counter++;
})

w_ecs_component_pre_remove_hook(hook_macro_comp_pre_remove, {
	g_hook_macro_counter++;
})

static w_entity_id g_hook_macro_id_pre_set_comp_id = W_ENTITY_INVALID;

w_ecs_component_id_pre_set_hook(hook_macro_comp_id_pre_set, g_hook_macro_id_pre_set_comp_id, {
	g_hook_macro_counter++;
})

static w_entity_id g_hook_macro_id_pre_remove_comp_id = W_ENTITY_INVALID;

w_ecs_component_id_pre_remove_hook(hook_macro_comp_id_pre_remove, g_hook_macro_id_pre_remove_comp_id, {
	g_hook_macro_counter++;
})

w_ecs_entity_create_hook(hook_macro_entity_create, {
	g_hook_macro_counter++;
	g_hook_macro_entity_id = entity;
})

w_ecs_entity_destroy_hook(hook_macro_entity_destroy, {
	g_hook_macro_counter++;
	g_hook_macro_entity_id = entity;
})

START_TEST(test_hook_macro_update_begin_fires)
{
	g_hook_macro_counter = 0;
	hook_macro_update_begin_register(&g_world);

	struct w_scheduler_time_step ts = {.enabled = true, .time_step = w_time_step_create(0, 1, true, true, true, true, true, true)};
	size_t ts_id = w_scheduler_register_time_step(&g_world.scheduler, &ts);
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_world.scheduler, &phase);

	w_ecs_update(&g_world);

	ck_assert_int_ge(g_hook_macro_counter, 1);
}
END_TEST

START_TEST(test_hook_macro_component_set_fires)
{
	g_hook_macro_counter = 0;
	hook_macro_comp_set_register(&g_world);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp = w_ecs_get_component_by_name(&g_world, "hm_comp_set");
	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp, entity, &val, sizeof(val));

	ck_assert_int_eq(g_hook_macro_counter, 1);
}
END_TEST

START_TEST(test_hook_macro_component_remove_fires)
{
	g_hook_macro_counter = 0;
	hook_macro_comp_remove_register(&g_world);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp = w_ecs_get_component_by_name(&g_world, "hm_comp_remove");
	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp, entity, &val, sizeof(val));
	w_ecs_remove_component_(&g_world, comp, entity);

	ck_assert_int_eq(g_hook_macro_counter, 1);
}
END_TEST

START_TEST(test_hook_macro_component_pre_set_fires)
{
	g_hook_macro_counter = 0;
	hook_macro_comp_pre_set_register(&g_world);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp = w_ecs_get_component_by_name(&g_world, "hm_pre_set");
	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp, entity, &val, sizeof(val));

	ck_assert_int_eq(g_hook_macro_counter, 1);
}
END_TEST

START_TEST(test_hook_macro_component_pre_remove_fires)
{
	g_hook_macro_counter = 0;
	hook_macro_comp_pre_remove_register(&g_world);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp = w_ecs_get_component_by_name(&g_world, "hm_pre_remove");
	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp, entity, &val, sizeof(val));
	w_ecs_remove_component_(&g_world, comp, entity);

	ck_assert_int_eq(g_hook_macro_counter, 1);
}
END_TEST

START_TEST(test_hook_macro_component_id_pre_set_fires)
{
	g_hook_macro_counter = 0;
	g_hook_macro_id_pre_set_comp_id = w_ecs_get_component_by_name(&g_world, "hm_id_pre_set");
	hook_macro_comp_id_pre_set_register(&g_world);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	int val = 1;
	w_ecs_set_component_(&g_world, 0, g_hook_macro_id_pre_set_comp_id, entity, &val, sizeof(val));

	ck_assert_int_eq(g_hook_macro_counter, 1);
}
END_TEST

START_TEST(test_hook_macro_component_id_pre_remove_fires)
{
	g_hook_macro_counter = 0;
	g_hook_macro_id_pre_remove_comp_id = w_ecs_get_component_by_name(&g_world, "hm_id_pre_remove");
	hook_macro_comp_id_pre_remove_register(&g_world);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	int val = 1;
	w_ecs_set_component_(&g_world, 0, g_hook_macro_id_pre_remove_comp_id, entity, &val, sizeof(val));
	w_ecs_remove_component_(&g_world, g_hook_macro_id_pre_remove_comp_id, entity);

	ck_assert_int_eq(g_hook_macro_counter, 1);
}
END_TEST

START_TEST(test_hook_macro_entity_create_fires)
{
	g_hook_macro_counter = 0;
	g_hook_macro_entity_id = W_ENTITY_INVALID;
	hook_macro_entity_create_register(&g_world);

	w_entity_id entity = w_ecs_request_entity(&g_world);

	ck_assert_int_eq(g_hook_macro_counter, 1);
	ck_assert_uint_eq(g_hook_macro_entity_id, entity);
}
END_TEST

START_TEST(test_hook_macro_entity_destroy_fires)
{
	g_hook_macro_counter = 0;
	g_hook_macro_entity_id = W_ENTITY_INVALID;
	hook_macro_entity_destroy_register(&g_world);

	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_ecs_return_entity(&g_world, entity);

	ck_assert_int_eq(g_hook_macro_counter, 1);
	ck_assert_uint_eq(g_hook_macro_entity_id, entity);
}
END_TEST


/*****************************
*  lifecycle hooks           *
*****************************/

static int g_startup_hook_counter;
static int g_restart_hook_counter;
static int g_shutdown_hook_counter;

w_ecs_startup_hook(test_startup_hook, {
	g_startup_hook_counter++;
})

w_ecs_restart_hook(test_restart_hook, {
	g_restart_hook_counter++;
})

w_ecs_shutdown_hook(test_shutdown_hook, {
	g_shutdown_hook_counter++;
})

START_TEST(test_startup_hook_fires_on_first_update)
{
	g_startup_hook_counter = 0;
	test_startup_hook_register(&g_world);

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	w_scheduler_register_time_step(&g_world.scheduler, &ts);

	ck_assert_int_eq(g_world.update_result, W_WORLD_UPDATE_RESULT_INIT);
	w_ecs_update(&g_world);

	ck_assert_int_eq(g_startup_hook_counter, 1);
	ck_assert_int_eq(g_world.update_result, W_WORLD_UPDATE_RESULT_CONTINUE);
}
END_TEST

START_TEST(test_startup_hook_fires_only_once)
{
	g_startup_hook_counter = 0;
	test_startup_hook_register(&g_world);

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	w_scheduler_register_time_step(&g_world.scheduler, &ts);

	w_ecs_update(&g_world);
	w_ecs_update(&g_world);
	w_ecs_update(&g_world);

	ck_assert_int_eq(g_startup_hook_counter, 1);
}
END_TEST

START_TEST(test_shutdown_hook_fires_on_shutdown)
{
	g_shutdown_hook_counter = 0;
	test_shutdown_hook_register(&g_world);

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	w_scheduler_register_time_step(&g_world.scheduler, &ts);

	// first update transitions from INIT to CONTINUE
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_shutdown_hook_counter, 0);

	// set shutdown and update
	g_world.update_result = W_WORLD_UPDATE_RESULT_SHUTDOWN;
	w_ecs_update(&g_world);

	ck_assert_int_eq(g_shutdown_hook_counter, 1);
}
END_TEST

START_TEST(test_restart_hook_fires_on_restart)
{
	g_restart_hook_counter = 0;
	test_restart_hook_register(&g_world);

	struct w_scheduler_time_step ts = {
		.enabled = true,
		.time_step = w_time_step_create(0, 1, true, true, true, true, true, true)
	};
	w_scheduler_register_time_step(&g_world.scheduler, &ts);

	// first update transitions from INIT to CONTINUE
	w_ecs_update(&g_world);
	ck_assert_int_eq(g_restart_hook_counter, 0);

	// set restart and update
	g_world.update_result = W_WORLD_UPDATE_RESULT_RESTART;
	w_ecs_update(&g_world);

	ck_assert_int_eq(g_restart_hook_counter, 1);
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
	tcase_add_test(tc_init, test_init_update_result_init);
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

	TCase *tc_buffered = tcase_create("buffered_entity_commands");
	tcase_add_checked_fixture(tc_buffered, world_setup, world_teardown);
	tcase_set_timeout(tc_buffered, 10);
	tcase_add_test(tc_buffered, test_buffered_entity_name_via_update);
	tcase_add_test(tc_buffered, test_unbuffered_return_entity);
	tcase_add_test(tc_buffered, test_buffered_return_entity);
	tcase_add_test(tc_buffered, test_unbuffered_clear_entity_name);
	tcase_add_test(tc_buffered, test_buffered_clear_entity_name);
	tcase_add_test(tc_buffered, test_unbuffered_set_component);
	tcase_add_test(tc_buffered, test_buffered_set_component);
	tcase_add_test(tc_buffered, test_unbuffered_remove_component);
	tcase_add_test(tc_buffered, test_buffered_remove_component);
	suite_add_tcase(s, tc_buffered);

	TCase *tc_sysexec = tcase_create("system_execution_verification");
	tcase_add_checked_fixture(tc_sysexec, world_setup, world_teardown);
	tcase_set_timeout(tc_sysexec, 10);
	tcase_add_test(tc_sysexec, test_system_executes_during_update);
	tcase_add_test(tc_sysexec, test_multiple_systems_execute_during_update);
	tcase_add_test(tc_sysexec, test_system_executes_multiple_updates);
	suite_add_tcase(s, tc_sysexec);

	TCase *tc_remove_all = tcase_create("remove_all_components");
	tcase_add_checked_fixture(tc_remove_all, world_setup, world_teardown);
	tcase_set_timeout(tc_remove_all, 10);
	tcase_add_test(tc_remove_all, test_remove_all_components_removes_all);
	tcase_add_test(tc_remove_all, test_remove_all_components_hooks_fired);
	tcase_add_test(tc_remove_all, test_remove_all_components_no_components_noop);
	tcase_add_test(tc_remove_all, test_remove_all_components_leaves_other_entities);
	suite_add_tcase(s, tc_remove_all);

	TCase *tc_destroy_hooks = tcase_create("entity_destroy_hooks");
	tcase_add_checked_fixture(tc_destroy_hooks, world_setup, world_teardown);
	tcase_set_timeout(tc_destroy_hooks, 10);
	tcase_add_test(tc_destroy_hooks, test_entity_destroy_hook_fires);
	tcase_add_test(tc_destroy_hooks, test_entity_destroy_hook_before_component_removal);
	tcase_add_test(tc_destroy_hooks, test_entity_destroy_hook_receives_correct_entity);
	tcase_add_test(tc_destroy_hooks, test_entity_destroy_hook_multiple_hooks_fire);
	tcase_add_test(tc_destroy_hooks, test_entity_destroy_hook_unregistered_not_fired);
	suite_add_tcase(s, tc_destroy_hooks);

	TCase *tc_module_res = tcase_create("module_resources");
	tcase_add_checked_fixture(tc_module_res, world_setup, world_teardown);
	tcase_set_timeout(tc_module_res, 10);
	tcase_add_test(tc_module_res, test_module_resource_set_get);
	tcase_add_test(tc_module_res, test_module_resource_multiple_slots);
	tcase_add_test(tc_module_res, test_module_resource_auto_growth);
	tcase_add_test(tc_module_res, test_module_resource_out_of_bounds_returns_null);
	tcase_add_test(tc_module_res, test_module_resource_clear);
	tcase_add_test(tc_module_res, test_module_resource_overwrite);
	suite_add_tcase(s, tc_module_res);

	TCase *tc_hook_macros = tcase_create("hook_definition_macros");
	tcase_add_checked_fixture(tc_hook_macros, world_setup, world_teardown);
	tcase_set_timeout(tc_hook_macros, 10);
	tcase_add_test(tc_hook_macros, test_hook_macro_update_begin_fires);
	tcase_add_test(tc_hook_macros, test_hook_macro_component_set_fires);
	tcase_add_test(tc_hook_macros, test_hook_macro_component_remove_fires);
	tcase_add_test(tc_hook_macros, test_hook_macro_component_pre_set_fires);
	tcase_add_test(tc_hook_macros, test_hook_macro_component_pre_remove_fires);
	tcase_add_test(tc_hook_macros, test_hook_macro_component_id_pre_set_fires);
	tcase_add_test(tc_hook_macros, test_hook_macro_component_id_pre_remove_fires);
	tcase_add_test(tc_hook_macros, test_hook_macro_entity_create_fires);
	tcase_add_test(tc_hook_macros, test_hook_macro_entity_destroy_fires);
	suite_add_tcase(s, tc_hook_macros);

	TCase *tc_lifecycle = tcase_create("lifecycle_hooks");
	tcase_add_checked_fixture(tc_lifecycle, world_setup, world_teardown);
	tcase_set_timeout(tc_lifecycle, 10);
	tcase_add_test(tc_lifecycle, test_startup_hook_fires_on_first_update);
	tcase_add_test(tc_lifecycle, test_startup_hook_fires_only_once);
	tcase_add_test(tc_lifecycle, test_shutdown_hook_fires_on_shutdown);
	tcase_add_test(tc_lifecycle, test_restart_hook_fires_on_restart);
	suite_add_tcase(s, tc_lifecycle);

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
