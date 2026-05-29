/**
 * @author      : ElGatoPanzon
 * @file        : test_streams
 * @created     : Wednesday May 27, 2026 18:40:00 CST
 * @description : tests for streams module
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/managed_alloc/whisker_managed_alloc.h"
#include "modules/streams/whisker_streams.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;

static void streams_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);

	wm_scheduler_defaults_init(&g_world, 60.0);
	wm_managed_alloc_init(&g_world);
	wm_streams_init(&g_world);
}

static void streams_teardown(void)
{
	wm_streams_free(&g_world);
	wm_managed_alloc_free(&g_world);
	wm_scheduler_defaults_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}

static void run_phase(size_t phase_id)
{
	w_ecs_run_phase_systems(&g_world, phase_id);
}

static w_entity_id create_input_stream_entity(uint64_t size)
{
	w_entity_id e = w_ecs_request_entity(&g_world);
	stream_input_buffer_size_set_value(&g_world, e, size);
	return e;
}

static w_entity_id create_output_stream_entity(uint64_t size)
{
	w_entity_id e = w_ecs_request_entity(&g_world);
	stream_output_buffer_size_set_value(&g_world, e, size);
	return e;
}

static void request_input_hot(w_entity_id e)
{
	req_stream_input_buffer_hot_set_tag_state(&g_world, e, true);
	run_phase(WM_STREAM_PHASE_INPUT_PRE_CONSUME);
}

static void request_output_hot(w_entity_id e)
{
	req_stream_output_buffer_hot_set_tag_state(&g_world, e, true);
	run_phase(WM_STREAM_PHASE_OUTPUT_PRE_CONSUME);
}


/*****************************
*  input lifecycle           *
*****************************/

START_TEST(test_input_hot_allocates_buffer)
{
	w_entity_id e = create_input_stream_entity(128);

	request_input_hot(e);

	ck_assert(stream_input_buffer_handle_exists(&g_world, e));
	ck_assert_uint_ne(*stream_input_buffer_handle_get(&g_world, e), WM_MANAGED_ALLOC_INVALID_HANDLE);
	ck_assert_ptr_nonnull(wm_managed_alloc_resolve_handle(&g_world, stream_input_buffer_handle, e));
	ck_assert_uint_eq(*stream_input_buffer_offset_get(&g_world, e), 0);
	ck_assert_uint_eq(*stream_input_buffer_length_get(&g_world, e), 0);
	ck_assert(!stream_input_available_tag_exists(&g_world, e));
	ck_assert(!req_stream_input_buffer_hot_tag_exists(&g_world, e));
}
END_TEST

START_TEST(test_input_hot_uses_requested_size)
{
	w_entity_id e = create_input_stream_entity(256);

	request_input_hot(e);

	w_entity_id comp_id = stream_input_buffer_handle_get_id(&g_world);
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	struct w_slab_arena *arena = &registry->slab_arenas[comp_id];
	uint64_t handle = *stream_input_buffer_handle_get(&g_world, e);
	struct w_slab_arena_entry *entry = w_slab_arena_get_handle_entry(arena, (size_t)handle);

	ck_assert_uint_eq(entry->actual_size, 256);
}
END_TEST

START_TEST(test_input_cold_frees_and_removes_buffer)
{
	w_entity_id e = create_input_stream_entity(128);
	request_input_hot(e);

	stream_input_available_set_tag_state(&g_world, e, true);
	stream_input_buffer_offset_set_value(&g_world, e, 4);
	stream_input_buffer_length_set_value(&g_world, e, 10);
	req_stream_input_buffer_cold_set_tag_state(&g_world, e, true);

	run_phase(WM_STREAM_PHASE_INPUT_POST_CONSUME);

	ck_assert(!stream_input_buffer_handle_exists(&g_world, e));
	ck_assert(!stream_input_available_tag_exists(&g_world, e));
	ck_assert(!req_stream_input_buffer_cold_tag_exists(&g_world, e));
}
END_TEST


/*****************************
*  output lifecycle          *
*****************************/

START_TEST(test_output_hot_allocates_buffer)
{
	w_entity_id e = create_output_stream_entity(128);

	request_output_hot(e);

	ck_assert(stream_output_buffer_handle_exists(&g_world, e));
	ck_assert_uint_ne(*stream_output_buffer_handle_get(&g_world, e), WM_MANAGED_ALLOC_INVALID_HANDLE);
	ck_assert_ptr_nonnull(wm_managed_alloc_resolve_handle(&g_world, stream_output_buffer_handle, e));
	ck_assert_uint_eq(*stream_output_buffer_offset_get(&g_world, e), 0);
	ck_assert_uint_eq(*stream_output_buffer_length_get(&g_world, e), 0);
	ck_assert(!stream_output_available_tag_exists(&g_world, e));
	ck_assert(!req_stream_output_buffer_hot_tag_exists(&g_world, e));
}
END_TEST

START_TEST(test_output_hot_uses_requested_size)
{
	w_entity_id e = create_output_stream_entity(512);

	request_output_hot(e);

	w_entity_id comp_id = stream_output_buffer_handle_get_id(&g_world);
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	struct w_slab_arena *arena = &registry->slab_arenas[comp_id];
	uint64_t handle = *stream_output_buffer_handle_get(&g_world, e);
	struct w_slab_arena_entry *entry = w_slab_arena_get_handle_entry(arena, (size_t)handle);

	ck_assert_uint_eq(entry->actual_size, 512);
}
END_TEST

START_TEST(test_output_cold_frees_and_removes_buffer)
{
	w_entity_id e = create_output_stream_entity(128);
	request_output_hot(e);

	stream_output_available_set_tag_state(&g_world, e, true);
	stream_output_buffer_offset_set_value(&g_world, e, 4);
	stream_output_buffer_length_set_value(&g_world, e, 10);
	req_stream_output_buffer_cold_set_tag_state(&g_world, e, true);

	run_phase(WM_STREAM_PHASE_OUTPUT_POST_CONSUME);

	ck_assert(!stream_output_buffer_handle_exists(&g_world, e));
	ck_assert(!stream_output_available_tag_exists(&g_world, e));
	ck_assert(!req_stream_output_buffer_cold_tag_exists(&g_world, e));
}
END_TEST


/*****************************
*  input compact             *
*****************************/

START_TEST(test_input_compact_resets_fully_consumed_buffer)
{
	w_entity_id e = create_input_stream_entity(128);
	request_input_hot(e);

	uint8_t *buffer = wm_managed_alloc_resolve_handle(&g_world, stream_input_buffer_handle, e);
	memcpy(buffer, "abcd", 4);
	stream_input_buffer_offset_set_value(&g_world, e, 4);
	stream_input_buffer_length_set_value(&g_world, e, 4);
	stream_input_available_set_tag_state(&g_world, e, true);

	run_phase(WM_STREAM_PHASE_INPUT_POST_CONSUME);

	ck_assert_uint_eq(*stream_input_buffer_offset_get(&g_world, e), 0);
	ck_assert_uint_eq(*stream_input_buffer_length_get(&g_world, e), 0);
	ck_assert(!stream_input_available_tag_exists(&g_world, e));
}
END_TEST

START_TEST(test_input_compact_moves_remaining_bytes)
{
	w_entity_id e = create_input_stream_entity(128);
	request_input_hot(e);

	uint8_t *buffer = wm_managed_alloc_resolve_handle(&g_world, stream_input_buffer_handle, e);
	memcpy(buffer, "abcdef", 6);
	stream_input_buffer_offset_set_value(&g_world, e, 2);
	stream_input_buffer_length_set_value(&g_world, e, 6);
	stream_input_available_set_tag_state(&g_world, e, true);

	run_phase(WM_STREAM_PHASE_INPUT_POST_CONSUME);

	buffer = wm_managed_alloc_resolve_handle(&g_world, stream_input_buffer_handle, e);
	ck_assert_uint_eq(*stream_input_buffer_offset_get(&g_world, e), 0);
	ck_assert_uint_eq(*stream_input_buffer_length_get(&g_world, e), 4);
	ck_assert_int_eq(memcmp(buffer, "cdef", 4), 0);
	ck_assert(stream_input_available_tag_exists(&g_world, e));
}
END_TEST

START_TEST(test_input_compact_clamps_offset_past_length)
{
	w_entity_id e = create_input_stream_entity(128);
	request_input_hot(e);

	stream_input_buffer_offset_set_value(&g_world, e, 20);
	stream_input_buffer_length_set_value(&g_world, e, 6);
	stream_input_available_set_tag_state(&g_world, e, true);

	run_phase(WM_STREAM_PHASE_INPUT_POST_CONSUME);

	ck_assert_uint_eq(*stream_input_buffer_offset_get(&g_world, e), 0);
	ck_assert_uint_eq(*stream_input_buffer_length_get(&g_world, e), 0);
	ck_assert(!stream_input_available_tag_exists(&g_world, e));
}
END_TEST


/*****************************
*  output compact            *
*****************************/

START_TEST(test_output_compact_resets_fully_consumed_buffer)
{
	w_entity_id e = create_output_stream_entity(128);
	request_output_hot(e);

	uint8_t *buffer = wm_managed_alloc_resolve_handle(&g_world, stream_output_buffer_handle, e);
	memcpy(buffer, "abcd", 4);
	stream_output_buffer_offset_set_value(&g_world, e, 4);
	stream_output_buffer_length_set_value(&g_world, e, 4);
	stream_output_available_set_tag_state(&g_world, e, true);

	run_phase(WM_STREAM_PHASE_OUTPUT_POST_CONSUME);

	ck_assert_uint_eq(*stream_output_buffer_offset_get(&g_world, e), 0);
	ck_assert_uint_eq(*stream_output_buffer_length_get(&g_world, e), 0);
	ck_assert(!stream_output_available_tag_exists(&g_world, e));
}
END_TEST

START_TEST(test_output_compact_moves_remaining_bytes)
{
	w_entity_id e = create_output_stream_entity(128);
	request_output_hot(e);

	uint8_t *buffer = wm_managed_alloc_resolve_handle(&g_world, stream_output_buffer_handle, e);
	memcpy(buffer, "abcdef", 6);
	stream_output_buffer_offset_set_value(&g_world, e, 2);
	stream_output_buffer_length_set_value(&g_world, e, 6);
	stream_output_available_set_tag_state(&g_world, e, true);

	run_phase(WM_STREAM_PHASE_OUTPUT_POST_CONSUME);

	buffer = wm_managed_alloc_resolve_handle(&g_world, stream_output_buffer_handle, e);
	ck_assert_uint_eq(*stream_output_buffer_offset_get(&g_world, e), 0);
	ck_assert_uint_eq(*stream_output_buffer_length_get(&g_world, e), 4);
	ck_assert_int_eq(memcmp(buffer, "cdef", 4), 0);
	ck_assert(stream_output_available_tag_exists(&g_world, e));
}
END_TEST

START_TEST(test_output_compact_clamps_offset_past_length)
{
	w_entity_id e = create_output_stream_entity(128);
	request_output_hot(e);

	stream_output_buffer_offset_set_value(&g_world, e, 20);
	stream_output_buffer_length_set_value(&g_world, e, 6);
	stream_output_available_set_tag_state(&g_world, e, true);

	run_phase(WM_STREAM_PHASE_OUTPUT_POST_CONSUME);

	ck_assert_uint_eq(*stream_output_buffer_offset_get(&g_world, e), 0);
	ck_assert_uint_eq(*stream_output_buffer_length_get(&g_world, e), 0);
	ck_assert(!stream_output_available_tag_exists(&g_world, e));
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *streams_suite(void)
{
	Suite *s = suite_create("streams");

	TCase *tc_input_lifecycle = tcase_create("input_lifecycle");
	tcase_add_checked_fixture(tc_input_lifecycle, streams_setup, streams_teardown);
	tcase_add_test(tc_input_lifecycle, test_input_hot_allocates_buffer);
	tcase_add_test(tc_input_lifecycle, test_input_hot_uses_requested_size);
	tcase_add_test(tc_input_lifecycle, test_input_cold_frees_and_removes_buffer);
	suite_add_tcase(s, tc_input_lifecycle);

	TCase *tc_output_lifecycle = tcase_create("output_lifecycle");
	tcase_add_checked_fixture(tc_output_lifecycle, streams_setup, streams_teardown);
	tcase_add_test(tc_output_lifecycle, test_output_hot_allocates_buffer);
	tcase_add_test(tc_output_lifecycle, test_output_hot_uses_requested_size);
	tcase_add_test(tc_output_lifecycle, test_output_cold_frees_and_removes_buffer);
	suite_add_tcase(s, tc_output_lifecycle);

	TCase *tc_input_compact = tcase_create("input_compact");
	tcase_add_checked_fixture(tc_input_compact, streams_setup, streams_teardown);
	tcase_add_test(tc_input_compact, test_input_compact_resets_fully_consumed_buffer);
	tcase_add_test(tc_input_compact, test_input_compact_moves_remaining_bytes);
	tcase_add_test(tc_input_compact, test_input_compact_clamps_offset_past_length);
	suite_add_tcase(s, tc_input_compact);

	TCase *tc_output_compact = tcase_create("output_compact");
	tcase_add_checked_fixture(tc_output_compact, streams_setup, streams_teardown);
	tcase_add_test(tc_output_compact, test_output_compact_resets_fully_consumed_buffer);
	tcase_add_test(tc_output_compact, test_output_compact_moves_remaining_bytes);
	tcase_add_test(tc_output_compact, test_output_compact_clamps_offset_past_length);
	suite_add_tcase(s, tc_output_compact);

	return s;
}

int main(void)
{
	Suite *s = streams_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
