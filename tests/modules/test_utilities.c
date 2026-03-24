/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_utilities
 * @created     : Monday Mar 24, 2026 16:15:00 CST
 * @description : tests for whisker_utilities module (timer system)
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
*  suite + runner            *
*****************************/

Suite *utilities_suite(void)
{
	Suite *s = suite_create("utilities_timer");

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
