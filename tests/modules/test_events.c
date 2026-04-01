/**
 * @author      : ElGatoPanzon
 * @file        : test_events
 * @created     : Tuesday Apr 01, 2026 17:06:17 CST
 * @description : tests for fire-and-forget events module
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/events/whisker_events.h"

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

#define EVENT_TEST "test_event"
#define EVENT_TEST2 "test_event2"

static void events_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_scheduler_defaults_init(&g_world, 60.0);

	wm_events_init(&g_world);
}

static void events_teardown(void)
{
	wm_events_free(&g_world);
	wm_scheduler_defaults_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  scheduler helper          *
*****************************/

// run one scheduler cycle to trigger cleanup hooks
static void run_scheduler_cycle(void)
{
	w_ecs_update(&g_world);
}


/*****************************
*  w_event_fire              *
*****************************/

START_TEST(test_event_fire_creates_entity_with_tag)
{
	w_entity_id e = w_event_fire(&g_world, EVENT_TEST);

	// entity should be valid
	ck_assert(w_ecs_is_valid_entity(e));

	// entity should have the event tag
	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);
	ck_assert(w_ecs_has_tag(&g_world, event_tag, e));
}
END_TEST

START_TEST(test_event_fire_marks_for_destroy_end_of_frame)
{
	w_entity_id e = w_event_fire(&g_world, EVENT_TEST);

	// entity should have destroy_end_of_frame tag
	w_entity_id destroy_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_TAG);
	w_entity_id destroy_eof_tag = w_ecs_get_component_by_name(&g_world, W_ENTITY_LIFECYCLE_DESTROY_END_OF_FRAME_TAG);

	ck_assert(w_ecs_has_tag(&g_world, destroy_tag, e));
	ck_assert(w_ecs_has_tag(&g_world, destroy_eof_tag, e));
}
END_TEST

START_TEST(test_event_fire_returns_entity_id)
{
	w_entity_id e1 = w_event_fire(&g_world, EVENT_TEST);
	w_entity_id e2 = w_event_fire(&g_world, EVENT_TEST2);

	// should return different entities
	ck_assert(e1 != e2);
	ck_assert(w_ecs_is_valid_entity(e1));
	ck_assert(w_ecs_is_valid_entity(e2));
}
END_TEST


/*****************************
*  w_event_set_data          *
*****************************/

START_TEST(test_event_set_data_combined_name)
{
	w_entity_id e = w_event_fire(&g_world, EVENT_TEST);

	int32_t data = 42;
	w_event_set_data(&g_world, e, EVENT_TEST, int32_t, "payload", &data);

	// component should exist with combined name
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, EVENT_TEST "_payload");
	ck_assert(w_ecs_has(&g_world, comp_id, e));

	// data should be correct
	int32_t *stored = w_ecs_get_str(&g_world, int32_t, EVENT_TEST "_payload", e);
	ck_assert_ptr_nonnull(stored);
	ck_assert_int_eq(*stored, 42);
}
END_TEST

START_TEST(test_event_set_data_multiple_data_components)
{
	w_entity_id e = w_event_fire(&g_world, EVENT_TEST);

	int32_t int_data = 100;
	float float_data = 3.14f;

	w_event_set_data(&g_world, e, EVENT_TEST, int32_t, "count", &int_data);
	w_event_set_data(&g_world, e, EVENT_TEST, float, "value", &float_data);

	w_entity_id count_id = w_ecs_get_component_by_name(&g_world, EVENT_TEST "_count");
	w_entity_id value_id = w_ecs_get_component_by_name(&g_world, EVENT_TEST "_value");

	ck_assert(w_ecs_has(&g_world, count_id, e));
	ck_assert(w_ecs_has(&g_world, value_id, e));

	int32_t *count = w_ecs_get_str(&g_world, int32_t, EVENT_TEST "_count", e);
	float *value = w_ecs_get_str(&g_world, float, EVENT_TEST "_value", e);

	ck_assert_int_eq(*count, 100);
	ck_assert_float_eq(*value, 3.14f);
}
END_TEST


/*****************************
*  w_event_fire_on           *
*****************************/

START_TEST(test_event_fire_on_sets_tag)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	w_event_fire_on(&g_world, e, EVENT_TEST);

	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);
	ck_assert(w_ecs_has_tag(&g_world, event_tag, e));
}
END_TEST

START_TEST(test_event_fire_on_queues_cleanup)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	w_event_fire_on(&g_world, e, EVENT_TEST);

	// verify removal buffer has an entry
	struct wm_events_state *state = wm_events_get_state(&g_world);
	ck_assert_uint_gt(state->removal_buffer_length, 0);
}
END_TEST

START_TEST(test_event_fire_on_cleanup_removes_tag)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	w_event_fire_on(&g_world, e, EVENT_TEST);

	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);
	ck_assert(w_ecs_has_tag(&g_world, event_tag, e));

	run_scheduler_cycle();

	// tag should be removed but entity survives
	ck_assert(!w_ecs_has_tag(&g_world, event_tag, e));
	ck_assert(w_ecs_is_valid_entity(e));
}
END_TEST

START_TEST(test_event_fire_on_entity_survives)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	// add a persistent component
	int32_t health = 100;
	w_ecs_set_str(&g_world, int32_t, "health", e, &health);

	// fire event on the entity
	w_event_fire_on(&g_world, e, EVENT_TEST);

	run_scheduler_cycle();

	// entity should still exist with its persistent component
	ck_assert(w_ecs_is_valid_entity(e));
	w_entity_id health_id = w_ecs_get_component_by_name(&g_world, "health");
	ck_assert(w_ecs_has(&g_world, health_id, e));

	int32_t *stored_health = w_ecs_get_str(&g_world, int32_t, "health", e);
	ck_assert_int_eq(*stored_health, 100);
}
END_TEST


/*****************************
*  w_event_fire_on_str       *
*****************************/

START_TEST(test_event_fire_on_str_finds_entity)
{
	w_entity_id e = w_ecs_request_entity_with_name(&g_world, "named_entity");

	w_event_fire_on_str(&g_world, "named_entity", EVENT_TEST);

	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);
	ck_assert(w_ecs_has_tag(&g_world, event_tag, e));
}
END_TEST

START_TEST(test_event_fire_on_str_invalid_name_noop)
{
	// fire on non-existent entity should not crash
	w_event_fire_on_str(&g_world, "nonexistent_entity", EVENT_TEST);

	// removal buffer should be empty
	struct wm_events_state *state = wm_events_get_state(&g_world);
	ck_assert_uint_eq(state->removal_buffer_length, 0);
}
END_TEST

START_TEST(test_event_fire_on_str_cleanup)
{
	w_entity_id e = w_ecs_request_entity_with_name(&g_world, "named_entity");

	w_event_fire_on_str(&g_world, "named_entity", EVENT_TEST);

	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);
	ck_assert(w_ecs_has_tag(&g_world, event_tag, e));

	run_scheduler_cycle();

	ck_assert(!w_ecs_has_tag(&g_world, event_tag, e));
	ck_assert(w_ecs_is_valid_entity(e));
}
END_TEST


/*****************************
*  w_event_set_data_on       *
*****************************/

START_TEST(test_event_set_data_on_sets_component)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	int32_t data = 99;
	w_event_set_data_on(&g_world, e, EVENT_TEST, int32_t, "payload", &data);

	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, EVENT_TEST "_payload");
	ck_assert(w_ecs_has(&g_world, comp_id, e));

	int32_t *stored = w_ecs_get_str(&g_world, int32_t, EVENT_TEST "_payload", e);
	ck_assert_int_eq(*stored, 99);
}
END_TEST

START_TEST(test_event_set_data_on_queues_cleanup)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	int32_t data = 99;
	w_event_set_data_on(&g_world, e, EVENT_TEST, int32_t, "payload", &data);

	struct wm_events_state *state = wm_events_get_state(&g_world);
	ck_assert_uint_gt(state->removal_buffer_length, 0);
}
END_TEST

START_TEST(test_event_set_data_on_cleanup_removes_component)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	int32_t data = 99;
	w_event_set_data_on(&g_world, e, EVENT_TEST, int32_t, "payload", &data);

	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, EVENT_TEST "_payload");
	ck_assert(w_ecs_has(&g_world, comp_id, e));

	run_scheduler_cycle();

	ck_assert(!w_ecs_has(&g_world, comp_id, e));
	ck_assert(w_ecs_is_valid_entity(e));
}
END_TEST


/*****************************
*  multiple events           *
*****************************/

START_TEST(test_multiple_events_same_entity_all_cleaned)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	w_event_fire_on(&g_world, e, EVENT_TEST);
	w_event_fire_on(&g_world, e, EVENT_TEST2);

	w_entity_id tag1 = w_ecs_get_component_by_name(&g_world, EVENT_TEST);
	w_entity_id tag2 = w_ecs_get_component_by_name(&g_world, EVENT_TEST2);

	ck_assert(w_ecs_has_tag(&g_world, tag1, e));
	ck_assert(w_ecs_has_tag(&g_world, tag2, e));

	run_scheduler_cycle();

	ck_assert(!w_ecs_has_tag(&g_world, tag1, e));
	ck_assert(!w_ecs_has_tag(&g_world, tag2, e));
	ck_assert(w_ecs_is_valid_entity(e));
}
END_TEST

START_TEST(test_multiple_data_events_all_cleaned)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	int32_t data1 = 1;
	int32_t data2 = 2;
	w_event_set_data_on(&g_world, e, EVENT_TEST, int32_t, "data1", &data1);
	w_event_set_data_on(&g_world, e, EVENT_TEST, int32_t, "data2", &data2);

	w_entity_id comp1 = w_ecs_get_component_by_name(&g_world, EVENT_TEST "_data1");
	w_entity_id comp2 = w_ecs_get_component_by_name(&g_world, EVENT_TEST "_data2");

	ck_assert(w_ecs_has(&g_world, comp1, e));
	ck_assert(w_ecs_has(&g_world, comp2, e));

	run_scheduler_cycle();

	ck_assert(!w_ecs_has(&g_world, comp1, e));
	ck_assert(!w_ecs_has(&g_world, comp2, e));
}
END_TEST


/*****************************
*  queryable during frame    *
*****************************/

START_TEST(test_events_queryable_before_cleanup)
{
	w_entity_id e1 = w_event_fire(&g_world, EVENT_TEST);
	w_entity_id e2 = w_event_fire(&g_world, EVENT_TEST);
	w_entity_id e3 = w_ecs_request_entity(&g_world);
	w_event_fire_on(&g_world, e3, EVENT_TEST);

	// all three should be queryable by the event tag
	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);

	ck_assert(w_ecs_has_tag(&g_world, event_tag, e1));
	ck_assert(w_ecs_has_tag(&g_world, event_tag, e2));
	ck_assert(w_ecs_has_tag(&g_world, event_tag, e3));

	// rebuild cache before query
	char *query_str = w_query_read(EVENT_TEST);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);

	// query count
	int count = 0;
	w_query_for_each(&g_world, query_str, {
		count++;
	});
	ck_assert_int_eq(count, 3);
}
END_TEST

START_TEST(test_events_with_data_queryable)
{
	w_entity_id e = w_event_fire(&g_world, EVENT_TEST);
	int32_t damage = 50;
	w_event_set_data(&g_world, e, EVENT_TEST, int32_t, "damage", &damage);

	// rebuild cache before query
	char *query_str = w_query_read(EVENT_TEST) w_query_read(EVENT_TEST "_damage");
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);

	// should be able to query both tag and data
	int count = 0;
	w_query_for_each(&g_world, query_str, {
		// verify entity has the expected data via direct lookup
		int32_t *dmg = w_ecs_get_str(&g_world, int32_t, EVENT_TEST "_damage", itor.entity_id);
		ck_assert_int_eq(*dmg, 50);
		count++;
	});
	ck_assert_int_eq(count, 1);
}
END_TEST


/*****************************
*  cleanup drains buffer     *
*****************************/

START_TEST(test_cleanup_drains_removal_buffer)
{
	struct wm_events_state *state = wm_events_get_state(&g_world);
	ck_assert_uint_eq(state->removal_buffer_length, 0);

	w_entity_id e = w_ecs_request_entity(&g_world);
	w_event_fire_on(&g_world, e, EVENT_TEST);

	ck_assert_uint_gt(state->removal_buffer_length, 0);

	run_scheduler_cycle();

	ck_assert_uint_eq(state->removal_buffer_length, 0);
}
END_TEST

START_TEST(test_cleanup_hook_fires_via_scheduler)
{
	// verify cleanup happens through the actual scheduler hook, not manual calls
	w_entity_id e = w_ecs_request_entity(&g_world);

	// add persistent component
	int32_t health = 100;
	w_ecs_set_str(&g_world, int32_t, "health", e, &health);

	// fire event and set data on entity
	w_event_fire_on(&g_world, e, EVENT_TEST);
	int32_t damage = 50;
	w_event_set_data_on(&g_world, e, EVENT_TEST, int32_t, "damage", &damage);

	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);
	w_entity_id damage_comp = w_ecs_get_component_by_name(&g_world, EVENT_TEST "_damage");

	// verify components exist before scheduler cycle
	ck_assert(w_ecs_has_tag(&g_world, event_tag, e));
	ck_assert(w_ecs_has(&g_world, damage_comp, e));

	// run scheduler - wm_events_cleanup hook fires at UPDATE_END
	run_scheduler_cycle();

	// event components removed by hook
	ck_assert(!w_ecs_has_tag(&g_world, event_tag, e));
	ck_assert(!w_ecs_has(&g_world, damage_comp, e));

	// entity and persistent component survive
	ck_assert(w_ecs_is_valid_entity(e));
	w_entity_id health_comp = w_ecs_get_component_by_name(&g_world, "health");
	ck_assert(w_ecs_has(&g_world, health_comp, e));
}
END_TEST


/*****************************
*  stress tests              *
*****************************/

START_TEST(test_stress_many_fire_events)
{
	int num = 100;
	w_entity_id *entities = malloc(num * sizeof(w_entity_id));

	for (int i = 0; i < num; i++)
	{
		entities[i] = w_event_fire(&g_world, EVENT_TEST);
	}

	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);

	// all should have the tag
	for (int i = 0; i < num; i++)
		ck_assert(w_ecs_has_tag(&g_world, event_tag, entities[i]));

	free(entities);
}
END_TEST

START_TEST(test_stress_many_fire_on_events)
{
	int num = 100;
	w_entity_id *entities = malloc(num * sizeof(w_entity_id));

	for (int i = 0; i < num; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		w_event_fire_on(&g_world, entities[i], EVENT_TEST);
	}

	w_entity_id event_tag = w_ecs_get_component_by_name(&g_world, EVENT_TEST);

	// all should have the tag
	for (int i = 0; i < num; i++)
		ck_assert(w_ecs_has_tag(&g_world, event_tag, entities[i]));

	run_scheduler_cycle();

	// all tags removed, entities survive
	for (int i = 0; i < num; i++)
	{
		ck_assert(!w_ecs_has_tag(&g_world, event_tag, entities[i]));
		ck_assert(w_ecs_is_valid_entity(entities[i]));
	}

	free(entities);
}
END_TEST

START_TEST(test_stress_mixed_events_cleanup)
{
	int num = 50;

	for (int i = 0; i < num; i++)
	{
		// fire standalone events (entity destroyed at end of frame)
		w_event_fire(&g_world, EVENT_TEST);

		// fire on existing entities (component removed, entity survives)
		w_entity_id e = w_ecs_request_entity(&g_world);
		w_event_fire_on(&g_world, e, EVENT_TEST2);

		int32_t data = i;
		w_event_set_data_on(&g_world, e, EVENT_TEST2, int32_t, "idx", &data);
	}

	struct wm_events_state *state = wm_events_get_state(&g_world);
	ck_assert_uint_gt(state->removal_buffer_length, 0);

	run_scheduler_cycle();

	ck_assert_uint_eq(state->removal_buffer_length, 0);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *events_suite(void)
{
	Suite *s = suite_create("events");

	TCase *tc_fire = tcase_create("w_event_fire");
	tcase_add_checked_fixture(tc_fire, events_setup, events_teardown);
	tcase_set_timeout(tc_fire, 10);
	tcase_add_test(tc_fire, test_event_fire_creates_entity_with_tag);
	tcase_add_test(tc_fire, test_event_fire_marks_for_destroy_end_of_frame);
	tcase_add_test(tc_fire, test_event_fire_returns_entity_id);
	suite_add_tcase(s, tc_fire);

	TCase *tc_set_data = tcase_create("w_event_set_data");
	tcase_add_checked_fixture(tc_set_data, events_setup, events_teardown);
	tcase_set_timeout(tc_set_data, 10);
	tcase_add_test(tc_set_data, test_event_set_data_combined_name);
	tcase_add_test(tc_set_data, test_event_set_data_multiple_data_components);
	suite_add_tcase(s, tc_set_data);

	TCase *tc_fire_on = tcase_create("w_event_fire_on");
	tcase_add_checked_fixture(tc_fire_on, events_setup, events_teardown);
	tcase_set_timeout(tc_fire_on, 10);
	tcase_add_test(tc_fire_on, test_event_fire_on_sets_tag);
	tcase_add_test(tc_fire_on, test_event_fire_on_queues_cleanup);
	tcase_add_test(tc_fire_on, test_event_fire_on_cleanup_removes_tag);
	tcase_add_test(tc_fire_on, test_event_fire_on_entity_survives);
	suite_add_tcase(s, tc_fire_on);

	TCase *tc_fire_on_str = tcase_create("w_event_fire_on_str");
	tcase_add_checked_fixture(tc_fire_on_str, events_setup, events_teardown);
	tcase_set_timeout(tc_fire_on_str, 10);
	tcase_add_test(tc_fire_on_str, test_event_fire_on_str_finds_entity);
	tcase_add_test(tc_fire_on_str, test_event_fire_on_str_invalid_name_noop);
	tcase_add_test(tc_fire_on_str, test_event_fire_on_str_cleanup);
	suite_add_tcase(s, tc_fire_on_str);

	TCase *tc_data_on = tcase_create("w_event_set_data_on");
	tcase_add_checked_fixture(tc_data_on, events_setup, events_teardown);
	tcase_set_timeout(tc_data_on, 10);
	tcase_add_test(tc_data_on, test_event_set_data_on_sets_component);
	tcase_add_test(tc_data_on, test_event_set_data_on_queues_cleanup);
	tcase_add_test(tc_data_on, test_event_set_data_on_cleanup_removes_component);
	suite_add_tcase(s, tc_data_on);

	TCase *tc_multi = tcase_create("multiple_events");
	tcase_add_checked_fixture(tc_multi, events_setup, events_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_multiple_events_same_entity_all_cleaned);
	tcase_add_test(tc_multi, test_multiple_data_events_all_cleaned);
	suite_add_tcase(s, tc_multi);

	TCase *tc_query = tcase_create("queryable");
	tcase_add_checked_fixture(tc_query, events_setup, events_teardown);
	tcase_set_timeout(tc_query, 10);
	tcase_add_test(tc_query, test_events_queryable_before_cleanup);
	tcase_add_test(tc_query, test_events_with_data_queryable);
	suite_add_tcase(s, tc_query);

	TCase *tc_cleanup = tcase_create("cleanup_system");
	tcase_add_checked_fixture(tc_cleanup, events_setup, events_teardown);
	tcase_set_timeout(tc_cleanup, 10);
	tcase_add_test(tc_cleanup, test_cleanup_drains_removal_buffer);
	tcase_add_test(tc_cleanup, test_cleanup_hook_fires_via_scheduler);
	suite_add_tcase(s, tc_cleanup);

	TCase *tc_stress = tcase_create("stress");
	tcase_add_checked_fixture(tc_stress, events_setup, events_teardown);
	tcase_set_timeout(tc_stress, 30);
	tcase_add_test(tc_stress, test_stress_many_fire_events);
	tcase_add_test(tc_stress, test_stress_many_fire_on_events);
	tcase_add_test(tc_stress, test_stress_mixed_events_cleanup);
	suite_add_tcase(s, tc_stress);

	return s;
}

int main(void)
{
	Suite *s = events_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
