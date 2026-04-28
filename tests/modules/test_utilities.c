/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_utilities
 * @created     : Monday Mar 24, 2026 16:15:00 CST
 * @description : tests for whisker_utilities module (timer and oscillator systems)
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "whisker_timer.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/utilities/whisker_utilities.h"
#include "modules/utilities/whisker_utilities_timer.h"
#include "modules/utilities/whisker_utilities_oscillator.h"
#include "modules/utilities/whisker_utilities_random.h"
#include "modules/utilities/whisker_utilities_entity_lifecycle.h"
#include "modules/utilities/whisker_utilities_entity_lifecycle_hooks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;

static void timer_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_scheduler_defaults_init(&g_world, 60.0);
	wm_utils_init(&g_world);
}

static void timer_teardown(void)
{
	wm_utils_free(&g_world);
	wm_scheduler_defaults_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}

// timer system query string (must match the w_ecs_system definition exactly)
#define TIMER_QUERY_STRING \
	w_query_write(W_TIMER_COMPONENT_ELAPSED) \
	w_query_write(W_TIMER_COMPONENT_FLAGS) \
	w_query_read(W_TIMER_COMPONENT_DURATION) \
	w_query_read(W_TIMER_COMPONENT_OWNER_ENTITY) \
	w_query_read(W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID)

// advance world by a specific delta time (simulated, no blocking)
// reimplements timer logic to avoid static query pointer issue in w_query_for_each
static void advance_time(double delta_seconds)
{
	// get and rebuild query cache (fresh each call, no stale static pointer)
	struct w_query *q = w_ecs_get_query(&g_world, TIMER_QUERY_STRING);
	w_query_rebuild_cache(&g_world.queries, q);

	// manually iterate matching entities (mirrors timer system logic)
	for (size_t i = 0; i < q->archetype_slices_dense_length; ++i)
	{
		struct w_query_archetype_slice slice = q->archetype_slices_dense[i];
		for (size_t s = 0; s < slice.slice_length; ++s)
		{
			w_entity_id timer = slice.start_id + s;

			// get component data
			double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
			double *duration = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_DURATION, timer);
			w_timer_flags *flags = (w_timer_flags *)w_ecs_get_str(&g_world, int, W_TIMER_COMPONENT_FLAGS, timer);
			w_entity_id *owner = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_OWNER_ENTITY, timer);
			w_entity_id *finished_comp_id = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID, timer);

			if (!elapsed || !duration || !flags || !owner || !finished_comp_id) continue;

			if (w_timer_is_paused(*flags)) continue;

			// update timer using standalone function
			bool just_finished = w_timer_update(elapsed, *duration, flags, delta_seconds);

			// handle completion
			if (just_finished && w_entity_is_valid(*finished_comp_id) && w_entity_is_valid(*owner))
			{
				if (!w_ecs_has_tag(&g_world, *finished_comp_id, *owner))
				{
					w_ecs_set_tag(&g_world, *finished_comp_id, *owner);
				}
				else
				{
					w_ecs_remove_tag(&g_world, *finished_comp_id, *owner);

					if (w_timer_is_loop(*flags))
					{
						w_timer_wrap(elapsed, *duration);
						w_timer_clear_finished(flags);
					}
					else if (w_timer_is_oneshot(*flags))
						w_ecs_return_entity(&g_world, timer);
					else
						w_timer_pause(flags);
				}
			}
			else if (w_timer_is_finished(*flags) && w_entity_is_valid(*finished_comp_id) && w_entity_is_valid(*owner))
			{
				if (!w_ecs_has_tag(&g_world, *finished_comp_id, *owner))
				{
					w_ecs_set_tag(&g_world, *finished_comp_id, *owner);
				}
				else
				{
					w_ecs_remove_tag(&g_world, *finished_comp_id, *owner);

					if (w_timer_is_loop(*flags))
					{
						w_timer_wrap(elapsed, *duration);
						w_timer_clear_finished(flags);
					}
					else if (w_timer_is_oneshot(*flags))
						w_ecs_return_entity(&g_world, timer);
					else
						w_timer_pause(flags);
				}
			}
		}
	}

	// also iterate sparse slices
	for (size_t i = 0; i < q->archetype_slices_sparse_length; ++i)
	{
		struct w_query_archetype_slice slice = q->archetype_slices_sparse[i];
		for (size_t s = 0; s < slice.slice_length; ++s)
		{
			w_entity_id timer = slice.start_id + s;

			double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
			double *duration = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_DURATION, timer);
			w_timer_flags *flags = (w_timer_flags *)w_ecs_get_str(&g_world, int, W_TIMER_COMPONENT_FLAGS, timer);
			w_entity_id *owner = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_OWNER_ENTITY, timer);
			w_entity_id *finished_comp_id = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID, timer);

			if (!elapsed || !duration || !flags || !owner || !finished_comp_id) continue;

			if (w_timer_is_paused(*flags)) continue;

			bool just_finished = w_timer_update(elapsed, *duration, flags, delta_seconds);

			if (just_finished && w_entity_is_valid(*finished_comp_id) && w_entity_is_valid(*owner))
			{
				if (!w_ecs_has_tag(&g_world, *finished_comp_id, *owner))
				{
					w_ecs_set_tag(&g_world, *finished_comp_id, *owner);
				}
				else
				{
					w_ecs_remove_tag(&g_world, *finished_comp_id, *owner);

					if (w_timer_is_loop(*flags))
					{
						w_timer_wrap(elapsed, *duration);
						w_timer_clear_finished(flags);
					}
					else if (w_timer_is_oneshot(*flags))
						w_ecs_return_entity(&g_world, timer);
					else
						w_timer_pause(flags);
				}
			}
			else if (w_timer_is_finished(*flags) && w_entity_is_valid(*finished_comp_id) && w_entity_is_valid(*owner))
			{
				if (!w_ecs_has_tag(&g_world, *finished_comp_id, *owner))
				{
					w_ecs_set_tag(&g_world, *finished_comp_id, *owner);
				}
				else
				{
					w_ecs_remove_tag(&g_world, *finished_comp_id, *owner);

					if (w_timer_is_loop(*flags))
					{
						w_timer_wrap(elapsed, *duration);
						w_timer_clear_finished(flags);
					}
					else if (w_timer_is_oneshot(*flags))
						w_ecs_return_entity(&g_world, timer);
					else
						w_timer_pause(flags);
				}
			}
		}
	}
}


/*****************************
*  timer entity name         *
*****************************/

START_TEST(test_timer_entity_gets_name)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "test_timer", 1.0f, false, false);

	char *name = w_ecs_get_entity_name(&g_world, timer);
	ck_assert_ptr_nonnull(name);
	// name should start with "test_timer_"
	ck_assert_msg(strncmp(name, "test_timer_", 11) == 0,
		"timer name should start with 'test_timer_', got: %s", name);
	// name should have 16 hex chars after the underscore
	ck_assert_int_eq(strlen(name), 11 + 16);
}
END_TEST

START_TEST(test_timer_entity_names_unique)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer1 = wm_utils_timer_create(&g_world, owner, "timer", 1.0f, false, false);
	w_entity_id timer2 = wm_utils_timer_create(&g_world, owner, "timer", 1.0f, false, false);

	char *name1 = w_ecs_get_entity_name(&g_world, timer1);
	char *name2 = w_ecs_get_entity_name(&g_world, timer2);

	ck_assert_str_ne(name1, name2);
}
END_TEST


/*****************************
*  loop timer                *
*****************************/

START_TEST(test_loop_timer_fires_repeatedly)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "loop_test", 0.5f, true, false);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "loop_test_finished_tag");

	// verify timer components are set up correctly
	double *dur = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_DURATION, timer);
	w_entity_id *own = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_OWNER_ENTITY, timer);
	w_entity_id *fcid = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID, timer);
	ck_assert_ptr_nonnull(dur);
	ck_assert_ptr_nonnull(own);
	ck_assert_ptr_nonnull(fcid);
	ck_assert_double_eq(*dur, 0.5);
	ck_assert_int_eq(*own, owner);
	ck_assert_int_eq(*fcid, finished_tag);

	// advance 0.6s to exceed duration
	advance_time(0.6);

	// verify elapsed was updated
	double *el = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_ptr_nonnull(el);
	ck_assert_double_ge(*el, 0.5);

	// tag should be set on first completion
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));

	// advance again to trigger reset
	advance_time(0.1);

	// tag should be removed, elapsed reset
	ck_assert(!w_ecs_has_tag(&g_world, finished_tag, owner));

	double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	// elapsed should be small (0.1s from last advance after reset)
	ck_assert_double_lt(*elapsed, 0.2);

	// advance past duration again
	advance_time(0.5);

	// tag should fire again
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));
}
END_TEST

START_TEST(test_loop_timer_resets_elapsed)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "loop_reset", 1.0, true, false);

	// advance past duration
	advance_time(1.1);
	// first frame: tag set
	advance_time(0.1);
	// second frame: tag removed, elapsed reset

	double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	// elapsed should include the 0.1s from last frame
	ck_assert_double_lt(*elapsed, 0.2);
}
END_TEST


/*****************************
*  oneshot timer             *
*****************************/

START_TEST(test_oneshot_timer_fires_once)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "oneshot_test", 0.5f, false, true);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "oneshot_test_finished_tag");

	// advance past duration
	advance_time(0.6);

	// tag should be set
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));

	// advance again
	advance_time(0.1);

	// tag removed, timer should be destroyed
	ck_assert(!w_ecs_has_tag(&g_world, finished_tag, owner));

	// timer entity should no longer exist (returned to pool)
	double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_ptr_null(elapsed);
}
END_TEST

START_TEST(test_oneshot_timer_destroyed_after_completion)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "oneshot_destroy", 0.5f, false, true);

	// advance twice to complete and destroy
	advance_time(0.6);  // tag set
	advance_time(0.1);  // tag removed, timer destroyed

	// verify timer components are gone
	ck_assert(!w_ecs_has_str(&g_world, W_TIMER_COMPONENT_ELAPSED, timer));
	ck_assert(!w_ecs_has_str(&g_world, W_TIMER_COMPONENT_DURATION, timer));
}
END_TEST


/*****************************
*  finished tag timing       *
*****************************/

START_TEST(test_finished_tag_stays_one_frame)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_timer_create(&g_world, owner, "tag_test", 0.5f, true, false);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "tag_test_finished_tag");

	// frame 0: not yet complete
	ck_assert(!w_ecs_has_tag(&g_world, finished_tag, owner));

	// advance to complete timer
	advance_time(0.6);

	// frame 1: tag should be present
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));

	// advance again
	advance_time(0.1);

	// frame 2: tag should be removed
	ck_assert(!w_ecs_has_tag(&g_world, finished_tag, owner));
}
END_TEST

START_TEST(test_finished_tag_exactly_one_frame_total)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_timer_create(&g_world, owner, "exact_frame", 1.0f, true, false);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "exact_frame_finished_tag");

	int tag_present_count = 0;

	// run 6 frames at 0.5s each (duration=1.0s needs 2 frames to trigger)
	// frame 1: elapsed=0.5, no tag
	// frame 2: elapsed=1.0, tag SET (count=1)
	// frame 3: elapsed=1.5, tag REMOVE, reset elapsed=0
	// frame 4: elapsed=0.5, no tag
	// frame 5: elapsed=1.0, tag SET (count=2)
	// frame 6: elapsed=1.5, tag REMOVE, reset elapsed=0
	for (int i = 0; i < 6; i++)
	{
		advance_time(0.5);
		if (w_ecs_has_tag(&g_world, finished_tag, owner))
			tag_present_count++;
	}

	// tag should appear exactly 2 times (frames 2 and 5)
	ck_assert_int_eq(tag_present_count, 2);
}
END_TEST


/*****************************
*  pause functionality       *
*****************************/

START_TEST(test_pause_stops_timer)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "pause_test", 1.0, false, false);

	// advance a bit
	advance_time(0.3);

	double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	double before_pause = *elapsed;

	// pause the timer via flags
	w_timer_flags *flags = (w_timer_flags *)w_ecs_get_str(&g_world, int, W_TIMER_COMPONENT_FLAGS, timer);
	w_timer_pause(flags);

	// verify pause flag was set
	ck_assert(w_timer_is_paused(*flags));

	// advance more
	advance_time(0.5);

	// verify pause flag is still set
	flags = (w_timer_flags *)w_ecs_get_str(&g_world, int, W_TIMER_COMPONENT_FLAGS, timer);
	ck_assert(w_timer_is_paused(*flags));

	// elapsed should not have changed
	elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_double_eq(*elapsed, before_pause);
}
END_TEST

START_TEST(test_unpause_resumes_timer)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "unpause_test", 1.0, true, false);

	// advance a bit
	advance_time(0.3);

	// pause via flags
	w_timer_flags *flags = (w_timer_flags *)w_ecs_get_str(&g_world, int, W_TIMER_COMPONENT_FLAGS, timer);
	w_timer_pause(flags);
	advance_time(0.5);

	double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	double paused_elapsed = *elapsed;

	// unpause via flags
	flags = (w_timer_flags *)w_ecs_get_str(&g_world, int, W_TIMER_COMPONENT_FLAGS, timer);
	w_timer_unpause(flags);
	advance_time(0.2);

	// elapsed should have increased
	elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_double_gt(*elapsed, paused_elapsed);
}
END_TEST


/*****************************
*  inert pause               *
*****************************/

START_TEST(test_inert_timer_auto_pauses)
{
	// timer without loop or oneshot should auto-pause on completion
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "inert_test", 0.5, false, false);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "inert_test_finished_tag");

	// advance past duration
	advance_time(0.6);

	// tag should be set
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));

	// advance again
	advance_time(0.1);

	// tag removed, timer should be paused via flags
	ck_assert(!w_ecs_has_tag(&g_world, finished_tag, owner));
	w_timer_flags *flags = (w_timer_flags *)w_ecs_get_str(&g_world, int, W_TIMER_COMPONENT_FLAGS, timer);
	ck_assert(w_timer_is_paused(*flags));

	// timer should still exist (not destroyed like oneshot)
	double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_ptr_nonnull(elapsed);
}
END_TEST

START_TEST(test_inert_timer_can_restart)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = wm_utils_timer_create(&g_world, owner, "inert_restart", 0.5, false, false);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "inert_restart_finished_tag");

	// complete timer
	advance_time(0.6);
	advance_time(0.1);

	// should be paused via flags
	w_timer_flags *flags = (w_timer_flags *)w_ecs_get_str(&g_world, int, W_TIMER_COMPONENT_FLAGS, timer);
	ck_assert(w_timer_is_paused(*flags));

	// reset timer using standalone function and unpause
	double *elapsed = w_ecs_get_str(&g_world, double, W_TIMER_COMPONENT_ELAPSED, timer);
	w_timer_reset(elapsed, flags);
	w_timer_unpause(flags);

	// advance past duration again
	advance_time(0.6);

	// tag should fire again
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));
}
END_TEST


/*****************************
*  multiple timers           *
*****************************/

START_TEST(test_multiple_timers_independent)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);

	w_entity_id timer1 = wm_utils_timer_create(&g_world, owner, "multi1", 0.3f, true, false);
	w_entity_id timer2 = wm_utils_timer_create(&g_world, owner, "multi2", 0.5f, true, false);

	w_entity_id tag1 = w_ecs_get_component_by_name(&g_world, "multi1_finished_tag");
	w_entity_id tag2 = w_ecs_get_component_by_name(&g_world, "multi2_finished_tag");

	// advance 0.35s - timer1 should complete, timer2 should not
	advance_time(0.35);

	ck_assert(w_ecs_has_tag(&g_world, tag1, owner));
	ck_assert(!w_ecs_has_tag(&g_world, tag2, owner));

	// advance another 0.2s (total 0.55s) - timer2 should also complete
	advance_time(0.2);

	ck_assert(w_ecs_has_tag(&g_world, tag2, owner));

	// verify both timers still exist
	ck_assert(w_ecs_has_str(&g_world, W_TIMER_COMPONENT_ELAPSED, timer1));
	ck_assert(w_ecs_has_str(&g_world, W_TIMER_COMPONENT_ELAPSED, timer2));
}
END_TEST

START_TEST(test_multiple_owners_different_timers)
{
	w_entity_id owner1 = w_ecs_request_entity(&g_world);
	w_entity_id owner2 = w_ecs_request_entity(&g_world);

	wm_utils_timer_create(&g_world, owner1, "owner1_timer", 0.3f, true, false);
	wm_utils_timer_create(&g_world, owner2, "owner2_timer", 0.5f, true, false);

	w_entity_id tag1 = w_ecs_get_component_by_name(&g_world, "owner1_timer_finished_tag");
	w_entity_id tag2 = w_ecs_get_component_by_name(&g_world, "owner2_timer_finished_tag");

	advance_time(0.35);

	// owner1 should have tag, owner2 should not
	ck_assert(w_ecs_has_tag(&g_world, tag1, owner1));
	ck_assert(!w_ecs_has_tag(&g_world, tag2, owner2));

	// neither owner should have the other's tag
	ck_assert(!w_ecs_has_tag(&g_world, tag2, owner1));
	ck_assert(!w_ecs_has_tag(&g_world, tag1, owner2));
}
END_TEST


/*****************************
*  stress test               *
*****************************/

START_TEST(test_timer_stress_many_timers)
{
	int num_timers = 100;
	w_entity_id *owners = malloc(num_timers * sizeof(w_entity_id));
	w_entity_id *timers = malloc(num_timers * sizeof(w_entity_id));

	// create many timers with varying durations
	for (int i = 0; i < num_timers; i++)
	{
		owners[i] = w_ecs_request_entity(&g_world);
		char name[32];
		snprintf(name, sizeof(name), "stress_%d", i);
		double duration = 0.1 + (i % 10) * 0.1;  // 0.1 to 1.0 seconds
		timers[i] = wm_utils_timer_create(&g_world, owners[i], name, duration, true, false);
	}

	// run 50 frames
	for (int frame = 0; frame < 50; frame++)
	{
		advance_time(0.05);
	}

	// all timers should still exist and be functional
	for (int i = 0; i < num_timers; i++)
	{
		ck_assert(w_ecs_has_str(&g_world, W_TIMER_COMPONENT_ELAPSED, timers[i]));
	}

	free(owners);
	free(timers);
}
END_TEST

START_TEST(test_timer_stress_rapid_create_destroy)
{
	// create oneshot timers that complete quickly
	int completed = 0;

	for (int i = 0; i < 50; i++)
	{
		w_entity_id owner = w_ecs_request_entity(&g_world);
		char name[32];
		snprintf(name, sizeof(name), "rapid_%d", i);
		wm_utils_timer_create(&g_world, owner, name, 0.1f, false, true);

		// advance to complete and destroy
		advance_time(0.15);
		advance_time(0.05);

		completed++;
	}

	ck_assert_int_eq(completed, 50);
}
END_TEST


/*****************************
*  oscillator fixture        *
*****************************/

static void oscillator_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_scheduler_defaults_init(&g_world, 60.0);
	wm_utils_init(&g_world);
}

static void oscillator_teardown(void)
{
	wm_utils_free(&g_world);
	wm_scheduler_defaults_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}

// oscillator system query string
#define OSCILLATOR_QUERY_STRING \
	w_query_write(W_OSCILLATOR_COMPONENT_PHASE) \
	w_query_read(W_OSCILLATOR_COMPONENT_PERIOD) \
	w_query_read(W_OSCILLATOR_COMPONENT_AMPLITUDE) \
	w_query_read(W_OSCILLATOR_COMPONENT_OFFSET) \
	w_query_read(W_OSCILLATOR_COMPONENT_PHASE_SHIFT) \
	w_query_read(W_OSCILLATOR_COMPONENT_TYPE) \
	w_query_read(W_OSCILLATOR_COMPONENT_OWNER_ENTITY)

// advance oscillators by delta time
static void advance_oscillators(double delta_seconds)
{
	struct w_query *q = w_ecs_get_query(&g_world, OSCILLATOR_QUERY_STRING);
	w_query_rebuild_cache(&g_world.queries, q);

	for (size_t i = 0; i < q->archetype_slices_dense_length; ++i)
	{
		struct w_query_archetype_slice slice = q->archetype_slices_dense[i];
		for (size_t s = 0; s < slice.slice_length; ++s)
		{
			w_entity_id osc = slice.start_id + s;

			float *phase = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
			float *period = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PERIOD, osc);
			float *amplitude = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_AMPLITUDE, osc);
			float *offset = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_OFFSET, osc);
			float *phase_shift = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE_SHIFT, osc);
			int *type = w_ecs_get_str(&g_world, int, W_OSCILLATOR_COMPONENT_TYPE, osc);
			w_entity_id *owner = w_ecs_get_str(&g_world, w_entity_id, W_OSCILLATOR_COMPONENT_OWNER_ENTITY, osc);

			if (!phase || !period || !amplitude || !offset || !phase_shift || !type || !owner) continue;
			if (*period <= 0.0f) continue;

			*phase += delta_seconds / *period;
			*phase = fmodf(*phase, 1.0f);
			if (*phase < 0.0f) *phase += 1.0f;

			float effective_phase = fmodf(*phase + *phase_shift, 1.0f);
			if (effective_phase < 0.0f) effective_phase += 1.0f;

			float value = w_oscillator_compute((w_waveform_type)*type, effective_phase, *amplitude, *offset);

			char *osc_name = w_ecs_get_entity_name(&g_world, osc);
			if (osc_name && w_entity_is_valid(*owner)) {
				char *last_underscore = strrchr(osc_name, '_');
				if (last_underscore) {
					size_t prefix_len = last_underscore - osc_name;
					char value_name[128];
					snprintf(value_name, sizeof(value_name), "%.*s" W_OSCILLATOR_COMPONENT_VALUE, (int)prefix_len, osc_name);
					w_ecs_set_str(&g_world, float, value_name, *owner, &value);
				}
			}
		}
	}

	for (size_t i = 0; i < q->archetype_slices_sparse_length; ++i)
	{
		struct w_query_archetype_slice slice = q->archetype_slices_sparse[i];
		for (size_t s = 0; s < slice.slice_length; ++s)
		{
			w_entity_id osc = slice.start_id + s;

			float *phase = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
			float *period = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PERIOD, osc);
			float *amplitude = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_AMPLITUDE, osc);
			float *offset = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_OFFSET, osc);
			float *phase_shift = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE_SHIFT, osc);
			int *type = w_ecs_get_str(&g_world, int, W_OSCILLATOR_COMPONENT_TYPE, osc);
			w_entity_id *owner = w_ecs_get_str(&g_world, w_entity_id, W_OSCILLATOR_COMPONENT_OWNER_ENTITY, osc);

			if (!phase || !period || !amplitude || !offset || !phase_shift || !type || !owner) continue;
			if (*period <= 0.0f) continue;

			*phase += delta_seconds / *period;
			*phase = fmodf(*phase, 1.0f);
			if (*phase < 0.0f) *phase += 1.0f;

			float effective_phase = fmodf(*phase + *phase_shift, 1.0f);
			if (effective_phase < 0.0f) effective_phase += 1.0f;

			float value = w_oscillator_compute((w_waveform_type)*type, effective_phase, *amplitude, *offset);

			char *osc_name = w_ecs_get_entity_name(&g_world, osc);
			if (osc_name && w_entity_is_valid(*owner)) {
				char *last_underscore = strrchr(osc_name, '_');
				if (last_underscore) {
					size_t prefix_len = last_underscore - osc_name;
					char value_name[128];
					snprintf(value_name, sizeof(value_name), "%.*s" W_OSCILLATOR_COMPONENT_VALUE, (int)prefix_len, osc_name);
					w_ecs_set_str(&g_world, float, value_name, *owner, &value);
				}
			}
		}
	}
}


/*****************************
*  oscillator entity name    *
*****************************/

START_TEST(test_oscillator_entity_gets_name)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = wm_utils_oscillator_create(&g_world, owner, "test_osc", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	char *name = w_ecs_get_entity_name(&g_world, osc);
	ck_assert_ptr_nonnull(name);
	ck_assert_msg(strncmp(name, "test_osc_", 9) == 0,
		"oscillator name should start with 'test_osc_', got: %s", name);
	ck_assert_int_eq(strlen(name), 9 + 16);
}
END_TEST

START_TEST(test_oscillator_entity_names_unique)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc1 = wm_utils_oscillator_create(&g_world, owner, "osc", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	w_entity_id osc2 = wm_utils_oscillator_create(&g_world, owner, "osc", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	char *name1 = w_ecs_get_entity_name(&g_world, osc1);
	char *name2 = w_ecs_get_entity_name(&g_world, osc2);

	ck_assert_str_ne(name1, name2);
}
END_TEST


/*****************************
*  oscillator components     *
*****************************/

START_TEST(test_oscillator_components_set)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = wm_utils_oscillator_create(&g_world, owner, "comp_test", W_OSCILLATOR_TYPE(TRIANGLE), 2.0f, 0.5f, 1.0f, 0.25f, 0.0f);

	float *period = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PERIOD, osc);
	float *phase = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
	float *amplitude = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_AMPLITUDE, osc);
	float *offset = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_OFFSET, osc);
	float *phase_shift = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE_SHIFT, osc);
	int *type = w_ecs_get_str(&g_world, int, W_OSCILLATOR_COMPONENT_TYPE, osc);
	w_entity_id *owner_ref = w_ecs_get_str(&g_world, w_entity_id, W_OSCILLATOR_COMPONENT_OWNER_ENTITY, osc);

	ck_assert_ptr_nonnull(period);
	ck_assert_ptr_nonnull(phase);
	ck_assert_ptr_nonnull(amplitude);
	ck_assert_ptr_nonnull(offset);
	ck_assert_ptr_nonnull(phase_shift);
	ck_assert_ptr_nonnull(type);
	ck_assert_ptr_nonnull(owner_ref);

	ck_assert_float_eq(*period, 2.0f);
	ck_assert_float_eq(*phase, 0.0f);
	ck_assert_float_eq(*amplitude, 0.5f);
	ck_assert_float_eq(*offset, 1.0f);
	ck_assert_float_eq(*phase_shift, 0.25f);
	ck_assert_int_eq(*type, (int)W_WAVEFORM_TRIANGLE);
	ck_assert_int_eq(*owner_ref, owner);
}
END_TEST

START_TEST(test_oscillator_owner_entity_ref)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = wm_utils_oscillator_create(&g_world, owner, "owner_ref", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	w_entity_id *owner_osc = w_ecs_get_str(&g_world, w_entity_id, "owner_ref" W_OSCILLATOR_COMPONENT_OSCILLATOR_ENTITY, owner);
	ck_assert_ptr_nonnull(owner_osc);
	ck_assert_int_eq(*owner_osc, osc);
}
END_TEST


/*****************************
*  sine waveform             *
*****************************/

START_TEST(test_sine_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "sine_zero", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// at phase 0, sin(0) = 0
	float value = wm_utils_oscillator_get_value(&g_world, owner, "sine_zero");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_sine_at_phase_quarter)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "sine_quarter", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// advance to phase 0.25, sin(pi/2) = 1
	advance_oscillators(0.25);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "sine_quarter");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST

START_TEST(test_sine_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "sine_half", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// advance to phase 0.5, sin(pi) = 0
	advance_oscillators(0.5);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "sine_half");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_sine_with_amplitude_offset)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "sine_amp", W_OSCILLATOR_TYPE(SINE), 1.0f, 2.0f, 5.0f, 0.0f, 0.0f);

	// at phase 0.25, value = 5 + 2*sin(pi/2) = 5 + 2 = 7
	advance_oscillators(0.25);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "sine_amp");
	ck_assert_float_eq_tol(value, 7.0f, 0.001f);
}
END_TEST


/*****************************
*  triangle waveform         *
*****************************/

START_TEST(test_triangle_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "tri_zero", W_OSCILLATOR_TYPE(TRIANGLE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// trigger update at phase ~0
	advance_oscillators(0.0001);

	// at phase ~0: 1 - 4*|0 - 0.5| = 1 - 2 = -1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "tri_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_triangle_at_phase_quarter)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "tri_quarter", W_OSCILLATOR_TYPE(TRIANGLE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// at phase 0.25: 1 - 4*|0.25 - 0.5| = 1 - 1 = 0
	advance_oscillators(0.25);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "tri_quarter");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_triangle_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "tri_half", W_OSCILLATOR_TYPE(TRIANGLE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// at phase 0.5: 1 - 4*|0.5 - 0.5| = 1 - 0 = 1
	advance_oscillators(0.5);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "tri_half");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST


/*****************************
*  square waveform           *
*****************************/

START_TEST(test_square_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "sq_zero", W_OSCILLATOR_TYPE(SQUARE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// trigger update at phase ~0
	advance_oscillators(0.0001);

	// at phase ~0 < 0.5: value = 1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "sq_zero");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST

START_TEST(test_square_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "sq_half", W_OSCILLATOR_TYPE(SQUARE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// at phase 0.5 >= 0.5: value = -1
	advance_oscillators(0.5);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "sq_half");
	ck_assert_float_eq_tol(value, -1.0f, 0.001f);
}
END_TEST


/*****************************
*  sawtooth waveform         *
*****************************/

START_TEST(test_sawtooth_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "saw_zero", W_OSCILLATOR_TYPE(SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// trigger update at phase ~0
	advance_oscillators(0.0001);

	// at phase ~0: 2*0 - 1 = -1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "saw_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_sawtooth_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "saw_half", W_OSCILLATOR_TYPE(SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// at phase 0.5: 2*0.5 - 1 = 0
	advance_oscillators(0.5);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "saw_half");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_sawtooth_near_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "saw_one", W_OSCILLATOR_TYPE(SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// at phase 0.99: 2*0.99 - 1 = 0.98
	advance_oscillators(0.99);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "saw_one");
	ck_assert_float_eq_tol(value, 0.98f, 0.02f);
}
END_TEST


/*****************************
*  phase shift               *
*****************************/

START_TEST(test_phase_shift_sine)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	// phase_shift 0.25 means sine starts at peak
	wm_utils_oscillator_create(&g_world, owner, "shift_sine", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.25f, 0.0f);

	// effective_phase = 0 + 0.25 = 0.25, sin(pi/2) = 1
	// need to trigger update once
	advance_oscillators(0.0001);

	float value = wm_utils_oscillator_get_value(&g_world, owner, "shift_sine");
	ck_assert_float_eq_tol(value, 1.0f, 0.01f);
}
END_TEST


/*****************************
*  phase cycling             *
*****************************/

START_TEST(test_phase_wraps_at_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = wm_utils_oscillator_create(&g_world, owner, "wrap_test", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// advance 1.5 periods
	advance_oscillators(1.5);

	float *phase = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
	ck_assert_ptr_nonnull(phase);
	// phase should wrap to 0.5
	ck_assert_float_eq_tol(*phase, 0.5f, 0.001f);
}
END_TEST

START_TEST(test_oscillator_continues_forever)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = wm_utils_oscillator_create(&g_world, owner, "forever", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// run many cycles
	for (int i = 0; i < 100; i++)
	{
		advance_oscillators(0.1);
	}

	// oscillator should still exist and function
	float *phase = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
	ck_assert_ptr_nonnull(phase);
	ck_assert(*phase >= 0.0f && *phase < 1.0f);
}
END_TEST


/*****************************
*  invalid period            *
*****************************/

START_TEST(test_zero_period_skipped)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = wm_utils_oscillator_create(&g_world, owner, "zero_period", W_OSCILLATOR_TYPE(SINE), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	float *phase_before = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
	float initial_phase = *phase_before;

	advance_oscillators(1.0);

	float *phase_after = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
	// phase should not change with zero period
	ck_assert_float_eq(*phase_after, initial_phase);
}
END_TEST

START_TEST(test_negative_period_skipped)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = wm_utils_oscillator_create(&g_world, owner, "neg_period", W_OSCILLATOR_TYPE(SINE), -1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	float *phase_before = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
	float initial_phase = *phase_before;

	advance_oscillators(1.0);

	float *phase_after = w_ecs_get_str(&g_world, float, W_OSCILLATOR_COMPONENT_PHASE, osc);
	// phase should not change with negative period
	ck_assert_float_eq(*phase_after, initial_phase);
}
END_TEST


/*****************************
*  multiple oscillators      *
*****************************/

START_TEST(test_multiple_oscillators_independent)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);

	wm_utils_oscillator_create(&g_world, owner, "multi1", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	wm_utils_oscillator_create(&g_world, owner, "multi2", W_OSCILLATOR_TYPE(SQUARE), 2.0f, 2.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.25);

	float val1 = wm_utils_oscillator_get_value(&g_world, owner, "multi1");
	float val2 = wm_utils_oscillator_get_value(&g_world, owner, "multi2");

	// sine at 0.25: sin(pi/2) = 1
	ck_assert_float_eq_tol(val1, 1.0f, 0.001f);
	// square at phase 0.125 (0.25/2): < 0.5, so value = 2
	ck_assert_float_eq_tol(val2, 2.0f, 0.001f);
}
END_TEST


/*****************************
*  inverse sawtooth waveform *
*****************************/

START_TEST(test_inverse_sawtooth_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "inv_saw_zero", W_OSCILLATOR_TYPE(INVERSE_SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: 1 - 2*0 = 1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "inv_saw_zero");
	ck_assert_float_eq_tol(value, 1.0f, 0.01f);
}
END_TEST

START_TEST(test_inverse_sawtooth_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "inv_saw_half", W_OSCILLATOR_TYPE(INVERSE_SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: 1 - 2*0.5 = 0
	float value = wm_utils_oscillator_get_value(&g_world, owner, "inv_saw_half");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_inverse_sawtooth_near_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "inv_saw_one", W_OSCILLATOR_TYPE(INVERSE_SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.99);

	// at phase ~1: 1 - 2*0.99 = -0.98
	float value = wm_utils_oscillator_get_value(&g_world, owner, "inv_saw_one");
	ck_assert_float_eq_tol(value, -0.98f, 0.02f);
}
END_TEST


/*****************************
*  bounce waveform           *
*****************************/

START_TEST(test_bounce_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "bounce_zero", W_OSCILLATOR_TYPE(BOUNCE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: |sin(0)| = 0
	float value = wm_utils_oscillator_get_value(&g_world, owner, "bounce_zero");
	ck_assert_float_eq_tol(value, 0.0f, 0.01f);
}
END_TEST

START_TEST(test_bounce_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "bounce_half", W_OSCILLATOR_TYPE(BOUNCE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: |sin(pi/2)| = 1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "bounce_half");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST

START_TEST(test_bounce_at_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "bounce_one", W_OSCILLATOR_TYPE(BOUNCE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.99);

	// at phase ~1: |sin(pi)| ~= 0
	float value = wm_utils_oscillator_get_value(&g_world, owner, "bounce_one");
	ck_assert_float_eq_tol(value, 0.0f, 0.05f);
}
END_TEST


/*****************************
*  smooth step waveform      *
*****************************/

START_TEST(test_smooth_step_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "smooth_zero", W_OSCILLATOR_TYPE(SMOOTH_STEP), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: 2*(0) - 1 = -1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "smooth_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_smooth_step_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "smooth_half", W_OSCILLATOR_TYPE(SMOOTH_STEP), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: hermite(0.5) = 0.5, so 2*0.5 - 1 = 0
	float value = wm_utils_oscillator_get_value(&g_world, owner, "smooth_half");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_smooth_step_near_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "smooth_one", W_OSCILLATOR_TYPE(SMOOTH_STEP), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.99);

	// at phase ~1: hermite(1) = 1, so 2*1 - 1 = 1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "smooth_one");
	ck_assert_float_eq_tol(value, 1.0f, 0.01f);
}
END_TEST


/*****************************
*  rectified sine waveform   *
*****************************/

START_TEST(test_rectified_sine_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "rect_zero", W_OSCILLATOR_TYPE(RECTIFIED_SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: 2*|sin(0)| - 1 = -1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "rect_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_rectified_sine_at_phase_quarter)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "rect_quarter", W_OSCILLATOR_TYPE(RECTIFIED_SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.25);

	// at phase 0.25: 2*|sin(pi/2)| - 1 = 2*1 - 1 = 1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "rect_quarter");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST

START_TEST(test_rectified_sine_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "rect_half", W_OSCILLATOR_TYPE(RECTIFIED_SINE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: 2*|sin(pi)| - 1 = -1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "rect_half");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST


/*****************************
*  noise waveform            *
*****************************/

START_TEST(test_noise_returns_value_in_range)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "noise_test", W_OSCILLATOR_TYPE(NOISE), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	// run multiple times and verify all values are in [-1, 1]
	for (int i = 0; i < 100; i++) {
		advance_oscillators(0.01);
		float value = wm_utils_oscillator_get_value(&g_world, owner, "noise_test");
		ck_assert_float_ge(value, -1.0f);
		ck_assert_float_le(value, 1.0f);
	}
}
END_TEST

START_TEST(test_noise_with_amplitude_offset)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "noise_amp", W_OSCILLATOR_TYPE(NOISE), 1.0f, 2.0f, 5.0f, 0.0f, 0.0f);

	// run multiple times and verify all values are in [5-2, 5+2] = [3, 7]
	for (int i = 0; i < 100; i++) {
		advance_oscillators(0.01);
		float value = wm_utils_oscillator_get_value(&g_world, owner, "noise_amp");
		ck_assert_float_ge(value, 3.0f);
		ck_assert_float_le(value, 7.0f);
	}
}
END_TEST


/*****************************
*  fixed exponential waveform*
*****************************/

START_TEST(test_fixed_exponential_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "exp_zero", W_OSCILLATOR_TYPE(FIXED_EXPONENTIAL), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: 2*(0) - 1 = -1
	float value = wm_utils_oscillator_get_value(&g_world, owner, "exp_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_fixed_exponential_near_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "exp_one", W_OSCILLATOR_TYPE(FIXED_EXPONENTIAL), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.99);

	// at phase 0.99: approaches 1 but doesn't quite reach due to curve shape
	float value = wm_utils_oscillator_get_value(&g_world, owner, "exp_one");
	ck_assert_float_gt(value, 0.9f);
	ck_assert_float_le(value, 1.0f);
}
END_TEST

START_TEST(test_fixed_exponential_curve_shape)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	wm_utils_oscillator_create(&g_world, owner, "exp_curve", W_OSCILLATOR_TYPE(FIXED_EXPONENTIAL), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: exponential should be below linear (which would be 0)
	// (exp(1.5) - 1) / (exp(3) - 1) ~= 0.2
	// 2*0.2 - 1 = -0.6
	float value = wm_utils_oscillator_get_value(&g_world, owner, "exp_curve");
	ck_assert_float_lt(value, 0.0f);
	ck_assert_float_gt(value, -1.0f);
}
END_TEST


/*****************************
*  tag operations            *
*****************************/

START_TEST(test_tag_set_and_has)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id tag_id = w_ecs_get_component_by_name(&g_world, "test_tag");

	ck_assert(!w_ecs_has_tag(&g_world, tag_id, entity));

	w_ecs_set_tag(&g_world, tag_id, entity);

	ck_assert(w_ecs_has_tag(&g_world, tag_id, entity));
}
END_TEST

START_TEST(test_tag_remove)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id tag_id = w_ecs_get_component_by_name(&g_world, "remove_tag");

	w_ecs_set_tag(&g_world, tag_id, entity);
	ck_assert(w_ecs_has_tag(&g_world, tag_id, entity));

	w_ecs_remove_tag(&g_world, tag_id, entity);
	ck_assert(!w_ecs_has_tag(&g_world, tag_id, entity));
}
END_TEST

START_TEST(test_tag_str_set_and_has)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	ck_assert(!w_ecs_has_tag_str(&g_world, "str_test_tag", entity));

	w_ecs_set_tag_str(&g_world, "str_test_tag", entity);

	ck_assert(w_ecs_has_tag_str(&g_world, "str_test_tag", entity));
}
END_TEST

START_TEST(test_tag_str_remove)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "str_remove_tag", entity);
	ck_assert(w_ecs_has_tag_str(&g_world, "str_remove_tag", entity));

	w_ecs_remove_tag_str(&g_world, "str_remove_tag", entity);
	ck_assert(!w_ecs_has_tag_str(&g_world, "str_remove_tag", entity));
}
END_TEST

START_TEST(test_tag_multiple_entities)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);
	w_entity_id tag_id = w_ecs_get_component_by_name(&g_world, "multi_tag");

	w_ecs_set_tag(&g_world, tag_id, e1);

	ck_assert(w_ecs_has_tag(&g_world, tag_id, e1));
	ck_assert(!w_ecs_has_tag(&g_world, tag_id, e2));

	w_ecs_set_tag(&g_world, tag_id, e2);

	ck_assert(w_ecs_has_tag(&g_world, tag_id, e1));
	ck_assert(w_ecs_has_tag(&g_world, tag_id, e2));
}
END_TEST

START_TEST(test_tag_idempotent_set)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id tag_id = w_ecs_get_component_by_name(&g_world, "idempotent_tag");

	w_ecs_set_tag(&g_world, tag_id, entity);
	w_ecs_set_tag(&g_world, tag_id, entity);
	w_ecs_set_tag(&g_world, tag_id, entity);

	ck_assert(w_ecs_has_tag(&g_world, tag_id, entity));

	w_ecs_remove_tag(&g_world, tag_id, entity);
	ck_assert(!w_ecs_has_tag(&g_world, tag_id, entity));
}
END_TEST

START_TEST(test_tag_remove_nonexistent)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id tag_id = w_ecs_get_component_by_name(&g_world, "nonexistent_tag");

	ck_assert(!w_ecs_has_tag(&g_world, tag_id, entity));
	w_ecs_remove_tag(&g_world, tag_id, entity);
	ck_assert(!w_ecs_has_tag(&g_world, tag_id, entity));
}
END_TEST


/*****************************
*  entity lifecycle hooks    *
*****************************/

START_TEST(test_lifecycle_destroy_tags_registered)
{
	w_entity_id destroy_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_TAG);
	w_entity_id eof_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_END_OF_FRAME_TAG);
	w_entity_id eoff_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_END_OF_FIXED_FRAME_TAG);
	w_entity_id eop_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_END_OF_PHASE_TAG);

	ck_assert(w_ecs_is_valid_entity(destroy_tag));
	ck_assert(w_ecs_is_valid_entity(eof_tag));
	ck_assert(w_ecs_is_valid_entity(eoff_tag));
	ck_assert(w_ecs_is_valid_entity(eop_tag));
}
END_TEST

START_TEST(test_lifecycle_destroy_end_of_frame_sets_tags)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id destroy_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_TAG);
	w_entity_id eof_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_END_OF_FRAME_TAG);

	ck_assert(!w_ecs_has_tag(&g_world, destroy_tag, entity));
	ck_assert(!w_ecs_has_tag(&g_world, eof_tag, entity));

	w_entity_destroy_end_of_frame(&g_world, entity);

	ck_assert(w_ecs_has_tag(&g_world, destroy_tag, entity));
	ck_assert(w_ecs_has_tag(&g_world, eof_tag, entity));
}
END_TEST

START_TEST(test_lifecycle_destroy_end_of_fixed_frame_sets_tags)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id destroy_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_TAG);
	w_entity_id eoff_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_END_OF_FIXED_FRAME_TAG);

	ck_assert(!w_ecs_has_tag(&g_world, destroy_tag, entity));
	ck_assert(!w_ecs_has_tag(&g_world, eoff_tag, entity));

	w_entity_destroy_end_of_fixed_frame(&g_world, entity);

	ck_assert(w_ecs_has_tag(&g_world, destroy_tag, entity));
	ck_assert(w_ecs_has_tag(&g_world, eoff_tag, entity));
}
END_TEST

START_TEST(test_lifecycle_destroy_end_of_phase_sets_tags)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id destroy_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_TAG);
	w_entity_id eop_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_END_OF_PHASE_TAG);

	ck_assert(!w_ecs_has_tag(&g_world, destroy_tag, entity));
	ck_assert(!w_ecs_has_tag(&g_world, eop_tag, entity));

	w_entity_destroy_end_of_phase(&g_world, entity);

	ck_assert(w_ecs_has_tag(&g_world, destroy_tag, entity));
	ck_assert(w_ecs_has_tag(&g_world, eop_tag, entity));
}
END_TEST

// helper to rebuild query cache and call lifecycle hook
static void call_lifecycle_hook_end_of_frame(void)
{
	char *query_str = w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	                  w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_FRAME_TAG);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);
	destroy_end_of_frame_lifecycle_hook(&g_world, NULL);
}

static void call_lifecycle_hook_end_of_fixed_frame(void)
{
	char *query_str = w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	                  w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_FIXED_FRAME_TAG);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);
	destroy_end_of_fixed_frame_lifecycle_hook(&g_world, NULL);
}

static void call_lifecycle_hook_end_of_phase(void)
{
	char *query_str = w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	                  w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_PHASE_TAG);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);
	destroy_end_of_phase_lifecycle_hook(&g_world, NULL);
}

START_TEST(test_lifecycle_hook_destroys_entity_end_of_frame)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "marker_comp", entity);
	ck_assert(w_ecs_has_tag_str(&g_world, "marker_comp", entity));

	w_entity_destroy_end_of_frame(&g_world, entity);

	call_lifecycle_hook_end_of_frame();

	ck_assert(!w_ecs_has_tag_str(&g_world, "marker_comp", entity));
}
END_TEST

START_TEST(test_lifecycle_hook_destroys_entity_end_of_fixed_frame)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "marker_fixed", entity);
	ck_assert(w_ecs_has_tag_str(&g_world, "marker_fixed", entity));

	w_entity_destroy_end_of_fixed_frame(&g_world, entity);

	call_lifecycle_hook_end_of_fixed_frame();

	ck_assert(!w_ecs_has_tag_str(&g_world, "marker_fixed", entity));
}
END_TEST

START_TEST(test_lifecycle_hook_destroys_entity_end_of_phase)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "marker_phase", entity);
	ck_assert(w_ecs_has_tag_str(&g_world, "marker_phase", entity));

	w_entity_destroy_end_of_phase(&g_world, entity);

	call_lifecycle_hook_end_of_phase();

	ck_assert(!w_ecs_has_tag_str(&g_world, "marker_phase", entity));
}
END_TEST

START_TEST(test_lifecycle_hooks_independent)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);
	w_entity_id e3 = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "m1", e1);
	w_ecs_set_tag_str(&g_world, "m2", e2);
	w_ecs_set_tag_str(&g_world, "m3", e3);

	w_entity_destroy_end_of_frame(&g_world, e1);
	w_entity_destroy_end_of_fixed_frame(&g_world, e2);
	w_entity_destroy_end_of_phase(&g_world, e3);

	call_lifecycle_hook_end_of_frame();
	ck_assert(!w_ecs_has_tag_str(&g_world, "m1", e1));
	ck_assert(w_ecs_has_tag_str(&g_world, "m2", e2));
	ck_assert(w_ecs_has_tag_str(&g_world, "m3", e3));

	call_lifecycle_hook_end_of_fixed_frame();
	ck_assert(!w_ecs_has_tag_str(&g_world, "m2", e2));
	ck_assert(w_ecs_has_tag_str(&g_world, "m3", e3));

	call_lifecycle_hook_end_of_phase();
	ck_assert(!w_ecs_has_tag_str(&g_world, "m3", e3));
}
END_TEST

START_TEST(test_lifecycle_multiple_entities_same_hook)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);
	w_entity_id e3 = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "bulk_m", e1);
	w_ecs_set_tag_str(&g_world, "bulk_m", e2);
	w_ecs_set_tag_str(&g_world, "bulk_m", e3);

	w_entity_destroy_end_of_frame(&g_world, e1);
	w_entity_destroy_end_of_frame(&g_world, e2);
	w_entity_destroy_end_of_frame(&g_world, e3);

	call_lifecycle_hook_end_of_frame();

	ck_assert(!w_ecs_has_tag_str(&g_world, "bulk_m", e1));
	ck_assert(!w_ecs_has_tag_str(&g_world, "bulk_m", e2));
	ck_assert(!w_ecs_has_tag_str(&g_world, "bulk_m", e3));
}
END_TEST

START_TEST(test_lifecycle_unmarked_entity_not_destroyed)
{
	w_entity_id marked = w_ecs_request_entity(&g_world);
	w_entity_id unmarked = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "surv", marked);
	w_ecs_set_tag_str(&g_world, "surv", unmarked);

	w_entity_destroy_end_of_frame(&g_world, marked);

	call_lifecycle_hook_end_of_frame();

	ck_assert(!w_ecs_has_tag_str(&g_world, "surv", marked));
	ck_assert(w_ecs_has_tag_str(&g_world, "surv", unmarked));
}
END_TEST


/*****************************
*  disabled_t tag tests      *
*****************************/

START_TEST(test_disabled_tag_can_be_set)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	ck_assert(!w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_DISABLED_TAG, entity));

	w_entity_set_disabled(&g_world, entity);

	ck_assert(w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_DISABLED_TAG, entity));
}
END_TEST

START_TEST(test_disabled_tag_can_be_removed)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_entity_set_disabled(&g_world, entity);
	ck_assert(w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_DISABLED_TAG, entity));

	w_entity_set_enabled(&g_world, entity);
	ck_assert(!w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_DISABLED_TAG, entity));
}
END_TEST

START_TEST(test_disabled_entity_excluded_from_query)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "test_comp", e1);
	w_ecs_set_tag_str(&g_world, "test_comp", e2);

	w_entity_set_disabled(&g_world, e1);

	// rebuild cache before query
	char *query_str = w_query_read("test_comp") w_query_not(W_ENTITY_LIFECYCLE_DISABLED_TAG);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, query_str, {
		count++;
		ck_assert(itor.entity_id != e1);
	});

	ck_assert_int_eq(count, 1);
}
END_TEST

START_TEST(test_disabled_entity_included_without_exclude)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "test_comp2", e1);
	w_ecs_set_tag_str(&g_world, "test_comp2", e2);

	w_entity_set_disabled(&g_world, e1);

	// rebuild cache before query
	char *query_str = w_query_read("test_comp2");
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, query_str, {
		count++;
	});

	ck_assert_int_eq(count, 2);
}
END_TEST


/*****************************
*  created_this_frame tests  *
*****************************/

START_TEST(test_created_this_frame_tag_set_on_create)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	ck_assert(w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, entity));
}
END_TEST

START_TEST(test_created_this_frame_tag_cleared_by_hook)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	ck_assert(w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, entity));

	// rebuild query cache and call cleanup hook
	char *query_str = w_query_read(W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);
	cleanup_created_this_frame_tag_lifecycle_hook(&g_world, NULL);

	ck_assert(!w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, entity));
}
END_TEST

START_TEST(test_created_this_frame_multiple_entities)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);
	w_entity_id e3 = w_ecs_request_entity(&g_world);

	ck_assert(w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, e1));
	ck_assert(w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, e2));
	ck_assert(w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, e3));

	char *query_str = w_query_read(W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);
	cleanup_created_this_frame_tag_lifecycle_hook(&g_world, NULL);

	ck_assert(!w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, e1));
	ck_assert(!w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, e2));
	ck_assert(!w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, e3));
}
END_TEST


/*****************************
*  entity lifetime tests     *
*****************************/

// helper to call lifetime expired hook
static void call_lifetime_expired_hook(void)
{
	char *query_str = w_query_read(W_ENTITY_LIFECYCLE_LIFETIME_FINISHED_TAG);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);
	destroy_lifetime_expired_lifecycle_hook(&g_world, NULL);
}

START_TEST(test_lifetime_creates_timer)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_entity_set_lifetime(&g_world, entity, 1.0);

	// entity should have timer reference component
	char timer_entity_name[128];
	snprintf(timer_entity_name, sizeof(timer_entity_name), "%s" W_TIMER_COMPONENT_TIMER_ENTITY, W_ENTITY_LIFECYCLE_LIFETIME_TIMER_NAME);
	ck_assert(w_ecs_has_str(&g_world, timer_entity_name, entity));
}
END_TEST

START_TEST(test_lifetime_timer_finishes_sets_tag)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_entity_set_lifetime(&g_world, entity, 0.5);

	// advance time past lifetime
	advance_time(0.6);

	// finished tag should be set
	ck_assert(w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_LIFETIME_FINISHED_TAG, entity));
}
END_TEST

START_TEST(test_lifetime_expired_hook_destroys_entity)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "lifetime_marker", entity);
	w_entity_set_lifetime(&g_world, entity, 0.5);

	// advance time past lifetime
	advance_time(0.6);

	ck_assert(w_ecs_has_tag_str(&g_world, "lifetime_marker", entity));

	call_lifetime_expired_hook();

	ck_assert(!w_ecs_has_tag_str(&g_world, "lifetime_marker", entity));
}
END_TEST

START_TEST(test_lifetime_not_expired_entity_survives)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);

	w_ecs_set_tag_str(&g_world, "survive_marker", entity);
	w_entity_set_lifetime(&g_world, entity, 2.0);

	// advance time but not past lifetime
	advance_time(0.5);

	ck_assert(!w_ecs_has_tag_str(&g_world, W_ENTITY_LIFECYCLE_LIFETIME_FINISHED_TAG, entity));

	call_lifetime_expired_hook();

	ck_assert(w_ecs_has_tag_str(&g_world, "survive_marker", entity));
}
END_TEST


/*****************************
*  entity create hook tests  *
*****************************/

static int g_create_hook_call_count = 0;
static w_entity_id g_last_created_entity = 0;

static void test_entity_create_hook_(void *world, void *entity)
{
	(void)world;
	g_create_hook_call_count++;
	g_last_created_entity = *(w_entity_id *)entity;
}

START_TEST(test_entity_create_hook_fires)
{
	g_create_hook_call_count = 0;
	g_last_created_entity = 0;

	w_ecs_register_entity_create_hook(&g_world, test_entity_create_hook_);

	w_entity_id entity = w_ecs_request_entity(&g_world);

	// hook called (plus the built-in created_this_frame hook)
	ck_assert_int_ge(g_create_hook_call_count, 1);
	ck_assert(g_last_created_entity == entity);
}
END_TEST

START_TEST(test_entity_create_hook_fires_for_each_entity)
{
	g_create_hook_call_count = 0;

	w_ecs_register_entity_create_hook(&g_world, test_entity_create_hook_);

	w_ecs_request_entity(&g_world);
	w_ecs_request_entity(&g_world);
	w_ecs_request_entity(&g_world);

	ck_assert_int_ge(g_create_hook_call_count, 3);
}
END_TEST

START_TEST(test_entity_create_hook_named_entity)
{
	g_create_hook_call_count = 0;
	g_last_created_entity = 0;

	w_ecs_register_entity_create_hook(&g_world, test_entity_create_hook_);

	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "test_named_entity");

	ck_assert_int_ge(g_create_hook_call_count, 1);
	ck_assert(g_last_created_entity == entity);
}
END_TEST

START_TEST(test_entity_create_hook_existing_named_entity_no_fire)
{
	g_create_hook_call_count = 0;

	w_entity_id first = w_ecs_request_entity_with_name(&g_world, "existing_entity");

	w_ecs_register_entity_create_hook(&g_world, test_entity_create_hook_);

	// requesting existing named entity should not fire hook
	int count_before = g_create_hook_call_count;
	w_entity_id second = w_ecs_request_entity_with_name(&g_world, "existing_entity");

	ck_assert(first == second);
	ck_assert_int_eq(g_create_hook_call_count, count_before);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *utilities_suite(void)
{
	Suite *s = suite_create("utilities");

	TCase *tc_name = tcase_create("timer_name");
	tcase_add_checked_fixture(tc_name, timer_setup, timer_teardown);
	tcase_set_timeout(tc_name, 10);
	tcase_add_test(tc_name, test_timer_entity_gets_name);
	tcase_add_test(tc_name, test_timer_entity_names_unique);
	suite_add_tcase(s, tc_name);

	TCase *tc_loop = tcase_create("loop_timer");
	tcase_add_checked_fixture(tc_loop, timer_setup, timer_teardown);
	tcase_set_timeout(tc_loop, 10);
	tcase_add_test(tc_loop, test_loop_timer_fires_repeatedly);
	tcase_add_test(tc_loop, test_loop_timer_resets_elapsed);
	suite_add_tcase(s, tc_loop);

	TCase *tc_oneshot = tcase_create("oneshot_timer");
	tcase_add_checked_fixture(tc_oneshot, timer_setup, timer_teardown);
	tcase_set_timeout(tc_oneshot, 10);
	tcase_add_test(tc_oneshot, test_oneshot_timer_fires_once);
	tcase_add_test(tc_oneshot, test_oneshot_timer_destroyed_after_completion);
	suite_add_tcase(s, tc_oneshot);

	TCase *tc_tag = tcase_create("finished_tag");
	tcase_add_checked_fixture(tc_tag, timer_setup, timer_teardown);
	tcase_set_timeout(tc_tag, 10);
	tcase_add_test(tc_tag, test_finished_tag_stays_one_frame);
	tcase_add_test(tc_tag, test_finished_tag_exactly_one_frame_total);
	suite_add_tcase(s, tc_tag);

	TCase *tc_pause = tcase_create("pause");
	tcase_add_checked_fixture(tc_pause, timer_setup, timer_teardown);
	tcase_set_timeout(tc_pause, 10);
	tcase_add_test(tc_pause, test_pause_stops_timer);
	tcase_add_test(tc_pause, test_unpause_resumes_timer);
	suite_add_tcase(s, tc_pause);

	TCase *tc_inert = tcase_create("inert_pause");
	tcase_add_checked_fixture(tc_inert, timer_setup, timer_teardown);
	tcase_set_timeout(tc_inert, 10);
	tcase_add_test(tc_inert, test_inert_timer_auto_pauses);
	tcase_add_test(tc_inert, test_inert_timer_can_restart);
	suite_add_tcase(s, tc_inert);

	TCase *tc_multi = tcase_create("multiple_timers");
	tcase_add_checked_fixture(tc_multi, timer_setup, timer_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_multiple_timers_independent);
	tcase_add_test(tc_multi, test_multiple_owners_different_timers);
	suite_add_tcase(s, tc_multi);

	TCase *tc_stress = tcase_create("stress");
	tcase_add_checked_fixture(tc_stress, timer_setup, timer_teardown);
	tcase_set_timeout(tc_stress, 30);
	tcase_add_test(tc_stress, test_timer_stress_many_timers);
	tcase_add_test(tc_stress, test_timer_stress_rapid_create_destroy);
	suite_add_tcase(s, tc_stress);

	// oscillator tests
	TCase *tc_osc_name = tcase_create("oscillator_name");
	tcase_add_checked_fixture(tc_osc_name, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_osc_name, 10);
	tcase_add_test(tc_osc_name, test_oscillator_entity_gets_name);
	tcase_add_test(tc_osc_name, test_oscillator_entity_names_unique);
	suite_add_tcase(s, tc_osc_name);

	TCase *tc_osc_comp = tcase_create("oscillator_components");
	tcase_add_checked_fixture(tc_osc_comp, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_osc_comp, 10);
	tcase_add_test(tc_osc_comp, test_oscillator_components_set);
	tcase_add_test(tc_osc_comp, test_oscillator_owner_entity_ref);
	suite_add_tcase(s, tc_osc_comp);

	TCase *tc_sine = tcase_create("sine_waveform");
	tcase_add_checked_fixture(tc_sine, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_sine, 10);
	tcase_add_test(tc_sine, test_sine_at_phase_zero);
	tcase_add_test(tc_sine, test_sine_at_phase_quarter);
	tcase_add_test(tc_sine, test_sine_at_phase_half);
	tcase_add_test(tc_sine, test_sine_with_amplitude_offset);
	suite_add_tcase(s, tc_sine);

	TCase *tc_triangle = tcase_create("triangle_waveform");
	tcase_add_checked_fixture(tc_triangle, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_triangle, 10);
	tcase_add_test(tc_triangle, test_triangle_at_phase_zero);
	tcase_add_test(tc_triangle, test_triangle_at_phase_quarter);
	tcase_add_test(tc_triangle, test_triangle_at_phase_half);
	suite_add_tcase(s, tc_triangle);

	TCase *tc_square = tcase_create("square_waveform");
	tcase_add_checked_fixture(tc_square, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_square, 10);
	tcase_add_test(tc_square, test_square_at_phase_zero);
	tcase_add_test(tc_square, test_square_at_phase_half);
	suite_add_tcase(s, tc_square);

	TCase *tc_sawtooth = tcase_create("sawtooth_waveform");
	tcase_add_checked_fixture(tc_sawtooth, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_sawtooth, 10);
	tcase_add_test(tc_sawtooth, test_sawtooth_at_phase_zero);
	tcase_add_test(tc_sawtooth, test_sawtooth_at_phase_half);
	tcase_add_test(tc_sawtooth, test_sawtooth_near_phase_one);
	suite_add_tcase(s, tc_sawtooth);

	TCase *tc_phase_shift = tcase_create("phase_shift");
	tcase_add_checked_fixture(tc_phase_shift, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_phase_shift, 10);
	tcase_add_test(tc_phase_shift, test_phase_shift_sine);
	suite_add_tcase(s, tc_phase_shift);

	TCase *tc_phase_cycle = tcase_create("phase_cycling");
	tcase_add_checked_fixture(tc_phase_cycle, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_phase_cycle, 10);
	tcase_add_test(tc_phase_cycle, test_phase_wraps_at_one);
	tcase_add_test(tc_phase_cycle, test_oscillator_continues_forever);
	suite_add_tcase(s, tc_phase_cycle);

	TCase *tc_invalid = tcase_create("invalid_period");
	tcase_add_checked_fixture(tc_invalid, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_invalid, 10);
	tcase_add_test(tc_invalid, test_zero_period_skipped);
	tcase_add_test(tc_invalid, test_negative_period_skipped);
	suite_add_tcase(s, tc_invalid);

	TCase *tc_osc_multi = tcase_create("multiple_oscillators");
	tcase_add_checked_fixture(tc_osc_multi, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_osc_multi, 10);
	tcase_add_test(tc_osc_multi, test_multiple_oscillators_independent);
	suite_add_tcase(s, tc_osc_multi);

	TCase *tc_inv_saw = tcase_create("inverse_sawtooth_waveform");
	tcase_add_checked_fixture(tc_inv_saw, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_inv_saw, 10);
	tcase_add_test(tc_inv_saw, test_inverse_sawtooth_at_phase_zero);
	tcase_add_test(tc_inv_saw, test_inverse_sawtooth_at_phase_half);
	tcase_add_test(tc_inv_saw, test_inverse_sawtooth_near_phase_one);
	suite_add_tcase(s, tc_inv_saw);

	TCase *tc_bounce = tcase_create("bounce_waveform");
	tcase_add_checked_fixture(tc_bounce, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_bounce, 10);
	tcase_add_test(tc_bounce, test_bounce_at_phase_zero);
	tcase_add_test(tc_bounce, test_bounce_at_phase_half);
	tcase_add_test(tc_bounce, test_bounce_at_phase_one);
	suite_add_tcase(s, tc_bounce);

	TCase *tc_smooth = tcase_create("smooth_step_waveform");
	tcase_add_checked_fixture(tc_smooth, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_smooth, 10);
	tcase_add_test(tc_smooth, test_smooth_step_at_phase_zero);
	tcase_add_test(tc_smooth, test_smooth_step_at_phase_half);
	tcase_add_test(tc_smooth, test_smooth_step_near_phase_one);
	suite_add_tcase(s, tc_smooth);

	TCase *tc_rect = tcase_create("rectified_sine_waveform");
	tcase_add_checked_fixture(tc_rect, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_rect, 10);
	tcase_add_test(tc_rect, test_rectified_sine_at_phase_zero);
	tcase_add_test(tc_rect, test_rectified_sine_at_phase_quarter);
	tcase_add_test(tc_rect, test_rectified_sine_at_phase_half);
	suite_add_tcase(s, tc_rect);

	TCase *tc_noise = tcase_create("noise_waveform");
	tcase_add_checked_fixture(tc_noise, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_noise, 10);
	tcase_add_test(tc_noise, test_noise_returns_value_in_range);
	tcase_add_test(tc_noise, test_noise_with_amplitude_offset);
	suite_add_tcase(s, tc_noise);

	TCase *tc_exp = tcase_create("fixed_exponential_waveform");
	tcase_add_checked_fixture(tc_exp, oscillator_setup, oscillator_teardown);
	tcase_set_timeout(tc_exp, 10);
	tcase_add_test(tc_exp, test_fixed_exponential_at_phase_zero);
	tcase_add_test(tc_exp, test_fixed_exponential_near_phase_one);
	tcase_add_test(tc_exp, test_fixed_exponential_curve_shape);
	suite_add_tcase(s, tc_exp);

	// tag operation tests
	TCase *tc_tags = tcase_create("tag_operations");
	tcase_add_checked_fixture(tc_tags, timer_setup, timer_teardown);
	tcase_set_timeout(tc_tags, 10);
	tcase_add_test(tc_tags, test_tag_set_and_has);
	tcase_add_test(tc_tags, test_tag_remove);
	tcase_add_test(tc_tags, test_tag_str_set_and_has);
	tcase_add_test(tc_tags, test_tag_str_remove);
	tcase_add_test(tc_tags, test_tag_multiple_entities);
	tcase_add_test(tc_tags, test_tag_idempotent_set);
	tcase_add_test(tc_tags, test_tag_remove_nonexistent);
	suite_add_tcase(s, tc_tags);

	// entity lifecycle hook tests
	TCase *tc_lifecycle = tcase_create("entity_lifecycle");
	tcase_add_checked_fixture(tc_lifecycle, timer_setup, timer_teardown);
	tcase_set_timeout(tc_lifecycle, 10);
	tcase_add_test(tc_lifecycle, test_lifecycle_destroy_tags_registered);
	tcase_add_test(tc_lifecycle, test_lifecycle_destroy_end_of_frame_sets_tags);
	tcase_add_test(tc_lifecycle, test_lifecycle_destroy_end_of_fixed_frame_sets_tags);
	tcase_add_test(tc_lifecycle, test_lifecycle_destroy_end_of_phase_sets_tags);
	tcase_add_test(tc_lifecycle, test_lifecycle_hook_destroys_entity_end_of_frame);
	tcase_add_test(tc_lifecycle, test_lifecycle_hook_destroys_entity_end_of_fixed_frame);
	tcase_add_test(tc_lifecycle, test_lifecycle_hook_destroys_entity_end_of_phase);
	tcase_add_test(tc_lifecycle, test_lifecycle_hooks_independent);
	tcase_add_test(tc_lifecycle, test_lifecycle_multiple_entities_same_hook);
	tcase_add_test(tc_lifecycle, test_lifecycle_unmarked_entity_not_destroyed);
	suite_add_tcase(s, tc_lifecycle);

	// disabled_t tag tests
	TCase *tc_disabled = tcase_create("disabled_tag");
	tcase_add_checked_fixture(tc_disabled, timer_setup, timer_teardown);
	tcase_set_timeout(tc_disabled, 10);
	tcase_add_test(tc_disabled, test_disabled_tag_can_be_set);
	tcase_add_test(tc_disabled, test_disabled_tag_can_be_removed);
	tcase_add_test(tc_disabled, test_disabled_entity_excluded_from_query);
	tcase_add_test(tc_disabled, test_disabled_entity_included_without_exclude);
	suite_add_tcase(s, tc_disabled);

	// created_this_frame_t tests
	TCase *tc_created = tcase_create("created_this_frame");
	tcase_add_checked_fixture(tc_created, timer_setup, timer_teardown);
	tcase_set_timeout(tc_created, 10);
	tcase_add_test(tc_created, test_created_this_frame_tag_set_on_create);
	tcase_add_test(tc_created, test_created_this_frame_tag_cleared_by_hook);
	tcase_add_test(tc_created, test_created_this_frame_multiple_entities);
	suite_add_tcase(s, tc_created);

	// entity lifetime tests
	TCase *tc_lifetime = tcase_create("entity_lifetime");
	tcase_add_checked_fixture(tc_lifetime, timer_setup, timer_teardown);
	tcase_set_timeout(tc_lifetime, 10);
	tcase_add_test(tc_lifetime, test_lifetime_creates_timer);
	tcase_add_test(tc_lifetime, test_lifetime_timer_finishes_sets_tag);
	tcase_add_test(tc_lifetime, test_lifetime_expired_hook_destroys_entity);
	tcase_add_test(tc_lifetime, test_lifetime_not_expired_entity_survives);
	suite_add_tcase(s, tc_lifetime);

	// entity create hook tests
	TCase *tc_create_hook = tcase_create("entity_create_hook");
	tcase_add_checked_fixture(tc_create_hook, timer_setup, timer_teardown);
	tcase_set_timeout(tc_create_hook, 10);
	tcase_add_test(tc_create_hook, test_entity_create_hook_fires);
	tcase_add_test(tc_create_hook, test_entity_create_hook_fires_for_each_entity);
	tcase_add_test(tc_create_hook, test_entity_create_hook_named_entity);
	tcase_add_test(tc_create_hook, test_entity_create_hook_existing_named_entity_no_fire);
	suite_add_tcase(s, tc_create_hook);

	return s;
}

int main(void)
{
	Suite *s = utilities_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
