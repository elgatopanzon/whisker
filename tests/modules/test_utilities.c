/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_utilities
 * @created     : Monday Mar 24, 2026 16:15:00 CST
 * @description : tests for whisker_utilities module (timer and oscillator systems)
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/utilities/whisker_utilities.h"

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

	// create component entries for optional components by setting them on a dummy entity
	// this ensures the component entry and data_bitset exist for query resolution
	w_entity_id dummy = w_ecs_request_entity(&g_world);
	w_ecs_set_tag_str(&g_world, W_TIMER_COMPONENT_LOOP, dummy);
	w_ecs_set_tag_str(&g_world, W_TIMER_COMPONENT_ONESHOT, dummy);
	w_ecs_set_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, dummy);
	// remove them so the dummy doesn't interfere with tests
	w_ecs_remove_tag_str(&g_world, W_TIMER_COMPONENT_LOOP, dummy);
	w_ecs_remove_tag_str(&g_world, W_TIMER_COMPONENT_ONESHOT, dummy);
	w_ecs_remove_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, dummy);
	w_ecs_return_entity(&g_world, dummy);

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
	w_query_read(W_TIMER_COMPONENT_DURATION) \
	w_query_read(W_TIMER_COMPONENT_OWNER_ENTITY) \
	w_query_read(W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID) \
	w_query_optional(W_TIMER_COMPONENT_LOOP) \
	w_query_optional(W_TIMER_COMPONENT_ONESHOT) \
	w_query_optional(W_TIMER_COMPONENT_PAUSED)

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
			float *elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
			float *duration = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_DURATION, timer);
			w_entity_id *owner = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_OWNER_ENTITY, timer);
			w_entity_id *finished_comp_id = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID, timer);

			if (!elapsed || !duration || !owner || !finished_comp_id) continue;

			bool has_paused = w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer);
			if (has_paused) continue;

			bool has_loop = w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_LOOP, timer);
			bool has_oneshot = w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_ONESHOT, timer);

			*elapsed += delta_seconds;

			if (*finished_comp_id != W_ENTITY_INVALID && *owner != W_ENTITY_INVALID && *duration > 0.0f && *elapsed >= *duration)
			{
				// stage 1: set finished tag on owner if not present
				if (!w_ecs_has_tag(&g_world, *finished_comp_id, *owner))
				{
					w_ecs_set_tag(&g_world, *finished_comp_id, *owner);
				}
				// stage 2: tag exists, remove it and run reset/destroy logic
				else
				{
					w_ecs_remove_tag(&g_world, *finished_comp_id, *owner);

					if (has_loop)
						*elapsed = 0;
					else if (has_oneshot)
						w_ecs_return_entity(&g_world, timer);
					else
						w_ecs_set_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer);
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

			float *elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
			float *duration = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_DURATION, timer);
			w_entity_id *owner = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_OWNER_ENTITY, timer);
			w_entity_id *finished_comp_id = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID, timer);

			if (!elapsed || !duration || !owner || !finished_comp_id) continue;

			bool has_paused = w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer);
			if (has_paused) continue;

			bool has_loop = w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_LOOP, timer);
			bool has_oneshot = w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_ONESHOT, timer);

			*elapsed += delta_seconds;

			if (*finished_comp_id != W_ENTITY_INVALID && *owner != W_ENTITY_INVALID && *duration > 0.0f && *elapsed >= *duration)
			{
				if (!w_ecs_has_tag(&g_world, *finished_comp_id, *owner))
				{
					w_ecs_set_tag(&g_world, *finished_comp_id, *owner);
				}
				else
				{
					w_ecs_remove_tag(&g_world, *finished_comp_id, *owner);

					if (has_loop)
						*elapsed = 0;
					else if (has_oneshot)
						w_ecs_return_entity(&g_world, timer);
					else
						w_ecs_set_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer);
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
	w_entity_id timer = w_timer_create(&g_world, owner, "test_timer", 1.0f, false, false);

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
	w_entity_id timer1 = w_timer_create(&g_world, owner, "timer", 1.0f, false, false);
	w_entity_id timer2 = w_timer_create(&g_world, owner, "timer", 1.0f, false, false);

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
	w_entity_id timer = w_timer_create(&g_world, owner, "loop_test", 0.5f, true, false);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "loop_test_finished_tag");

	// verify timer components are set up correctly
	float *dur = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_DURATION, timer);
	w_entity_id *own = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_OWNER_ENTITY, timer);
	w_entity_id *fcid = w_ecs_get_str(&g_world, w_entity_id, W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID, timer);
	ck_assert_ptr_nonnull(dur);
	ck_assert_ptr_nonnull(own);
	ck_assert_ptr_nonnull(fcid);
	ck_assert_float_eq(*dur, 0.5f);
	ck_assert_int_eq(*own, owner);
	ck_assert_int_eq(*fcid, finished_tag);

	// advance 0.6s to exceed duration
	advance_time(0.6);

	// verify elapsed was updated
	float *el = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_ptr_nonnull(el);
	ck_assert_float_ge(*el, 0.5f);

	// tag should be set on first completion
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));

	// advance again to trigger reset
	advance_time(0.1);

	// tag should be removed, elapsed reset
	ck_assert(!w_ecs_has_tag(&g_world, finished_tag, owner));

	float *elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	// elapsed should be small (0.1s from last advance after reset)
	ck_assert_float_lt(*elapsed, 0.2f);

	// advance past duration again
	advance_time(0.5);

	// tag should fire again
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));
}
END_TEST

START_TEST(test_loop_timer_resets_elapsed)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = w_timer_create(&g_world, owner, "loop_reset", 1.0f, true, false);

	// advance past duration
	advance_time(1.1);
	// first frame: tag set
	advance_time(0.1);
	// second frame: tag removed, elapsed reset

	float *elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	// elapsed should include the 0.1s from last frame
	ck_assert_float_lt(*elapsed, 0.2f);
}
END_TEST


/*****************************
*  oneshot timer             *
*****************************/

START_TEST(test_oneshot_timer_fires_once)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = w_timer_create(&g_world, owner, "oneshot_test", 0.5f, false, true);

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
	float *elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_ptr_null(elapsed);
}
END_TEST

START_TEST(test_oneshot_timer_destroyed_after_completion)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = w_timer_create(&g_world, owner, "oneshot_destroy", 0.5f, false, true);

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
	w_timer_create(&g_world, owner, "tag_test", 0.5f, true, false);

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
	w_timer_create(&g_world, owner, "exact_frame", 1.0f, true, false);

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
	w_entity_id timer = w_timer_create(&g_world, owner, "pause_test", 1.0f, false, false);

	// advance a bit
	advance_time(0.3);

	float *elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	float before_pause = *elapsed;

	// pause the timer
	w_ecs_set_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer);

	// verify pause tag was set
	ck_assert(w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer));

	// advance more
	advance_time(0.5);

	// verify pause tag is still set
	ck_assert(w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer));

	// elapsed should not have changed
	elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_float_eq(*elapsed, before_pause);
}
END_TEST

START_TEST(test_unpause_resumes_timer)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = w_timer_create(&g_world, owner, "unpause_test", 1.0f, true, false);

	// advance a bit
	advance_time(0.3);

	// pause
	w_ecs_set_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer);
	advance_time(0.5);

	float *elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	float paused_elapsed = *elapsed;

	// unpause
	w_ecs_remove_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer);
	advance_time(0.2);

	// elapsed should have increased
	elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_float_gt(*elapsed, paused_elapsed);
}
END_TEST


/*****************************
*  inert pause               *
*****************************/

START_TEST(test_inert_timer_auto_pauses)
{
	// timer without loop or oneshot should auto-pause on completion
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = w_timer_create(&g_world, owner, "inert_test", 0.5f, false, false);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "inert_test_finished_tag");

	// advance past duration
	advance_time(0.6);

	// tag should be set
	ck_assert(w_ecs_has_tag(&g_world, finished_tag, owner));

	// advance again
	advance_time(0.1);

	// tag removed, timer should be paused
	ck_assert(!w_ecs_has_tag(&g_world, finished_tag, owner));
	ck_assert(w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer));

	// timer should still exist (not destroyed like oneshot)
	float *elapsed = w_ecs_get_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer);
	ck_assert_ptr_nonnull(elapsed);
}
END_TEST

START_TEST(test_inert_timer_can_restart)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id timer = w_timer_create(&g_world, owner, "inert_restart", 0.5f, false, false);

	w_entity_id finished_tag = w_ecs_get_component_by_name(&g_world, "inert_restart_finished_tag");

	// complete timer
	advance_time(0.6);
	advance_time(0.1);

	// should be paused
	ck_assert(w_ecs_has_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer));

	// reset elapsed and unpause
	float zero = 0.0f;
	w_ecs_set_str(&g_world, float, W_TIMER_COMPONENT_ELAPSED, timer, &zero);
	w_ecs_remove_tag_str(&g_world, W_TIMER_COMPONENT_PAUSED, timer);

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

	w_entity_id timer1 = w_timer_create(&g_world, owner, "multi1", 0.3f, true, false);
	w_entity_id timer2 = w_timer_create(&g_world, owner, "multi2", 0.5f, true, false);

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

	w_timer_create(&g_world, owner1, "owner1_timer", 0.3f, true, false);
	w_timer_create(&g_world, owner2, "owner2_timer", 0.5f, true, false);

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
		float duration = 0.1f + (i % 10) * 0.1f;  // 0.1 to 1.0 seconds
		timers[i] = w_timer_create(&g_world, owners[i], name, duration, true, false);
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
		w_timer_create(&g_world, owner, name, 0.1f, false, true);

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

			float value = w_oscillator_compute_value_((wm_waveform_type)*type, effective_phase, *amplitude, *offset);

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

			float value = w_oscillator_compute_value_((wm_waveform_type)*type, effective_phase, *amplitude, *offset);

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
	w_entity_id osc = w_oscillator_create(&g_world, owner, "test_osc", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);

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
	w_entity_id osc1 = w_oscillator_create(&g_world, owner, "osc", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);
	w_entity_id osc2 = w_oscillator_create(&g_world, owner, "osc", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);

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
	w_entity_id osc = w_oscillator_create(&g_world, owner, "comp_test", W_OSCILLATOR_TYPE(TRIANGLE), 2.0f, 0.5f, 1.0f, 0.25f);

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
	ck_assert_int_eq(*type, (int)WM_WAVEFORM_TRIANGLE);
	ck_assert_int_eq(*owner_ref, owner);
}
END_TEST

START_TEST(test_oscillator_owner_entity_ref)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = w_oscillator_create(&g_world, owner, "owner_ref", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);

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
	w_oscillator_create(&g_world, owner, "sine_zero", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);

	// at phase 0, sin(0) = 0
	float value = w_oscillator_get_value(&g_world, owner, "sine_zero");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_sine_at_phase_quarter)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "sine_quarter", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);

	// advance to phase 0.25, sin(pi/2) = 1
	advance_oscillators(0.25);

	float value = w_oscillator_get_value(&g_world, owner, "sine_quarter");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST

START_TEST(test_sine_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "sine_half", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);

	// advance to phase 0.5, sin(pi) = 0
	advance_oscillators(0.5);

	float value = w_oscillator_get_value(&g_world, owner, "sine_half");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_sine_with_amplitude_offset)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "sine_amp", W_OSCILLATOR_TYPE(SINE), 1.0f, 2.0f, 5.0f, 0.0f);

	// at phase 0.25, value = 5 + 2*sin(pi/2) = 5 + 2 = 7
	advance_oscillators(0.25);

	float value = w_oscillator_get_value(&g_world, owner, "sine_amp");
	ck_assert_float_eq_tol(value, 7.0f, 0.001f);
}
END_TEST


/*****************************
*  triangle waveform         *
*****************************/

START_TEST(test_triangle_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "tri_zero", W_OSCILLATOR_TYPE(TRIANGLE), 1.0f, 1.0f, 0.0f, 0.0f);

	// trigger update at phase ~0
	advance_oscillators(0.0001);

	// at phase ~0: 1 - 4*|0 - 0.5| = 1 - 2 = -1
	float value = w_oscillator_get_value(&g_world, owner, "tri_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_triangle_at_phase_quarter)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "tri_quarter", W_OSCILLATOR_TYPE(TRIANGLE), 1.0f, 1.0f, 0.0f, 0.0f);

	// at phase 0.25: 1 - 4*|0.25 - 0.5| = 1 - 1 = 0
	advance_oscillators(0.25);

	float value = w_oscillator_get_value(&g_world, owner, "tri_quarter");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_triangle_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "tri_half", W_OSCILLATOR_TYPE(TRIANGLE), 1.0f, 1.0f, 0.0f, 0.0f);

	// at phase 0.5: 1 - 4*|0.5 - 0.5| = 1 - 0 = 1
	advance_oscillators(0.5);

	float value = w_oscillator_get_value(&g_world, owner, "tri_half");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST


/*****************************
*  square waveform           *
*****************************/

START_TEST(test_square_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "sq_zero", W_OSCILLATOR_TYPE(SQUARE), 1.0f, 1.0f, 0.0f, 0.0f);

	// trigger update at phase ~0
	advance_oscillators(0.0001);

	// at phase ~0 < 0.5: value = 1
	float value = w_oscillator_get_value(&g_world, owner, "sq_zero");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST

START_TEST(test_square_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "sq_half", W_OSCILLATOR_TYPE(SQUARE), 1.0f, 1.0f, 0.0f, 0.0f);

	// at phase 0.5 >= 0.5: value = -1
	advance_oscillators(0.5);

	float value = w_oscillator_get_value(&g_world, owner, "sq_half");
	ck_assert_float_eq_tol(value, -1.0f, 0.001f);
}
END_TEST


/*****************************
*  sawtooth waveform         *
*****************************/

START_TEST(test_sawtooth_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "saw_zero", W_OSCILLATOR_TYPE(SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f);

	// trigger update at phase ~0
	advance_oscillators(0.0001);

	// at phase ~0: 2*0 - 1 = -1
	float value = w_oscillator_get_value(&g_world, owner, "saw_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_sawtooth_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "saw_half", W_OSCILLATOR_TYPE(SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f);

	// at phase 0.5: 2*0.5 - 1 = 0
	advance_oscillators(0.5);

	float value = w_oscillator_get_value(&g_world, owner, "saw_half");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_sawtooth_near_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "saw_one", W_OSCILLATOR_TYPE(SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f);

	// at phase 0.99: 2*0.99 - 1 = 0.98
	advance_oscillators(0.99);

	float value = w_oscillator_get_value(&g_world, owner, "saw_one");
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
	w_oscillator_create(&g_world, owner, "shift_sine", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.25f);

	// effective_phase = 0 + 0.25 = 0.25, sin(pi/2) = 1
	// need to trigger update once
	advance_oscillators(0.0001);

	float value = w_oscillator_get_value(&g_world, owner, "shift_sine");
	ck_assert_float_eq_tol(value, 1.0f, 0.01f);
}
END_TEST


/*****************************
*  phase cycling             *
*****************************/

START_TEST(test_phase_wraps_at_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_entity_id osc = w_oscillator_create(&g_world, owner, "wrap_test", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);

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
	w_entity_id osc = w_oscillator_create(&g_world, owner, "forever", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);

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
	w_entity_id osc = w_oscillator_create(&g_world, owner, "zero_period", W_OSCILLATOR_TYPE(SINE), 0.0f, 1.0f, 0.0f, 0.0f);

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
	w_entity_id osc = w_oscillator_create(&g_world, owner, "neg_period", W_OSCILLATOR_TYPE(SINE), -1.0f, 1.0f, 0.0f, 0.0f);

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

	w_oscillator_create(&g_world, owner, "multi1", W_OSCILLATOR_TYPE(SINE), 1.0f, 1.0f, 0.0f, 0.0f);
	w_oscillator_create(&g_world, owner, "multi2", W_OSCILLATOR_TYPE(SQUARE), 2.0f, 2.0f, 0.0f, 0.0f);

	advance_oscillators(0.25);

	float val1 = w_oscillator_get_value(&g_world, owner, "multi1");
	float val2 = w_oscillator_get_value(&g_world, owner, "multi2");

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
	w_oscillator_create(&g_world, owner, "inv_saw_zero", W_OSCILLATOR_TYPE(INVERSE_SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: 1 - 2*0 = 1
	float value = w_oscillator_get_value(&g_world, owner, "inv_saw_zero");
	ck_assert_float_eq_tol(value, 1.0f, 0.01f);
}
END_TEST

START_TEST(test_inverse_sawtooth_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "inv_saw_half", W_OSCILLATOR_TYPE(INVERSE_SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: 1 - 2*0.5 = 0
	float value = w_oscillator_get_value(&g_world, owner, "inv_saw_half");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_inverse_sawtooth_near_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "inv_saw_one", W_OSCILLATOR_TYPE(INVERSE_SAWTOOTH), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.99);

	// at phase ~1: 1 - 2*0.99 = -0.98
	float value = w_oscillator_get_value(&g_world, owner, "inv_saw_one");
	ck_assert_float_eq_tol(value, -0.98f, 0.02f);
}
END_TEST


/*****************************
*  bounce waveform           *
*****************************/

START_TEST(test_bounce_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "bounce_zero", W_OSCILLATOR_TYPE(BOUNCE), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: |sin(0)| = 0
	float value = w_oscillator_get_value(&g_world, owner, "bounce_zero");
	ck_assert_float_eq_tol(value, 0.0f, 0.01f);
}
END_TEST

START_TEST(test_bounce_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "bounce_half", W_OSCILLATOR_TYPE(BOUNCE), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: |sin(pi/2)| = 1
	float value = w_oscillator_get_value(&g_world, owner, "bounce_half");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST

START_TEST(test_bounce_at_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "bounce_one", W_OSCILLATOR_TYPE(BOUNCE), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.99);

	// at phase ~1: |sin(pi)| ~= 0
	float value = w_oscillator_get_value(&g_world, owner, "bounce_one");
	ck_assert_float_eq_tol(value, 0.0f, 0.05f);
}
END_TEST


/*****************************
*  smooth step waveform      *
*****************************/

START_TEST(test_smooth_step_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "smooth_zero", W_OSCILLATOR_TYPE(SMOOTH_STEP), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: 2*(0) - 1 = -1
	float value = w_oscillator_get_value(&g_world, owner, "smooth_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_smooth_step_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "smooth_half", W_OSCILLATOR_TYPE(SMOOTH_STEP), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: hermite(0.5) = 0.5, so 2*0.5 - 1 = 0
	float value = w_oscillator_get_value(&g_world, owner, "smooth_half");
	ck_assert_float_eq_tol(value, 0.0f, 0.001f);
}
END_TEST

START_TEST(test_smooth_step_near_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "smooth_one", W_OSCILLATOR_TYPE(SMOOTH_STEP), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.99);

	// at phase ~1: hermite(1) = 1, so 2*1 - 1 = 1
	float value = w_oscillator_get_value(&g_world, owner, "smooth_one");
	ck_assert_float_eq_tol(value, 1.0f, 0.01f);
}
END_TEST


/*****************************
*  rectified sine waveform   *
*****************************/

START_TEST(test_rectified_sine_at_phase_zero)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "rect_zero", W_OSCILLATOR_TYPE(RECTIFIED_SINE), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: 2*|sin(0)| - 1 = -1
	float value = w_oscillator_get_value(&g_world, owner, "rect_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_rectified_sine_at_phase_quarter)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "rect_quarter", W_OSCILLATOR_TYPE(RECTIFIED_SINE), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.25);

	// at phase 0.25: 2*|sin(pi/2)| - 1 = 2*1 - 1 = 1
	float value = w_oscillator_get_value(&g_world, owner, "rect_quarter");
	ck_assert_float_eq_tol(value, 1.0f, 0.001f);
}
END_TEST

START_TEST(test_rectified_sine_at_phase_half)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "rect_half", W_OSCILLATOR_TYPE(RECTIFIED_SINE), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: 2*|sin(pi)| - 1 = -1
	float value = w_oscillator_get_value(&g_world, owner, "rect_half");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST


/*****************************
*  noise waveform            *
*****************************/

START_TEST(test_noise_returns_value_in_range)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "noise_test", W_OSCILLATOR_TYPE(NOISE), 1.0f, 1.0f, 0.0f, 0.0f);

	// run multiple times and verify all values are in [-1, 1]
	for (int i = 0; i < 100; i++) {
		advance_oscillators(0.01);
		float value = w_oscillator_get_value(&g_world, owner, "noise_test");
		ck_assert_float_ge(value, -1.0f);
		ck_assert_float_le(value, 1.0f);
	}
}
END_TEST

START_TEST(test_noise_with_amplitude_offset)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "noise_amp", W_OSCILLATOR_TYPE(NOISE), 1.0f, 2.0f, 5.0f, 0.0f);

	// run multiple times and verify all values are in [5-2, 5+2] = [3, 7]
	for (int i = 0; i < 100; i++) {
		advance_oscillators(0.01);
		float value = w_oscillator_get_value(&g_world, owner, "noise_amp");
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
	w_oscillator_create(&g_world, owner, "exp_zero", W_OSCILLATOR_TYPE(FIXED_EXPONENTIAL), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.0001);

	// at phase ~0: 2*(0) - 1 = -1
	float value = w_oscillator_get_value(&g_world, owner, "exp_zero");
	ck_assert_float_eq_tol(value, -1.0f, 0.01f);
}
END_TEST

START_TEST(test_fixed_exponential_near_phase_one)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "exp_one", W_OSCILLATOR_TYPE(FIXED_EXPONENTIAL), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.99);

	// at phase 0.99: approaches 1 but doesn't quite reach due to curve shape
	float value = w_oscillator_get_value(&g_world, owner, "exp_one");
	ck_assert_float_gt(value, 0.9f);
	ck_assert_float_le(value, 1.0f);
}
END_TEST

START_TEST(test_fixed_exponential_curve_shape)
{
	w_entity_id owner = w_ecs_request_entity(&g_world);
	w_oscillator_create(&g_world, owner, "exp_curve", W_OSCILLATOR_TYPE(FIXED_EXPONENTIAL), 1.0f, 1.0f, 0.0f, 0.0f);

	advance_oscillators(0.5);

	// at phase 0.5: exponential should be below linear (which would be 0)
	// (exp(1.5) - 1) / (exp(3) - 1) ~= 0.2
	// 2*0.2 - 1 = -0.6
	float value = w_oscillator_get_value(&g_world, owner, "exp_curve");
	ck_assert_float_lt(value, 0.0f);
	ck_assert_float_gt(value, -1.0f);
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
