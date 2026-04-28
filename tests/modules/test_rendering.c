/**
 * @author      : ElGatoPanzon
 * @file        : test_rendering
 * @created     : Saturday Apr 05, 2026 20:38:00 CST
 * @description : Tests for rendering module phase ordering with scheduler_defaults
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/rendering/whisker_rendering.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;
static struct w_rendering_display_config g_display_config;
static struct w_rendering_render_config g_render_config;

static void rendering_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_scheduler_defaults_init(&g_world, 60.0);
	memset(&g_display_config, 0, sizeof(g_display_config));
	memset(&g_render_config, 0, sizeof(g_render_config));
	wm_rendering_init(&g_world, &g_display_config, &g_render_config);
}

static void rendering_teardown(void)
{
	wm_rendering_free(&g_world);
	wm_scheduler_defaults_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  phase count               *
*****************************/

START_TEST(test_rendering_phase_count)
{
	// 22 from scheduler_defaults + 12 from rendering module
	ck_assert_int_eq(g_world.scheduler.phases_order_length, 34);
}
END_TEST


/*****************************
*  render phase IDs valid    *
*****************************/

START_TEST(test_rendering_phase_ids_valid)
{
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_PRE_SCALE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_ON_SCALE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_POST_SCALE));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_PRE_FILTER));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_ON_FILTER));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_POST_FILTER));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_PRE_DRAW));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_ON_DRAW));
	ck_assert_ptr_nonnull(w_ecs_get_system_phase(&g_world, WM_RENDER_PHASE_POST_DRAW));
}
END_TEST


/*****************************
*  logic phase ordering      *
*****************************/

START_TEST(test_phase_ordering_logic)
{
	// PRE_UPDATE < ON_UPDATE < POST_UPDATE across phases_order
	size_t *order = g_world.scheduler.phases_order;
	size_t len = g_world.scheduler.phases_order_length;

	size_t pos_pre = 0, pos_on = 0, pos_post = 0;

	for (size_t i = 0; i < len; i++) {
		if (order[i] == WM_PHASE_PRE_UPDATE)  pos_pre = i;
		if (order[i] == WM_PHASE_ON_UPDATE)   pos_on = i;
		if (order[i] == WM_PHASE_POST_UPDATE) pos_post = i;
	}

	ck_assert_uint_lt(pos_pre, pos_on);
	ck_assert_uint_lt(pos_on, pos_post);
}
END_TEST


/*****************************
*  render phase ordering     *
*****************************/

START_TEST(test_phase_ordering_render)
{
	// PRE_RENDER < ON_RENDER < POST_RENDER within phases_order
	size_t *order = g_world.scheduler.phases_order;
	size_t len = g_world.scheduler.phases_order_length;

	size_t pos_pre = 0, pos_on = 0, pos_post = 0;

	for (size_t i = 0; i < len; i++) {
		if (order[i] == WM_PHASE_PRE_RENDER)  pos_pre = i;
		if (order[i] == WM_PHASE_ON_RENDER)   pos_on = i;
		if (order[i] == WM_PHASE_POST_RENDER) pos_post = i;
	}

	ck_assert_uint_lt(pos_pre, pos_on);
	ck_assert_uint_lt(pos_on, pos_post);
}
END_TEST


/************************************
*  rendering module phase ordering  *
************************************/

START_TEST(test_phase_ordering_rendering_module)
{
	// FINAL_RENDER < PRE_SCALE < ON_SCALE < POST_SCALE < PRE_FILTER < ON_FILTER
	// < POST_FILTER < PRE_DRAW < ON_DRAW < POST_DRAW
	// (rendering module phases now come after FINAL_RENDER in execution order)
	size_t *order = g_world.scheduler.phases_order;
	size_t len = g_world.scheduler.phases_order_length;

	size_t pos_final_render = 0;
	size_t pos_pre_scale    = 0;
	size_t pos_on_scale     = 0;
	size_t pos_post_scale   = 0;
	size_t pos_pre_filter   = 0;
	size_t pos_on_filter    = 0;
	size_t pos_post_filter  = 0;
	size_t pos_pre_draw     = 0;
	size_t pos_on_draw      = 0;
	size_t pos_post_draw    = 0;

	for (size_t i = 0; i < len; i++) {
		if (order[i] == WM_PHASE_FINAL_RENDER)         pos_final_render = i;
		if (order[i] == WM_RENDER_PHASE_PRE_SCALE)     pos_pre_scale    = i;
		if (order[i] == WM_RENDER_PHASE_ON_SCALE)      pos_on_scale     = i;
		if (order[i] == WM_RENDER_PHASE_POST_SCALE)    pos_post_scale   = i;
		if (order[i] == WM_RENDER_PHASE_PRE_FILTER)    pos_pre_filter   = i;
		if (order[i] == WM_RENDER_PHASE_ON_FILTER)     pos_on_filter    = i;
		if (order[i] == WM_RENDER_PHASE_POST_FILTER)   pos_post_filter  = i;
		if (order[i] == WM_RENDER_PHASE_PRE_DRAW)      pos_pre_draw     = i;
		if (order[i] == WM_RENDER_PHASE_ON_DRAW)       pos_on_draw      = i;
		if (order[i] == WM_RENDER_PHASE_POST_DRAW)     pos_post_draw    = i;
	}

	ck_assert_uint_lt(pos_final_render, pos_pre_scale);
	ck_assert_uint_lt(pos_pre_scale,    pos_on_scale);
	ck_assert_uint_lt(pos_on_scale,     pos_post_scale);
	ck_assert_uint_lt(pos_post_scale,   pos_pre_filter);
	ck_assert_uint_lt(pos_pre_filter,   pos_on_filter);
	ck_assert_uint_lt(pos_on_filter,    pos_post_filter);
	ck_assert_uint_lt(pos_post_filter,  pos_pre_draw);
	ck_assert_uint_lt(pos_pre_draw,     pos_on_draw);
	ck_assert_uint_lt(pos_on_draw,      pos_post_draw);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *rendering_suite(void)
{
	Suite *s = suite_create("rendering");

	TCase *tc_counts = tcase_create("counts");
	tcase_add_checked_fixture(tc_counts, rendering_setup, rendering_teardown);
	tcase_set_timeout(tc_counts, 10);
	tcase_add_test(tc_counts, test_rendering_phase_count);
	suite_add_tcase(s, tc_counts);

	TCase *tc_ids = tcase_create("ids_valid");
	tcase_add_checked_fixture(tc_ids, rendering_setup, rendering_teardown);
	tcase_set_timeout(tc_ids, 10);
	tcase_add_test(tc_ids, test_rendering_phase_ids_valid);
	suite_add_tcase(s, tc_ids);

	TCase *tc_order = tcase_create("phase_ordering");
	tcase_add_checked_fixture(tc_order, rendering_setup, rendering_teardown);
	tcase_set_timeout(tc_order, 10);
	tcase_add_test(tc_order, test_phase_ordering_logic);
	tcase_add_test(tc_order, test_phase_ordering_render);
	tcase_add_test(tc_order, test_phase_ordering_rendering_module);
	suite_add_tcase(s, tc_order);

	return s;
}

int main(void)
{
	Suite *s = rendering_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
