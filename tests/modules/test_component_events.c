/**
 * @author      : ElGatoPanzon
 * @file        : test_component_events
 * @created     : Tuesday Mar 24, 2026 21:21:46 CST
 * @description : tests for component_events module (added/changed/removed tracking)
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/component_events/whisker_component_events.h"

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

// component IDs resolved in setup
static w_entity_id g_health_id;
static w_entity_id g_position_id;

#define COMP_HEALTH "test_health"
#define COMP_POSITION "test_position"

static void events_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_scheduler_defaults_init(&g_world, 60.0);

	wm_component_events_init(&g_world);

	// resolve test component IDs
	g_health_id = w_ecs_get_component_by_name(&g_world, COMP_HEALTH);
	g_position_id = w_ecs_get_component_by_name(&g_world, COMP_POSITION);

	// register components for event tracking
	wm_component_events_register(&g_world, g_health_id);
	wm_component_events_register(&g_world, g_position_id);
}

static void events_teardown(void)
{
	wm_component_events_free(&g_world);
	wm_scheduler_defaults_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  added events              *
*****************************/

START_TEST(test_added_event_fires_on_new_component)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	// set a component for the first time
	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	// the _added tag should be on the entity
	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	ck_assert(w_ecs_has_tag(&g_world, added_tag, e));
}
END_TEST

START_TEST(test_added_event_does_not_fire_on_overwrite)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_ADDED_SUFFIX);

	// remove the added tag manually to clear it
	w_ecs_remove_tag(&g_world, added_tag, e);

	// set same component again (overwrite, same value)
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	// added should NOT fire again (component already existed)
	ck_assert(!w_ecs_has_tag(&g_world, added_tag, e));
}
END_TEST

START_TEST(test_added_event_not_on_untracked_component)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	// set a component that is NOT registered for tracking
	int32_t val = 42;
	w_ecs_set_str(&g_world, int32_t, "untracked_comp", e, &val);

	// no event tag should exist
	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, "untracked_comp" WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	ck_assert(!w_ecs_has_tag(&g_world, added_tag, e));
}
END_TEST


/*****************************
*  changed events            *
*****************************/

START_TEST(test_changed_event_fires_on_value_change)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	// change the value
	float new_health = 50.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &new_health);

	w_entity_id changed_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_CHANGED_SUFFIX);
	ck_assert(w_ecs_has_tag(&g_world, changed_tag, e));
}
END_TEST

START_TEST(test_changed_event_does_not_fire_on_same_value)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	// set the same value again
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	w_entity_id changed_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_CHANGED_SUFFIX);
	ck_assert(!w_ecs_has_tag(&g_world, changed_tag, e));
}
END_TEST

START_TEST(test_changed_event_with_struct_component)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	w_vec2 pos = { .x = 1.0f, .y = 2.0f };
	w_ecs_set_str(&g_world, w_vec2, COMP_POSITION, e, &pos);

	// change only one field
	w_vec2 new_pos = { .x = 3.0f, .y = 2.0f };
	w_ecs_set_str(&g_world, w_vec2, COMP_POSITION, e, &new_pos);

	w_entity_id changed_tag = w_ecs_get_component_by_name(&g_world, COMP_POSITION WM_COMPONENT_EVENTS_CHANGED_SUFFIX);
	ck_assert(w_ecs_has_tag(&g_world, changed_tag, e));
}
END_TEST


/*****************************
*  removed events            *
*****************************/

START_TEST(test_removed_event_fires_on_remove)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	// remove the component
	w_ecs_remove_str(&g_world, COMP_HEALTH, e);

	w_entity_id removed_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_REMOVED_SUFFIX);
	ck_assert(w_ecs_has_tag(&g_world, removed_tag, e));
}
END_TEST

START_TEST(test_removed_event_not_on_untracked_component)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	int32_t val = 42;
	w_ecs_set_str(&g_world, int32_t, "untracked_remove", e, &val);
	w_ecs_remove_str(&g_world, "untracked_remove", e);

	w_entity_id removed_tag = w_ecs_get_component_by_name(&g_world, "untracked_remove" WM_COMPONENT_EVENTS_REMOVED_SUFFIX);
	ck_assert(!w_ecs_has_tag(&g_world, removed_tag, e));
}
END_TEST


/*****************************
*  cleanup helper            *
*****************************/

// helper: drain the removal buffer directly
static void run_cleanup(void)
{
	struct wm_component_events_registry *reg = wm_component_events_get_registry(&g_world);
	if (!reg) return;

	// index-based loop: length can grow during iteration
	for (size_t i = 0; i < reg->removal_buffer_length; i++)
	{
		w_pack32x2 pair = reg->removal_buffer[i];
		w_entity_id owner = pair.left;
		w_entity_id tag_id = pair.right;
		w_component_remove(&g_world.components, tag_id, owner);
	}

	reg->removal_buffer_length = 0;
}

START_TEST(test_cleanup_removes_event_tags)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	ck_assert(w_ecs_has_tag(&g_world, added_tag, e));

	// run cleanup
	run_cleanup();

	// event tag should be gone
	ck_assert(!w_ecs_has_tag(&g_world, added_tag, e));
}
END_TEST

START_TEST(test_cleanup_removes_all_event_types)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	// trigger added
	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	// trigger changed
	float new_health = 50.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &new_health);

	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	w_entity_id changed_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_CHANGED_SUFFIX);

	ck_assert(w_ecs_has_tag(&g_world, added_tag, e));
	ck_assert(w_ecs_has_tag(&g_world, changed_tag, e));

	run_cleanup();

	ck_assert(!w_ecs_has_tag(&g_world, added_tag, e));
	ck_assert(!w_ecs_has_tag(&g_world, changed_tag, e));
}
END_TEST

START_TEST(test_cleanup_drains_removal_buffer)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	struct wm_component_events_registry *reg = wm_component_events_get_registry(&g_world);
	ck_assert_uint_eq(reg->removal_buffer_length, 0);

	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	// removal buffer should have an entry queued
	ck_assert_uint_gt(reg->removal_buffer_length, 0);

	run_cleanup();

	// removal buffer should be drained
	ck_assert_uint_eq(reg->removal_buffer_length, 0);
}
END_TEST


/*****************************
*  w_ecs_set_tracked macro   *
*****************************/

START_TEST(test_set_tracked_registers_and_fires)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	// use a fresh component that hasn't been registered yet
	w_entity_id comp = w_ecs_get_component_by_name(&g_world, "tracked_test");
	float val = 1.0f;
	w_ecs_set_tracked(&g_world, float, comp, e, &val);

	// should have _added tag
	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, "tracked_test" WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	ck_assert(w_ecs_has_tag(&g_world, added_tag, e));
}
END_TEST

START_TEST(test_set_tracked_str_registers_and_fires)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	float val = 1.0f;
	w_ecs_set_tracked_str(&g_world, float, "tracked_str_test", e, &val);

	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, "tracked_str_test" WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	ck_assert(w_ecs_has_tag(&g_world, added_tag, e));
}
END_TEST


/*****************************
*  multiple entities         *
*****************************/

START_TEST(test_events_independent_per_entity)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);

	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e1, &health);

	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_ADDED_SUFFIX);

	// e1 should have added tag, e2 should not
	ck_assert(w_ecs_has_tag(&g_world, added_tag, e1));
	ck_assert(!w_ecs_has_tag(&g_world, added_tag, e2));
}
END_TEST

START_TEST(test_events_multiple_components_on_same_entity)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	float health = 100.0f;
	w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &health);

	w_vec2 pos = { .x = 1.0f, .y = 2.0f };
	w_ecs_set_str(&g_world, w_vec2, COMP_POSITION, e, &pos);

	w_entity_id health_added = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	w_entity_id pos_added = w_ecs_get_component_by_name(&g_world, COMP_POSITION WM_COMPONENT_EVENTS_ADDED_SUFFIX);

	ck_assert(w_ecs_has_tag(&g_world, health_added, e));
	ck_assert(w_ecs_has_tag(&g_world, pos_added, e));
}
END_TEST


/*****************************
*  stress test               *
*****************************/

START_TEST(test_stress_many_events)
{
	int num = 100;
	w_entity_id *entities = malloc(num * sizeof(w_entity_id));

	for (int i = 0; i < num; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		float val = (float)i;
		w_ecs_set_str(&g_world, float, COMP_HEALTH, entities[i], &val);
	}

	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_ADDED_SUFFIX);

	// all should have added tag
	for (int i = 0; i < num; i++)
		ck_assert(w_ecs_has_tag(&g_world, added_tag, entities[i]));

	// cleanup should remove all
	run_cleanup();

	for (int i = 0; i < num; i++)
		ck_assert(!w_ecs_has_tag(&g_world, added_tag, entities[i]));

	free(entities);
}
END_TEST

START_TEST(test_stress_add_change_remove_cycle)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	w_entity_id added_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	w_entity_id changed_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_CHANGED_SUFFIX);
	w_entity_id removed_tag = w_ecs_get_component_by_name(&g_world, COMP_HEALTH WM_COMPONENT_EVENTS_REMOVED_SUFFIX);

	for (int cycle = 0; cycle < 10; cycle++)
	{
		// add
		float val = (float)cycle;
		w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &val);
		ck_assert(w_ecs_has_tag(&g_world, added_tag, e));
		run_cleanup();

		// change
		float new_val = val + 1.0f;
		w_ecs_set_str(&g_world, float, COMP_HEALTH, e, &new_val);
		ck_assert(w_ecs_has_tag(&g_world, changed_tag, e));
		run_cleanup();

		// remove
		w_ecs_remove_str(&g_world, COMP_HEALTH, e);
		ck_assert(w_ecs_has_tag(&g_world, removed_tag, e));
		run_cleanup();
	}
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *component_events_suite(void)
{
	Suite *s = suite_create("component_events");

	TCase *tc_added = tcase_create("added_events");
	tcase_add_checked_fixture(tc_added, events_setup, events_teardown);
	tcase_set_timeout(tc_added, 10);
	tcase_add_test(tc_added, test_added_event_fires_on_new_component);
	tcase_add_test(tc_added, test_added_event_does_not_fire_on_overwrite);
	tcase_add_test(tc_added, test_added_event_not_on_untracked_component);
	suite_add_tcase(s, tc_added);

	TCase *tc_changed = tcase_create("changed_events");
	tcase_add_checked_fixture(tc_changed, events_setup, events_teardown);
	tcase_set_timeout(tc_changed, 10);
	tcase_add_test(tc_changed, test_changed_event_fires_on_value_change);
	tcase_add_test(tc_changed, test_changed_event_does_not_fire_on_same_value);
	tcase_add_test(tc_changed, test_changed_event_with_struct_component);
	suite_add_tcase(s, tc_changed);

	TCase *tc_removed = tcase_create("removed_events");
	tcase_add_checked_fixture(tc_removed, events_setup, events_teardown);
	tcase_set_timeout(tc_removed, 10);
	tcase_add_test(tc_removed, test_removed_event_fires_on_remove);
	tcase_add_test(tc_removed, test_removed_event_not_on_untracked_component);
	suite_add_tcase(s, tc_removed);

	TCase *tc_cleanup = tcase_create("cleanup_system");
	tcase_add_checked_fixture(tc_cleanup, events_setup, events_teardown);
	tcase_set_timeout(tc_cleanup, 10);
	tcase_add_test(tc_cleanup, test_cleanup_removes_event_tags);
	tcase_add_test(tc_cleanup, test_cleanup_removes_all_event_types);
	tcase_add_test(tc_cleanup, test_cleanup_drains_removal_buffer);
	suite_add_tcase(s, tc_cleanup);

	TCase *tc_tracked = tcase_create("set_tracked_macro");
	tcase_add_checked_fixture(tc_tracked, events_setup, events_teardown);
	tcase_set_timeout(tc_tracked, 10);
	tcase_add_test(tc_tracked, test_set_tracked_registers_and_fires);
	tcase_add_test(tc_tracked, test_set_tracked_str_registers_and_fires);
	suite_add_tcase(s, tc_tracked);

	TCase *tc_multi = tcase_create("multiple_entities");
	tcase_add_checked_fixture(tc_multi, events_setup, events_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_events_independent_per_entity);
	tcase_add_test(tc_multi, test_events_multiple_components_on_same_entity);
	suite_add_tcase(s, tc_multi);

	TCase *tc_stress = tcase_create("stress");
	tcase_add_checked_fixture(tc_stress, events_setup, events_teardown);
	tcase_set_timeout(tc_stress, 30);
	tcase_add_test(tc_stress, test_stress_many_events);
	tcase_add_test(tc_stress, test_stress_add_change_remove_cycle);
	suite_add_tcase(s, tc_stress);

	return s;
}

int main(void)
{
	Suite *s = component_events_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
