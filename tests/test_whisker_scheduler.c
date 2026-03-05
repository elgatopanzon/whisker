/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_scheduler
 * @created     : Wednesday Mar 04, 2026 17:32:46 CST
 * @description : tests for whisker_scheduler.h job scheduler
 */

#include "whisker_std.h"
#include "whisker_scheduler.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_scheduler g_scheduler;

static void scheduler_setup(void)
{
	w_scheduler_init(&g_scheduler);
}

static void scheduler_teardown(void)
{
	w_scheduler_free(&g_scheduler);
}


/*****************************
*  init and free             *
*****************************/

START_TEST(test_init_arrays_empty)
{
	ck_assert_int_eq(g_scheduler.time_steps_length, 0);
	ck_assert_int_eq(g_scheduler.phases_length, 0);
	ck_assert_int_eq(g_scheduler.time_steps_order_length, 0);
	ck_assert_int_eq(g_scheduler.phases_order_length, 0);
	ck_assert_int_eq(g_scheduler.schedule.items_length, 0);
}
END_TEST

START_TEST(test_init_schedule_dirty)
{
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_free_nulls_pointers)
{
	struct w_scheduler s;
	w_scheduler_init(&s);
	w_scheduler_free(&s);
	ck_assert_ptr_null(s.time_steps);
	ck_assert_ptr_null(s.phases);
	ck_assert_ptr_null(s.time_steps_order);
	ck_assert_ptr_null(s.phases_order);
	ck_assert_ptr_null(s.schedule.items);
}
END_TEST


/*****************************
*  phase register/get/clear  *
*****************************/

START_TEST(test_phase_register_returns_id)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t id = w_scheduler_register_phase(&g_scheduler, &phase);
	ck_assert_int_eq(id, 1);
}
END_TEST

START_TEST(test_phase_register_increments_ids)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t id1 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t id2 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t id3 = w_scheduler_register_phase(&g_scheduler, &phase);
	ck_assert_int_eq(id1, 1);
	ck_assert_int_eq(id2, 2);
	ck_assert_int_eq(id3, 3);
}
END_TEST

START_TEST(test_phase_register_marks_dirty)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_register_phase(&g_scheduler, &phase);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_phase_get_returns_registered)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 42};
	size_t id = w_scheduler_register_phase(&g_scheduler, &phase);
	struct w_scheduler_phase *got = w_scheduler_get_phase(&g_scheduler, id);
	ck_assert_ptr_nonnull(got);
	ck_assert(got->enabled);
	ck_assert_int_eq(got->time_step_id, 42);
}
END_TEST

START_TEST(test_phase_get_invalid_returns_null)
{
	struct w_scheduler_phase *got = w_scheduler_get_phase(&g_scheduler, 999);
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_phase_reset_clears_all)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	w_scheduler_register_phase(&g_scheduler, &phase);
	w_scheduler_register_phase(&g_scheduler, &phase);
	w_scheduler_register_phase(&g_scheduler, &phase);
	ck_assert_int_eq(g_scheduler.phases_length, 3);
	ck_assert_int_eq(g_scheduler.phases_order_length, 3);

	w_scheduler_reset_phases(&g_scheduler);
	ck_assert_int_eq(g_scheduler.phases_length, 0);
	ck_assert_int_eq(g_scheduler.phases_order_length, 0);
}
END_TEST

START_TEST(test_phase_reset_marks_dirty)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	w_scheduler_register_phase(&g_scheduler, &phase);
	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_reset_phases(&g_scheduler);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST


/*****************************
*  timestep register/get/clear
*****************************/

START_TEST(test_timestep_register_returns_id)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t id = w_scheduler_register_time_step(&g_scheduler, &ts);
	ck_assert_int_eq(id, 1);
}
END_TEST

START_TEST(test_timestep_register_increments_ids)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t id1 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t id2 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t id3 = w_scheduler_register_time_step(&g_scheduler, &ts);
	ck_assert_int_eq(id1, 1);
	ck_assert_int_eq(id2, 2);
	ck_assert_int_eq(id3, 3);
}
END_TEST

START_TEST(test_timestep_register_marks_dirty)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_register_time_step(&g_scheduler, &ts);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_timestep_get_returns_registered)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t id = w_scheduler_register_time_step(&g_scheduler, &ts);
	struct w_scheduler_time_step *got = w_scheduler_get_time_step(&g_scheduler, id);
	ck_assert_ptr_nonnull(got);
	ck_assert(got->enabled);
}
END_TEST

START_TEST(test_timestep_get_invalid_returns_null)
{
	struct w_scheduler_time_step *got = w_scheduler_get_time_step(&g_scheduler, 999);
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_timestep_reset_clears_all)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	w_scheduler_register_time_step(&g_scheduler, &ts);
	w_scheduler_register_time_step(&g_scheduler, &ts);
	ck_assert_int_eq(g_scheduler.time_steps_length, 2);
	ck_assert_int_eq(g_scheduler.time_steps_order_length, 2);

	w_scheduler_reset_time_steps(&g_scheduler);
	ck_assert_int_eq(g_scheduler.time_steps_length, 0);
	ck_assert_int_eq(g_scheduler.time_steps_order_length, 0);
}
END_TEST


/*****************************
*  phase state               *
*****************************/

START_TEST(test_phase_state_disable)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t id = w_scheduler_register_phase(&g_scheduler, &phase);
	struct w_scheduler_phase *got = w_scheduler_get_phase(&g_scheduler, id);
	ck_assert(got->enabled);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_phase_state(&g_scheduler, id, false);
	ck_assert(!got->enabled);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_phase_state_enable)
{
	struct w_scheduler_phase phase = {.enabled = false, .time_step_id = 0};
	size_t id = w_scheduler_register_phase(&g_scheduler, &phase);
	struct w_scheduler_phase *got = w_scheduler_get_phase(&g_scheduler, id);
	ck_assert(!got->enabled);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_phase_state(&g_scheduler, id, true);
	ck_assert(got->enabled);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_phase_state_same_not_dirty)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t id = w_scheduler_register_phase(&g_scheduler, &phase);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_phase_state(&g_scheduler, id, true);
	ck_assert(!g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_phase_state_invalid_noop)
{
	w_scheduler_set_phase_state(&g_scheduler, 999, true);
	// should not crash
}
END_TEST


/*****************************
*  timestep state            *
*****************************/

START_TEST(test_timestep_state_disable)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t id = w_scheduler_register_time_step(&g_scheduler, &ts);
	struct w_scheduler_time_step *got = w_scheduler_get_time_step(&g_scheduler, id);
	ck_assert(got->enabled);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_time_step_state(&g_scheduler, id, false);
	ck_assert(!got->enabled);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_timestep_state_enable)
{
	struct w_scheduler_time_step ts = {.enabled = false};
	size_t id = w_scheduler_register_time_step(&g_scheduler, &ts);
	struct w_scheduler_time_step *got = w_scheduler_get_time_step(&g_scheduler, id);
	ck_assert(!got->enabled);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_time_step_state(&g_scheduler, id, true);
	ck_assert(got->enabled);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_timestep_state_same_not_dirty)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t id = w_scheduler_register_time_step(&g_scheduler, &ts);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_time_step_state(&g_scheduler, id, true);
	ck_assert(!g_scheduler.schedule.schedule_dirty);
}
END_TEST


/*****************************
*  phase ordering            *
*****************************/

START_TEST(test_phase_runs_before_reorders)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t p1 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p2 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p3 = w_scheduler_register_phase(&g_scheduler, &phase);

	// initial order: p1, p2, p3
	ck_assert_int_eq(g_scheduler.phases_order[0], p1);
	ck_assert_int_eq(g_scheduler.phases_order[1], p2);
	ck_assert_int_eq(g_scheduler.phases_order[2], p3);

	// move p3 to run before p1
	w_scheduler_set_phase_runs_before(&g_scheduler, p3, p1);
	ck_assert_int_eq(g_scheduler.phases_order[0], p3);
	ck_assert_int_eq(g_scheduler.phases_order[1], p1);
	ck_assert_int_eq(g_scheduler.phases_order[2], p2);
}
END_TEST

START_TEST(test_phase_runs_after_reorders)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t p1 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p2 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p3 = w_scheduler_register_phase(&g_scheduler, &phase);

	// initial order: p1, p2, p3
	// move p1 to run after p3
	w_scheduler_set_phase_runs_after(&g_scheduler, p1, p3);
	ck_assert_int_eq(g_scheduler.phases_order[0], p2);
	ck_assert_int_eq(g_scheduler.phases_order[1], p3);
	ck_assert_int_eq(g_scheduler.phases_order[2], p1);
}
END_TEST

START_TEST(test_phase_runs_before_marks_dirty)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t p1 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p2 = w_scheduler_register_phase(&g_scheduler, &phase);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_phase_runs_before(&g_scheduler, p2, p1);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_phase_runs_after_marks_dirty)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t p1 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p2 = w_scheduler_register_phase(&g_scheduler, &phase);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_phase_runs_after(&g_scheduler, p1, p2);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_phase_runs_invalid_noop)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t p1 = w_scheduler_register_phase(&g_scheduler, &phase);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_phase_runs_before(&g_scheduler, p1, 999);
	ck_assert(!g_scheduler.schedule.schedule_dirty);

	w_scheduler_set_phase_runs_after(&g_scheduler, 999, p1);
	ck_assert(!g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_phase_runs_same_noop)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t p1 = w_scheduler_register_phase(&g_scheduler, &phase);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_phase_runs_before(&g_scheduler, p1, p1);
	ck_assert(!g_scheduler.schedule.schedule_dirty);
}
END_TEST


/*****************************
*  timestep ordering         *
*****************************/

START_TEST(test_timestep_runs_before_reorders)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t t1 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t2 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t3 = w_scheduler_register_time_step(&g_scheduler, &ts);

	// initial order: t1, t2, t3
	ck_assert_int_eq(g_scheduler.time_steps_order[0], t1);
	ck_assert_int_eq(g_scheduler.time_steps_order[1], t2);
	ck_assert_int_eq(g_scheduler.time_steps_order[2], t3);

	// move t3 to run before t1
	w_scheduler_set_time_step_runs_before(&g_scheduler, t3, t1);
	ck_assert_int_eq(g_scheduler.time_steps_order[0], t3);
	ck_assert_int_eq(g_scheduler.time_steps_order[1], t1);
	ck_assert_int_eq(g_scheduler.time_steps_order[2], t2);
}
END_TEST

START_TEST(test_timestep_runs_after_reorders)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t t1 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t2 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t3 = w_scheduler_register_time_step(&g_scheduler, &ts);

	// move t1 to run after t3
	w_scheduler_set_time_step_runs_after(&g_scheduler, t1, t3);
	ck_assert_int_eq(g_scheduler.time_steps_order[0], t2);
	ck_assert_int_eq(g_scheduler.time_steps_order[1], t3);
	ck_assert_int_eq(g_scheduler.time_steps_order[2], t1);
}
END_TEST

START_TEST(test_timestep_runs_before_marks_dirty)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t t1 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t2 = w_scheduler_register_time_step(&g_scheduler, &ts);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_time_step_runs_before(&g_scheduler, t2, t1);
	ck_assert(g_scheduler.schedule.schedule_dirty);
}
END_TEST

START_TEST(test_timestep_runs_invalid_noop)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t t1 = w_scheduler_register_time_step(&g_scheduler, &ts);

	g_scheduler.schedule.schedule_dirty = false;
	w_scheduler_set_time_step_runs_before(&g_scheduler, t1, 999);
	ck_assert(!g_scheduler.schedule.schedule_dirty);
}
END_TEST


/*****************************
*  schedule generation       *
*****************************/

START_TEST(test_schedule_empty_scheduler)
{
	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, NULL, 0);
	ck_assert_ptr_nonnull(sched);
	// should only have BEGIN and END
	ck_assert_int_eq(sched->items_length, 2);
	ck_assert_int_eq(sched->items[0].action, W_SCHEDULER_ACTIONS_SCHEDULE_BEGIN);
	ck_assert_int_eq(sched->items[1].action, W_SCHEDULER_ACTIONS_SCHEDULE_END);
}
END_TEST

START_TEST(test_schedule_single_timestep_no_phases)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, NULL, 0);
	// BEGIN, TIMESTEP_BEGIN, TIMESTEP_END, END
	ck_assert_int_eq(sched->items_length, 4);
	ck_assert_int_eq(sched->items[0].action, W_SCHEDULER_ACTIONS_SCHEDULE_BEGIN);
	ck_assert_int_eq(sched->items[1].action, W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN);
	ck_assert_int_eq(sched->items[2].action, W_SCHEDULER_ACTIONS_TIMESTEP_END);
	ck_assert_int_eq(sched->items[3].action, W_SCHEDULER_ACTIONS_SCHEDULE_END);
}
END_TEST

START_TEST(test_schedule_disabled_timestep_skipped)
{
	struct w_scheduler_time_step ts = {.enabled = false};
	w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, NULL, 0);
	// disabled timestep skipped, only BEGIN/END
	ck_assert_int_eq(sched->items_length, 2);
}
END_TEST

START_TEST(test_schedule_disabled_phase_skipped)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = false, .time_step_id = ts_id};
	w_scheduler_register_phase(&g_scheduler, &phase);

	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, NULL, 0);
	// BEGIN, TIMESTEP_BEGIN, TIMESTEP_END, END (no phase actions)
	ck_assert_int_eq(sched->items_length, 4);
}
END_TEST

START_TEST(test_schedule_single_phase_with_jobs)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_scheduler, &phase);

	struct w_scheduler_job jobs[] = {
		{.job_id = 100, .phase_id = phase_id},
		{.job_id = 200, .phase_id = phase_id},
	};

	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, jobs, 2);
	// BEGIN, TS_BEGIN, PHASE_BEGIN, DISPATCH, DISPATCH, PHASE_END, TS_END, END
	ck_assert_int_eq(sched->items_length, 8);
	ck_assert_int_eq(sched->items[0].action, W_SCHEDULER_ACTIONS_SCHEDULE_BEGIN);
	ck_assert_int_eq(sched->items[1].action, W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN);
	ck_assert_int_eq(sched->items[2].action, W_SCHEDULER_ACTIONS_PHASE_BEGIN);
	ck_assert_int_eq(sched->items[2].phase_id, phase_id);
	ck_assert_int_eq(sched->items[3].action, W_SCHEDULER_ACTIONS_DISPATCH);
	ck_assert_int_eq(sched->items[3].job_id, 100);
	ck_assert_int_eq(sched->items[4].action, W_SCHEDULER_ACTIONS_DISPATCH);
	ck_assert_int_eq(sched->items[4].job_id, 200);
	ck_assert_int_eq(sched->items[5].action, W_SCHEDULER_ACTIONS_PHASE_END);
	ck_assert_int_eq(sched->items[6].action, W_SCHEDULER_ACTIONS_TIMESTEP_END);
	ck_assert_int_eq(sched->items[7].action, W_SCHEDULER_ACTIONS_SCHEDULE_END);
}
END_TEST

START_TEST(test_schedule_not_dirty_reuses_cache)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_schedule *sched1 = w_scheduler_get_schedule(&g_scheduler, NULL, 0);
	ck_assert(!g_scheduler.schedule.schedule_dirty);

	// second call should return same cached schedule
	struct w_scheduler_schedule *sched2 = w_scheduler_get_schedule(&g_scheduler, NULL, 0);
	ck_assert_ptr_eq(sched1, sched2);
}
END_TEST

START_TEST(test_schedule_dirty_rebuilds)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_schedule *sched1 = w_scheduler_get_schedule(&g_scheduler, NULL, 0);
	size_t len1 = sched1->items_length;

	// register another timestep, marks dirty
	w_scheduler_register_time_step(&g_scheduler, &ts);
	ck_assert(g_scheduler.schedule.schedule_dirty);

	struct w_scheduler_schedule *sched2 = w_scheduler_get_schedule(&g_scheduler, NULL, 0);
	ck_assert_int_gt(sched2->items_length, len1);
	ck_assert(!g_scheduler.schedule.schedule_dirty);
}
END_TEST


/*****************************
*  multiple timesteps/phases *
*****************************/

START_TEST(test_multiple_timesteps_order)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t t1 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t2 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t3 = w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, NULL, 0);

	// find timestep begin actions and verify order
	size_t ts_order[3];
	size_t ts_count = 0;
	for (size_t i = 0; i < sched->items_length; i++) {
		if (sched->items[i].action == W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN) {
			ts_order[ts_count++] = sched->items[i].time_step_id;
		}
	}
	ck_assert_int_eq(ts_count, 3);
	ck_assert_int_eq(ts_order[0], t1);
	ck_assert_int_eq(ts_order[1], t2);
	ck_assert_int_eq(ts_order[2], t3);
}
END_TEST

START_TEST(test_multiple_timesteps_reordered)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t t1 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t2 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t t3 = w_scheduler_register_time_step(&g_scheduler, &ts);

	// reorder: t3 before t1
	w_scheduler_set_time_step_runs_before(&g_scheduler, t3, t1);

	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, NULL, 0);

	size_t ts_order[3];
	size_t ts_count = 0;
	for (size_t i = 0; i < sched->items_length; i++) {
		if (sched->items[i].action == W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN) {
			ts_order[ts_count++] = sched->items[i].time_step_id;
		}
	}
	ck_assert_int_eq(ts_count, 3);
	ck_assert_int_eq(ts_order[0], t3);
	ck_assert_int_eq(ts_order[1], t1);
	ck_assert_int_eq(ts_order[2], t2);
}
END_TEST

START_TEST(test_phases_match_timestep)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts1 = w_scheduler_register_time_step(&g_scheduler, &ts);
	size_t ts2 = w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_phase phase1 = {.enabled = true, .time_step_id = ts1};
	struct w_scheduler_phase phase2 = {.enabled = true, .time_step_id = ts2};
	size_t p1 = w_scheduler_register_phase(&g_scheduler, &phase1);
	size_t p2 = w_scheduler_register_phase(&g_scheduler, &phase2);

	struct w_scheduler_job jobs[] = {
		{.job_id = 10, .phase_id = p1},
		{.job_id = 20, .phase_id = p2},
	};

	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, jobs, 2);

	// verify jobs appear in correct timestep context
	bool in_ts1 = false, in_ts2 = false;
	bool job10_in_ts1 = false, job20_in_ts2 = false;

	for (size_t i = 0; i < sched->items_length; i++) {
		if (sched->items[i].action == W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN) {
			if (sched->items[i].time_step_id == ts1) in_ts1 = true;
			else if (sched->items[i].time_step_id == ts2) in_ts2 = true;
		}
		if (sched->items[i].action == W_SCHEDULER_ACTIONS_TIMESTEP_END) {
			if (in_ts1) in_ts1 = false;
			if (in_ts2) in_ts2 = false;
		}
		if (sched->items[i].action == W_SCHEDULER_ACTIONS_DISPATCH) {
			if (sched->items[i].job_id == 10 && in_ts1) job10_in_ts1 = true;
			if (sched->items[i].job_id == 20 && in_ts2) job20_in_ts2 = true;
		}
	}

	ck_assert(job10_in_ts1);
	ck_assert(job20_in_ts2);
}
END_TEST


/*****************************
*  edge cases                *
*****************************/

START_TEST(test_many_phases_stress)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_scheduler, &ts);

	const int count = 100;
	size_t phase_ids[100];
	for (int i = 0; i < count; i++) {
		struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
		phase_ids[i] = w_scheduler_register_phase(&g_scheduler, &phase);
	}

	ck_assert_int_eq(g_scheduler.phases_length, count);
	ck_assert_int_eq(g_scheduler.phases_order_length, count);

	// verify all phases retrievable
	for (int i = 0; i < count; i++) {
		struct w_scheduler_phase *p = w_scheduler_get_phase(&g_scheduler, phase_ids[i]);
		ck_assert_ptr_nonnull(p);
	}
}
END_TEST

START_TEST(test_many_timesteps_stress)
{
	const int count = 50;
	size_t ts_ids[50];
	for (int i = 0; i < count; i++) {
		struct w_scheduler_time_step ts = {.enabled = true};
		ts_ids[i] = w_scheduler_register_time_step(&g_scheduler, &ts);
	}

	ck_assert_int_eq(g_scheduler.time_steps_length, count);
	ck_assert_int_eq(g_scheduler.time_steps_order_length, count);

	// verify all retrievable
	for (int i = 0; i < count; i++) {
		struct w_scheduler_time_step *t = w_scheduler_get_time_step(&g_scheduler, ts_ids[i]);
		ck_assert_ptr_nonnull(t);
	}
}
END_TEST

START_TEST(test_complex_reordering)
{
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = 0};
	size_t p1 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p2 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p3 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p4 = w_scheduler_register_phase(&g_scheduler, &phase);
	size_t p5 = w_scheduler_register_phase(&g_scheduler, &phase);

	// initial: 1,2,3,4,5
	// move 5 before 2: 1,5,2,3,4
	w_scheduler_set_phase_runs_before(&g_scheduler, p5, p2);
	ck_assert_int_eq(g_scheduler.phases_order[0], p1);
	ck_assert_int_eq(g_scheduler.phases_order[1], p5);
	ck_assert_int_eq(g_scheduler.phases_order[2], p2);
	ck_assert_int_eq(g_scheduler.phases_order[3], p3);
	ck_assert_int_eq(g_scheduler.phases_order[4], p4);

	// move 1 after 4: 5,2,3,4,1
	w_scheduler_set_phase_runs_after(&g_scheduler, p1, p4);
	ck_assert_int_eq(g_scheduler.phases_order[0], p5);
	ck_assert_int_eq(g_scheduler.phases_order[1], p2);
	ck_assert_int_eq(g_scheduler.phases_order[2], p3);
	ck_assert_int_eq(g_scheduler.phases_order[3], p4);
	ck_assert_int_eq(g_scheduler.phases_order[4], p1);
}
END_TEST

START_TEST(test_jobs_no_matching_phase)
{
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_scheduler_register_time_step(&g_scheduler, &ts);

	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = ts_id};
	size_t phase_id = w_scheduler_register_phase(&g_scheduler, &phase);

	// job with non-matching phase_id
	struct w_scheduler_job jobs[] = {
		{.job_id = 100, .phase_id = 999},
	};

	struct w_scheduler_schedule *sched = w_scheduler_get_schedule(&g_scheduler, jobs, 1);
	// should have no DISPATCH actions
	for (size_t i = 0; i < sched->items_length; i++) {
		ck_assert_int_ne(sched->items[i].action, W_SCHEDULER_ACTIONS_DISPATCH);
	}
	(void)phase_id;
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_scheduler_suite(void)
{
	Suite *s = suite_create("whisker_scheduler");

	TCase *tc_init = tcase_create("init_free");
	tcase_add_checked_fixture(tc_init, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_arrays_empty);
	tcase_add_test(tc_init, test_init_schedule_dirty);
	tcase_add_test(tc_init, test_free_nulls_pointers);
	suite_add_tcase(s, tc_init);

	TCase *tc_phase = tcase_create("phase_management");
	tcase_add_checked_fixture(tc_phase, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_phase, 10);
	tcase_add_test(tc_phase, test_phase_register_returns_id);
	tcase_add_test(tc_phase, test_phase_register_increments_ids);
	tcase_add_test(tc_phase, test_phase_register_marks_dirty);
	tcase_add_test(tc_phase, test_phase_get_returns_registered);
	tcase_add_test(tc_phase, test_phase_get_invalid_returns_null);
	tcase_add_test(tc_phase, test_phase_reset_clears_all);
	tcase_add_test(tc_phase, test_phase_reset_marks_dirty);
	suite_add_tcase(s, tc_phase);

	TCase *tc_timestep = tcase_create("timestep_management");
	tcase_add_checked_fixture(tc_timestep, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_timestep, 10);
	tcase_add_test(tc_timestep, test_timestep_register_returns_id);
	tcase_add_test(tc_timestep, test_timestep_register_increments_ids);
	tcase_add_test(tc_timestep, test_timestep_register_marks_dirty);
	tcase_add_test(tc_timestep, test_timestep_get_returns_registered);
	tcase_add_test(tc_timestep, test_timestep_get_invalid_returns_null);
	tcase_add_test(tc_timestep, test_timestep_reset_clears_all);
	suite_add_tcase(s, tc_timestep);

	TCase *tc_phase_state = tcase_create("phase_state");
	tcase_add_checked_fixture(tc_phase_state, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_phase_state, 10);
	tcase_add_test(tc_phase_state, test_phase_state_disable);
	tcase_add_test(tc_phase_state, test_phase_state_enable);
	tcase_add_test(tc_phase_state, test_phase_state_same_not_dirty);
	tcase_add_test(tc_phase_state, test_phase_state_invalid_noop);
	suite_add_tcase(s, tc_phase_state);

	TCase *tc_ts_state = tcase_create("timestep_state");
	tcase_add_checked_fixture(tc_ts_state, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_ts_state, 10);
	tcase_add_test(tc_ts_state, test_timestep_state_disable);
	tcase_add_test(tc_ts_state, test_timestep_state_enable);
	tcase_add_test(tc_ts_state, test_timestep_state_same_not_dirty);
	suite_add_tcase(s, tc_ts_state);

	TCase *tc_phase_order = tcase_create("phase_ordering");
	tcase_add_checked_fixture(tc_phase_order, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_phase_order, 10);
	tcase_add_test(tc_phase_order, test_phase_runs_before_reorders);
	tcase_add_test(tc_phase_order, test_phase_runs_after_reorders);
	tcase_add_test(tc_phase_order, test_phase_runs_before_marks_dirty);
	tcase_add_test(tc_phase_order, test_phase_runs_after_marks_dirty);
	tcase_add_test(tc_phase_order, test_phase_runs_invalid_noop);
	tcase_add_test(tc_phase_order, test_phase_runs_same_noop);
	suite_add_tcase(s, tc_phase_order);

	TCase *tc_ts_order = tcase_create("timestep_ordering");
	tcase_add_checked_fixture(tc_ts_order, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_ts_order, 10);
	tcase_add_test(tc_ts_order, test_timestep_runs_before_reorders);
	tcase_add_test(tc_ts_order, test_timestep_runs_after_reorders);
	tcase_add_test(tc_ts_order, test_timestep_runs_before_marks_dirty);
	tcase_add_test(tc_ts_order, test_timestep_runs_invalid_noop);
	suite_add_tcase(s, tc_ts_order);

	TCase *tc_schedule = tcase_create("schedule_generation");
	tcase_add_checked_fixture(tc_schedule, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_schedule, 10);
	tcase_add_test(tc_schedule, test_schedule_empty_scheduler);
	tcase_add_test(tc_schedule, test_schedule_single_timestep_no_phases);
	tcase_add_test(tc_schedule, test_schedule_disabled_timestep_skipped);
	tcase_add_test(tc_schedule, test_schedule_disabled_phase_skipped);
	tcase_add_test(tc_schedule, test_schedule_single_phase_with_jobs);
	tcase_add_test(tc_schedule, test_schedule_not_dirty_reuses_cache);
	tcase_add_test(tc_schedule, test_schedule_dirty_rebuilds);
	suite_add_tcase(s, tc_schedule);

	TCase *tc_multi = tcase_create("multiple_timesteps_phases");
	tcase_add_checked_fixture(tc_multi, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_multiple_timesteps_order);
	tcase_add_test(tc_multi, test_multiple_timesteps_reordered);
	tcase_add_test(tc_multi, test_phases_match_timestep);
	suite_add_tcase(s, tc_multi);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_add_checked_fixture(tc_edge, scheduler_setup, scheduler_teardown);
	tcase_set_timeout(tc_edge, 10);
	tcase_add_test(tc_edge, test_many_phases_stress);
	tcase_add_test(tc_edge, test_many_timesteps_stress);
	tcase_add_test(tc_edge, test_complex_reordering);
	tcase_add_test(tc_edge, test_jobs_no_matching_phase);
	suite_add_tcase(s, tc_edge);

	return s;
}

int main(void)
{
	Suite *s = whisker_scheduler_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
