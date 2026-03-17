/**
 * @author      : ElGatoPanzon
 * @file        : test_serialisation
 * @created     : Sunday Mar 15, 2026 21:24:35 CST
 * @description : Tests for whisker_serialisation module
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "whisker_serialisation.h"

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

static void serialisation_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_serialisation_init(&g_world);
}

static void serialisation_teardown(void)
{
	wm_serialisation_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  smoke tests               *
*****************************/

START_TEST(test_init_free_no_crash)
{
	/* wm_serialisation_init and free ran in fixture without crash */
	ck_assert(true);
}
END_TEST


/*****************************
*  persistent entity tests   *
*****************************/

START_TEST(test_persistent_entities_includes_named)
{
	// create named entities
	w_entity_id e1 = w_ecs_request_entity_with_name(&g_world, "entity_one");
	w_entity_id e2 = w_ecs_request_entity_with_name(&g_world, "entity_two");

	_Atomic size_t len = 0;
	_Atomic size_t size = 0;
	w_entity_id *list = w_serialisation_get_persistent_entity_list(&g_world, &len, &size);

	ck_assert_uint_ge(len, 2);

	// check both named entities are in the list
	bool found_e1 = false;
	bool found_e2 = false;
	for (size_t i = 0; i < len; ++i) {
		if (list[i] == e1) found_e1 = true;
		if (list[i] == e2) found_e2 = true;
	}
	ck_assert(found_e1);
	ck_assert(found_e2);

	free(list);
}
END_TEST

START_TEST(test_persistent_entities_excludes_unnamed)
{
	// create an unnamed entity
	w_entity_id anon = w_ecs_request_entity(&g_world);

	_Atomic size_t len = 0;
	_Atomic size_t size = 0;
	w_entity_id *list = w_serialisation_get_persistent_entity_list(&g_world, &len, &size);

	// anon entity should NOT be in the list
	bool found_anon = false;
	for (size_t i = 0; i < len; ++i) {
		if (list[i] == anon) found_anon = true;
	}
	ck_assert(!found_anon);

	free(list);
}
END_TEST

START_TEST(test_persistent_entities_mixed)
{
	// BUG: entity_to_name array is zero-initialized, but W_STRING_TABLE_INVALID_ID
	// is -1 (UINT32_MAX). When a named entity extends the array, slots for
	// anonymous entities at lower IDs become 0, which != -1, so they incorrectly
	// pass the validity check. This test documents expected behavior but will
	// fail until the bug is fixed.
	w_entity_id anon1 = w_ecs_request_entity(&g_world);
	w_entity_id named = w_ecs_request_entity_with_name(&g_world, "named_entity");
	w_entity_id anon2 = w_ecs_request_entity(&g_world);

	_Atomic size_t len = 0;
	_Atomic size_t size = 0;
	w_entity_id *list = w_serialisation_get_persistent_entity_list(&g_world, &len, &size);

	bool found_anon1 = false;
	bool found_named = false;
	bool found_anon2 = false;
	for (size_t i = 0; i < len; ++i) {
		if (list[i] == anon1) found_anon1 = true;
		if (list[i] == named) found_named = true;
		if (list[i] == anon2) found_anon2 = true;
	}

	// named should be found, anonymous should not
	ck_assert(found_named);
	ck_assert(!found_anon1);
	ck_assert(!found_anon2);

	free(list);
}
END_TEST

START_TEST(test_persistent_entities_empty_world)
{
	// no entities created, list should be empty
	_Atomic size_t len = 0;
	_Atomic size_t size = 0;
	w_entity_id *list = w_serialisation_get_persistent_entity_list(&g_world, &len, &size);

	ck_assert_uint_eq(len, 0);
	ck_assert_ptr_nonnull(list);

	free(list);
}
END_TEST


/*****************************
*  components list tests     *
*****************************/

START_TEST(test_components_list_empty_world)
{
	// no components registered, list should be empty
	_Atomic size_t len = 0;
	_Atomic size_t size = 0;
	w_entity_id *list = w_serialisation_get_components_list(&g_world, &len, &size);

	ck_assert_uint_eq(len, 0);
	ck_assert_ptr_nonnull(list);

	free(list);
}
END_TEST

START_TEST(test_components_list_single_component)
{
	// register one component type
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "test_component_a");
	int val = 42;
	w_ecs_set_component_(&g_world, 0, comp_id, entity, &val, sizeof(val));

	_Atomic size_t len = 0;
	_Atomic size_t size = 0;
	w_entity_id *list = w_serialisation_get_components_list(&g_world, &len, &size);

	ck_assert_uint_ge(len, 1);

	// find our component in the list
	bool found = false;
	for (size_t i = 0; i < len; ++i) {
		if (list[i] == comp_id) found = true;
	}
	ck_assert(found);

	free(list);
}
END_TEST

START_TEST(test_components_list_multiple_components)
{
	// register multiple component types
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp_a = w_ecs_get_component_by_name(&g_world, "comp_alpha");
	w_entity_id comp_b = w_ecs_get_component_by_name(&g_world, "comp_beta");
	w_entity_id comp_c = w_ecs_get_component_by_name(&g_world, "comp_gamma");

	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp_a, entity, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_b, entity, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_c, entity, &val, sizeof(val));

	_Atomic size_t len = 0;
	_Atomic size_t size = 0;
	w_entity_id *list = w_serialisation_get_components_list(&g_world, &len, &size);

	ck_assert_uint_ge(len, 3);

	// all three should be present
	bool found_a = false, found_b = false, found_c = false;
	for (size_t i = 0; i < len; ++i) {
		if (list[i] == comp_a) found_a = true;
		if (list[i] == comp_b) found_b = true;
		if (list[i] == comp_c) found_c = true;
	}
	ck_assert(found_a);
	ck_assert(found_b);
	ck_assert(found_c);

	free(list);
}
END_TEST

START_TEST(test_components_list_returns_unique_ids)
{
	// register same component on multiple entities
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "shared_comp");

	int val = 10;
	w_ecs_set_component_(&g_world, 0, comp_id, e1, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_id, e2, &val, sizeof(val));

	_Atomic size_t len = 0;
	_Atomic size_t size = 0;
	w_entity_id *list = w_serialisation_get_components_list(&g_world, &len, &size);

	// component type should appear only once even if attached to multiple entities
	int count = 0;
	for (size_t i = 0; i < len; ++i) {
		if (list[i] == comp_id) count++;
	}
	ck_assert_int_eq(count, 1);

	free(list);
}
END_TEST


/*****************************
*  dump_to_buffer tests      *
*****************************/

START_TEST(test_dump_to_buffer_returns_true_on_success)
{
	struct wm_serialisation_ctx ctx = {0};
	bool result = w_serialisation_dump_to_buffer(&g_world, &ctx);
	ck_assert(result);

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_to_buffer_contains_whisker_save_header)
{
	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	ck_assert_ptr_nonnull(strstr(ctx.buffer, "# whisker save"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_to_buffer_contains_version_line)
{
	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	ck_assert_ptr_nonnull(strstr(ctx.buffer, "# version"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_to_buffer_contains_created_timestamp)
{
	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	ck_assert_ptr_nonnull(strstr(ctx.buffer, "# created"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_to_buffer_entities_count_matches)
{
	w_ecs_request_entity_with_name(&g_world, "test_a");
	w_ecs_request_entity_with_name(&g_world, "test_b");

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// should have "# entities 2" in buffer
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "# entities 2"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_to_buffer_components_count_matches)
{
	w_entity_id entity = w_ecs_request_entity(&g_world);
	w_entity_id comp_a = w_ecs_get_component_by_name(&g_world, "comp_x");
	w_entity_id comp_b = w_ecs_get_component_by_name(&g_world, "comp_y");

	int val = 1;
	w_ecs_set_component_(&g_world, 0, comp_a, entity, &val, sizeof(val));
	w_ecs_set_component_(&g_world, 0, comp_b, entity, &val, sizeof(val));

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// should have "# components 2" in buffer
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "# components 2"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_to_buffer_entity_commands_written)
{
	w_ecs_request_entity_with_name(&g_world, "hero");
	w_ecs_request_entity_with_name(&g_world, "villain");

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	ck_assert_ptr_nonnull(strstr(ctx.buffer, "entity \"hero\""));
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "entity \"villain\""));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_to_buffer_entities_saved_matches_count)
{
	w_ecs_request_entity_with_name(&g_world, "ent_one");
	w_ecs_request_entity_with_name(&g_world, "ent_two");
	w_ecs_request_entity_with_name(&g_world, "ent_three");

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	ck_assert_uint_eq(ctx.entities_saved, 3);

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST


/*****************************
*  component set cmd tests   *
*****************************/

// simple serialize hook that writes known params
static void test_serialize_hook_int(void *ctx, void *data)
{
	(void)ctx;
	struct wm_serialisation_component_ctx *comp_ctx = data;
	snprintf(comp_ctx->hook_params_buffer, sizeof(comp_ctx->hook_params_buffer), "42");
}

// serialize hook that writes multiple params
static void test_serialize_hook_multi(void *ctx, void *data)
{
	(void)ctx;
	struct wm_serialisation_component_ctx *comp_ctx = data;
	snprintf(comp_ctx->hook_params_buffer, sizeof(comp_ctx->hook_params_buffer), "1 2 3");
}

// serialize hook that writes empty string
static void test_serialize_hook_empty(void *ctx, void *data)
{
	(void)ctx;
	struct wm_serialisation_component_ctx *comp_ctx = data;
	comp_ctx->hook_params_buffer[0] = '\0';
}

START_TEST(test_dump_component_set_command_written)
{
	// create named entity with component
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "player");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "health");

	int val = 100;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int32_t, comp_id, entity, &val, sizeof(val));

	// register serialize hook
	w_serialisation_register_component_hooks(&g_world, W_COMPONENT_TYPE_int32_t, test_serialize_hook_int, NULL);

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// buffer should contain a set command for this entity/component
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "set \"player\""));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_component_type_name_in_set_command)
{
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "npc");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "score");

	int val = 50;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int32_t, comp_id, entity, &val, sizeof(val));

	w_serialisation_register_component_hooks(&g_world, W_COMPONENT_TYPE_int32_t, test_serialize_hook_int, NULL);

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// set command includes unquoted type name: set "ent" "comp" type_name params
	ck_assert_ptr_nonnull(strstr(ctx.buffer, " int32_t "));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_entity_name_in_set_command)
{
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "boss_enemy");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "damage");

	int val = 200;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int32_t, comp_id, entity, &val, sizeof(val));

	w_serialisation_register_component_hooks(&g_world, W_COMPONENT_TYPE_int32_t, test_serialize_hook_int, NULL);

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// verify entity name appears in set command
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "\"boss_enemy\""));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_hook_params_in_set_command)
{
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "item");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "quantity");

	int val = 5;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int32_t, comp_id, entity, &val, sizeof(val));

	w_serialisation_register_component_hooks(&g_world, W_COMPONENT_TYPE_int32_t, test_serialize_hook_int, NULL);

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// hook writes "42" to params buffer
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "42"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_multiple_components_multiple_set_commands)
{
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "warrior");
	w_entity_id comp_a = w_ecs_get_component_by_name(&g_world, "strength");
	w_entity_id comp_b = w_ecs_get_component_by_name(&g_world, "agility");

	int val_a = 10;
	int val_b = 15;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int32_t, comp_a, entity, &val_a, sizeof(val_a));
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int32_t, comp_b, entity, &val_b, sizeof(val_b));

	w_serialisation_register_component_hooks(&g_world, W_COMPONENT_TYPE_int32_t, test_serialize_hook_int, NULL);

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// count occurrences of set command for warrior
	int count = 0;
	char *pos = ctx.buffer;
	while ((pos = strstr(pos, "set \"warrior\"")) != NULL) {
		count++;
		pos++;
	}
	ck_assert_int_eq(count, 2);

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_empty_params_no_crash)
{
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "empty_test");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "empty_comp");

	int val = 0;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int32_t, comp_id, entity, &val, sizeof(val));

	// register hook that produces empty params
	w_serialisation_register_component_hooks(&g_world, W_COMPONENT_TYPE_int32_t, test_serialize_hook_empty, NULL);

	struct wm_serialisation_ctx ctx = {0};
	bool result = w_serialisation_dump_to_buffer(&g_world, &ctx);

	// should succeed without crash
	ck_assert(result);
	ck_assert_ptr_nonnull(ctx.buffer);
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "set \"empty_test\""));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_dump_component_no_hook_still_writes_set)
{
	// component with no serialize hook registered should still write set command
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "no_hook_ent");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "no_hook_comp");

	float val = 3.14f;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_float, comp_id, entity, &val, sizeof(val));

	// no hook registered for float type

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// set command should still be written even without hook
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "set \"no_hook_ent\""));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST


/*****************************
*  types - bool tests        *
*****************************/

START_TEST(test_bool_true_serialises_as_true)
{
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "bool_true_ent");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "flag_enabled");

	bool val = true;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_bool, comp_id, entity, &val, sizeof(val));

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// should contain "true" for the bool value
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "true"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_bool_false_serialises_as_false)
{
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "bool_false_ent");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "flag_disabled");

	bool val = false;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_bool, comp_id, entity, &val, sizeof(val));

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// should contain "false" for the bool value
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "false"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST

START_TEST(test_bool_set_command_format)
{
	w_entity_id entity = w_ecs_request_entity_with_name(&g_world, "format_test_ent");
	w_entity_id comp_id = w_ecs_get_component_by_name(&g_world, "is_active");

	bool val = true;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_bool, comp_id, entity, &val, sizeof(val));

	struct wm_serialisation_ctx ctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &ctx);

	// verify set command format: set "entityname" "compname" bool true/false
	ck_assert_ptr_nonnull(strstr(ctx.buffer, "set \"format_test_ent\" \"is_active\" bool true"));

	free(ctx.buffer);
	free(ctx.entities);
	free(ctx.components);
}
END_TEST


/*****************************
*  deserialisation tests     *
*****************************/

// helper: make a mutable copy of a string literal for restore_from_buffer
static char *make_buffer(const char *str)
{
	size_t len = strlen(str);
	char *buf = malloc(len + 1);
	memcpy(buf, str, len + 1);
	return buf;
}

START_TEST(test_restore_returns_true_on_valid_buffer)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# created 2026-03-15 21:30:00\n"
		"# version 1\n"
		"# entities 0\n"
		"# components 0\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);
	ck_assert_int_eq(dctx.err, 0);
	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_fails_without_header)
{
	char *buf = make_buffer(
		"# version 1\n"
		"# entities 0\n"
		"# components 0\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(!result);
	ck_assert_int_ne(dctx.err, 0);
	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_parses_version)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 42\n"
		"# entities 0\n"
		"# components 0\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert_uint_eq(dctx.version, 42);
	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_creates_entities)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 2\n"
		"# components 0\n"
		"entity \"alpha\"\n"
		"entity \"beta\"\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);

	ck_assert_uint_eq(dctx.entities_loaded, 2);

	w_entity_id a = w_ecs_get_entity_by_name(&g_world, "alpha");
	w_entity_id b = w_ecs_get_entity_by_name(&g_world, "beta");
	ck_assert_uint_ne(a, W_ENTITY_INVALID);
	ck_assert_uint_ne(b, W_ENTITY_INVALID);

	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_entity_malformed_quote)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"entity \"broken\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(!result);
	ck_assert_int_ne(dctx.err, 0);
	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_set_bool_true)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 1\n"
		"# components 1\n"
		"entity \"flag_ent\"\n"
		"set \"flag_ent\" \"is_on\" bool true\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);
	ck_assert_uint_eq(dctx.components_loaded, 1);

	w_entity_id eid = w_ecs_get_entity_by_name(&g_world, "flag_ent");
	w_entity_id cid = w_ecs_get_component_by_name(&g_world, "is_on");
	bool *val = w_ecs_get_component_(&g_world, cid, eid);
	ck_assert_ptr_nonnull(val);
	ck_assert(*val == true);

	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_set_bool_false)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 1\n"
		"# components 1\n"
		"entity \"flag_ent2\"\n"
		"set \"flag_ent2\" \"is_off\" bool false\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);

	w_entity_id eid = w_ecs_get_entity_by_name(&g_world, "flag_ent2");
	w_entity_id cid = w_ecs_get_component_by_name(&g_world, "is_off");
	bool *val = w_ecs_get_component_(&g_world, cid, eid);
	ck_assert_ptr_nonnull(val);
	ck_assert(*val == false);

	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_set_unknown_entity_fails)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 0\n"
		"# components 0\n"
		"set \"ghost\" \"comp\" bool true\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(!result);
	ck_assert_int_ne(dctx.err, 0);
	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_set_unknown_type_fails)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 1\n"
		"# components 0\n"
		"entity \"ent\"\n"
		"set \"ent\" \"comp\" fake_type_xyz 123\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(!result);
	ck_assert_int_ne(dctx.err, 0);
	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_unparsed_lines_collected)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"some_unknown_command foo\n"
		"# entities 0\n"
		"# components 0\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert_uint_eq(dctx.unparsed_length, 1);
	ck_assert_str_eq(dctx.unparsed[0], "some_unknown_command foo");
	free(dctx.unparsed[0]);
	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_restore_multiple_entities_and_components)
{
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 2\n"
		"# components 2\n"
		"entity \"e1\"\n"
		"entity \"e2\"\n"
		"set \"e1\" \"flag_a\" bool true\n"
		"set \"e2\" \"flag_b\" bool false\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);
	ck_assert_uint_eq(dctx.entities_loaded, 2);
	ck_assert_uint_eq(dctx.components_loaded, 2);

	w_entity_id e1 = w_ecs_get_entity_by_name(&g_world, "e1");
	w_entity_id e2 = w_ecs_get_entity_by_name(&g_world, "e2");
	w_entity_id ca = w_ecs_get_component_by_name(&g_world, "flag_a");
	w_entity_id cb = w_ecs_get_component_by_name(&g_world, "flag_b");

	bool *va = w_ecs_get_component_(&g_world, ca, e1);
	bool *vb = w_ecs_get_component_(&g_world, cb, e2);
	ck_assert(*va == true);
	ck_assert(*vb == false);

	free(dctx.unparsed);
	free(buf);
}
END_TEST


/*************************************
*  end-to-end serialize/deserialize  *
*************************************/

START_TEST(test_e2e_serialize_deserialize_bool)
{
	// serialize: create world with entities and bool components
	w_entity_id hero = w_ecs_request_entity_with_name(&g_world, "hero");
	w_entity_id villain = w_ecs_request_entity_with_name(&g_world, "villain");

	w_entity_id alive_comp = w_ecs_get_component_by_name(&g_world, "alive");
	w_entity_id visible_comp = w_ecs_get_component_by_name(&g_world, "visible");

	bool t = true;
	bool f = false;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_bool, alive_comp, hero, &t, sizeof(bool));
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_bool, visible_comp, hero, &t, sizeof(bool));
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_bool, alive_comp, villain, &f, sizeof(bool));
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_bool, visible_comp, villain, &t, sizeof(bool));

	// dump to buffer
	struct wm_serialisation_ctx sctx = {0};
	bool dump_ok = w_serialisation_dump_to_buffer(&g_world, &sctx);
	ck_assert(dump_ok);
	// entities_saved includes component type entities (alive, visible) which also get names
	ck_assert_uint_ge(sctx.entities_saved, 2);

	// create a fresh world for deserialization
	struct w_arena arena2 = {0};
	struct w_string_table st2 = {0};
	struct w_ecs_world world2 = {0};
	w_arena_init(&arena2, 4096);
	w_string_table_init(&st2, &arena2, 16, 64, NULL);
	w_ecs_world_init(&world2, &st2, &arena2);
	wm_serialisation_init(&world2);

	// restore from buffer into fresh world
	struct wm_deserialisation_ctx dctx = {0};
	bool restore_ok = w_serialisation_restore_from_buffer(&world2, sctx.buffer, sctx.buffer_length, &dctx);
	ck_assert(restore_ok);
	ck_assert_uint_ge(dctx.entities_loaded, 2);
	ck_assert_uint_ge(dctx.components_loaded, 4);

	// verify entities exist in new world
	w_entity_id hero2 = w_ecs_get_entity_by_name(&world2, "hero");
	w_entity_id villain2 = w_ecs_get_entity_by_name(&world2, "villain");
	ck_assert_uint_ne(hero2, W_ENTITY_INVALID);
	ck_assert_uint_ne(villain2, W_ENTITY_INVALID);

	// verify component data by name lookup
	w_entity_id alive2 = w_ecs_get_component_by_name(&world2, "alive");
	w_entity_id visible2 = w_ecs_get_component_by_name(&world2, "visible");

	bool *hero_alive = w_ecs_get_component_(&world2, alive2, hero2);
	bool *hero_visible = w_ecs_get_component_(&world2, visible2, hero2);
	bool *villain_alive = w_ecs_get_component_(&world2, alive2, villain2);
	bool *villain_visible = w_ecs_get_component_(&world2, visible2, villain2);

	ck_assert_ptr_nonnull(hero_alive);
	ck_assert_ptr_nonnull(hero_visible);
	ck_assert_ptr_nonnull(villain_alive);
	ck_assert_ptr_nonnull(villain_visible);

	ck_assert(*hero_alive == true);
	ck_assert(*hero_visible == true);
	ck_assert(*villain_alive == false);
	ck_assert(*villain_visible == true);

	// cleanup
	free(dctx.unparsed);
	free(sctx.buffer);
	free(sctx.entities);
	free(sctx.components);
	wm_serialisation_free(&world2);
	w_ecs_world_free(&world2);
	w_string_table_free(&st2);
	w_arena_free(&arena2);
}
END_TEST

START_TEST(test_e2e_entity_ids_may_differ)
{
	// serialize world with entities
	w_ecs_request_entity_with_name(&g_world, "obj_a");
	w_ecs_request_entity_with_name(&g_world, "obj_b");

	struct wm_serialisation_ctx sctx = {0};
	w_serialisation_dump_to_buffer(&g_world, &sctx);

	// create fresh world with some pre-existing entities to shift IDs
	struct w_arena arena2 = {0};
	struct w_string_table st2 = {0};
	struct w_ecs_world world2 = {0};
	w_arena_init(&arena2, 4096);
	w_string_table_init(&st2, &arena2, 16, 64, NULL);
	w_ecs_world_init(&world2, &st2, &arena2);
	wm_serialisation_init(&world2);

	// create some entities first to shift IDs
	w_ecs_request_entity(&world2);
	w_ecs_request_entity(&world2);
	w_ecs_request_entity(&world2);

	struct wm_deserialisation_ctx dctx = {0};
	w_serialisation_restore_from_buffer(&world2, sctx.buffer, sctx.buffer_length, &dctx);

	// entities exist by name even though IDs differ
	w_entity_id a2 = w_ecs_get_entity_by_name(&world2, "obj_a");
	w_entity_id b2 = w_ecs_get_entity_by_name(&world2, "obj_b");
	ck_assert_uint_ne(a2, W_ENTITY_INVALID);
	ck_assert_uint_ne(b2, W_ENTITY_INVALID);

	free(dctx.unparsed);
	free(sctx.buffer);
	free(sctx.entities);
	free(sctx.components);
	wm_serialisation_free(&world2);
	w_ecs_world_free(&world2);
	w_string_table_free(&st2);
	w_arena_free(&arena2);
}
END_TEST


/*****************************
*  migration tests           *
*****************************/

// migration hooks set bool components on known entities to track which ran
static void migration_hook_set_migrated_v1(void *ctx, void *data)
{
	struct w_ecs_world *world = ctx;
	(void)data;

	w_entity_id ent = w_ecs_get_entity_by_name(world, "migrate_target");
	if (ent == W_ENTITY_INVALID) return;

	w_entity_id comp = w_ecs_get_component_by_name(world, "migrated_v1");
	bool val = true;
	w_ecs_set_component_(world, W_COMPONENT_TYPE_bool, comp, ent, &val, sizeof(val));
}

static void migration_hook_set_migrated_v2(void *ctx, void *data)
{
	struct w_ecs_world *world = ctx;
	(void)data;

	w_entity_id ent = w_ecs_get_entity_by_name(world, "migrate_target");
	if (ent == W_ENTITY_INVALID) return;

	w_entity_id comp = w_ecs_get_component_by_name(world, "migrated_v2");
	bool val = true;
	w_ecs_set_component_(world, W_COMPONENT_TYPE_bool, comp, ent, &val, sizeof(val));
}

static void migration_hook_set_migrated_v3(void *ctx, void *data)
{
	struct w_ecs_world *world = ctx;
	(void)data;

	w_entity_id ent = w_ecs_get_entity_by_name(world, "migrate_target");
	if (ent == W_ENTITY_INVALID) return;

	w_entity_id comp = w_ecs_get_component_by_name(world, "migrated_v3");
	bool val = true;
	w_ecs_set_component_(world, W_COMPONENT_TYPE_bool, comp, ent, &val, sizeof(val));
}

START_TEST(test_migration_hooks_run_on_version_0)
{
	// register 2 migration hooks
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v1);
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v2);

	// version 0 save -- no migrations applied yet
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 1\n"
		"# components 0\n"
		"entity \"migrate_target\"\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);

	// both migration hooks should have run
	w_entity_id ent = w_ecs_get_entity_by_name(&g_world, "migrate_target");
	w_entity_id c1 = w_ecs_get_component_by_name(&g_world, "migrated_v1");
	w_entity_id c2 = w_ecs_get_component_by_name(&g_world, "migrated_v2");

	bool *v1 = w_ecs_get_component_(&g_world, c1, ent);
	bool *v2 = w_ecs_get_component_(&g_world, c2, ent);
	ck_assert_ptr_nonnull(v1);
	ck_assert_ptr_nonnull(v2);
	ck_assert(*v1 == true);
	ck_assert(*v2 == true);

	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_migration_hooks_skipped_when_version_current)
{
	// register 2 migration hooks
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v1);
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v2);

	// version 2 save -- both migrations already applied
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 2\n"
		"# entities 1\n"
		"# components 0\n"
		"entity \"migrate_target\"\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);

	// neither hook should have run -- components should not exist on entity
	w_entity_id ent = w_ecs_get_entity_by_name(&g_world, "migrate_target");
	w_entity_id c1 = w_ecs_get_component_by_name(&g_world, "migrated_v1");
	w_entity_id c2 = w_ecs_get_component_by_name(&g_world, "migrated_v2");

	bool *v1 = w_ecs_get_component_(&g_world, c1, ent);
	bool *v2 = w_ecs_get_component_(&g_world, c2, ent);
	ck_assert_ptr_null(v1);
	ck_assert_ptr_null(v2);

	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_migration_hooks_partial_skip)
{
	// register 3 migration hooks (indices 0, 1, 2)
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v1);
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v2);
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v3);

	// version 1 save -- first migration already applied, run hooks 1 and 2
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 1\n"
		"# entities 1\n"
		"# components 0\n"
		"entity \"migrate_target\"\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);

	w_entity_id ent = w_ecs_get_entity_by_name(&g_world, "migrate_target");
	w_entity_id c1 = w_ecs_get_component_by_name(&g_world, "migrated_v1");
	w_entity_id c2 = w_ecs_get_component_by_name(&g_world, "migrated_v2");
	w_entity_id c3 = w_ecs_get_component_by_name(&g_world, "migrated_v3");

	// hook 0 (v1) should be skipped
	bool *v1 = w_ecs_get_component_(&g_world, c1, ent);
	ck_assert_ptr_null(v1);

	// hooks 1 (v2) and 2 (v3) should have run
	bool *v2 = w_ecs_get_component_(&g_world, c2, ent);
	bool *v3 = w_ecs_get_component_(&g_world, c3, ent);
	ck_assert_ptr_nonnull(v2);
	ck_assert_ptr_nonnull(v3);
	ck_assert(*v2 == true);
	ck_assert(*v3 == true);

	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_migration_hooks_version_exceeds_hook_count)
{
	// register 1 migration hook
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v1);

	// version 99 -- way beyond registered hook count
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 99\n"
		"# entities 1\n"
		"# components 0\n"
		"entity \"migrate_target\"\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);

	// hook should not have run
	w_entity_id ent = w_ecs_get_entity_by_name(&g_world, "migrate_target");
	w_entity_id c1 = w_ecs_get_component_by_name(&g_world, "migrated_v1");
	bool *v1 = w_ecs_get_component_(&g_world, c1, ent);
	ck_assert_ptr_null(v1);

	free(dctx.unparsed);
	free(buf);
}
END_TEST


/*****************************
*  post-load tests           *
*****************************/

// post-load hooks set components to track execution
static void post_load_hook_set_loaded_a(void *ctx, void *data)
{
	struct w_ecs_world *world = ctx;
	(void)data;

	w_entity_id ent = w_ecs_get_entity_by_name(world, "post_load_target");
	if (ent == W_ENTITY_INVALID) return;

	w_entity_id comp = w_ecs_get_component_by_name(world, "loaded_a");
	bool val = true;
	w_ecs_set_component_(world, W_COMPONENT_TYPE_bool, comp, ent, &val, sizeof(val));
}

static void post_load_hook_set_loaded_b(void *ctx, void *data)
{
	struct w_ecs_world *world = ctx;
	(void)data;

	w_entity_id ent = w_ecs_get_entity_by_name(world, "post_load_target");
	if (ent == W_ENTITY_INVALID) return;

	w_entity_id comp = w_ecs_get_component_by_name(world, "loaded_b");
	bool val = true;
	w_ecs_set_component_(world, W_COMPONENT_TYPE_bool, comp, ent, &val, sizeof(val));
}

START_TEST(test_post_load_hooks_all_run_on_version_0)
{
	// register 2 post-load hooks
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_POST_LOAD, post_load_hook_set_loaded_a);
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_POST_LOAD, post_load_hook_set_loaded_b);

	// version 0 save
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 1\n"
		"# components 0\n"
		"entity \"post_load_target\"\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);

	// both hooks should have run
	w_entity_id ent = w_ecs_get_entity_by_name(&g_world, "post_load_target");
	w_entity_id ca = w_ecs_get_component_by_name(&g_world, "loaded_a");
	w_entity_id cb = w_ecs_get_component_by_name(&g_world, "loaded_b");

	bool *va = w_ecs_get_component_(&g_world, ca, ent);
	bool *vb = w_ecs_get_component_(&g_world, cb, ent);
	ck_assert_ptr_nonnull(va);
	ck_assert_ptr_nonnull(vb);
	ck_assert(*va == true);
	ck_assert(*vb == true);

	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_post_load_hooks_all_run_regardless_of_version)
{
	// register 2 post-load hooks
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_POST_LOAD, post_load_hook_set_loaded_a);
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_POST_LOAD, post_load_hook_set_loaded_b);

	// version 99 save -- high version should NOT skip post-load hooks
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 99\n"
		"# entities 1\n"
		"# components 0\n"
		"entity \"post_load_target\"\n"
	);
	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);

	// both hooks should still run -- post-load hooks ignore version
	w_entity_id ent = w_ecs_get_entity_by_name(&g_world, "post_load_target");
	w_entity_id ca = w_ecs_get_component_by_name(&g_world, "loaded_a");
	w_entity_id cb = w_ecs_get_component_by_name(&g_world, "loaded_b");

	bool *va = w_ecs_get_component_(&g_world, ca, ent);
	bool *vb = w_ecs_get_component_(&g_world, cb, ent);
	ck_assert_ptr_nonnull(va);
	ck_assert_ptr_nonnull(vb);
	ck_assert(*va == true);
	ck_assert(*vb == true);

	free(dctx.unparsed);
	free(buf);
}
END_TEST

START_TEST(test_post_load_hooks_run_after_migration_hooks)
{
	// register migration hook that sets migrated_v1
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, migration_hook_set_migrated_v1);
	// register post-load hook
	w_serialisation_register_lifecycle_hook(&g_world, WM_SERIALISATION_LIFECYCLE_HOOK_POST_LOAD, post_load_hook_set_loaded_a);

	// version 0 -- migration should run, then post-load
	char *buf = make_buffer(
		"# whisker save\n"
		"# version 0\n"
		"# entities 1\n"
		"# components 0\n"
		"entity \"migrate_target\"\n"
	);

	// also need post_load_target for the post-load hook
	w_ecs_request_entity_with_name(&g_world, "post_load_target");

	struct wm_deserialisation_ctx dctx = {0};
	bool result = w_serialisation_restore_from_buffer(&g_world, buf, strlen(buf), &dctx);
	ck_assert(result);

	// migration hook should have run
	w_entity_id mig_ent = w_ecs_get_entity_by_name(&g_world, "migrate_target");
	w_entity_id c_mig = w_ecs_get_component_by_name(&g_world, "migrated_v1");
	bool *v_mig = w_ecs_get_component_(&g_world, c_mig, mig_ent);
	ck_assert_ptr_nonnull(v_mig);
	ck_assert(*v_mig == true);

	// post-load hook should have run
	w_entity_id pl_ent = w_ecs_get_entity_by_name(&g_world, "post_load_target");
	w_entity_id c_pl = w_ecs_get_component_by_name(&g_world, "loaded_a");
	bool *v_pl = w_ecs_get_component_(&g_world, c_pl, pl_ent);
	ck_assert_ptr_nonnull(v_pl);
	ck_assert(*v_pl == true);

	free(dctx.unparsed);
	free(buf);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *serialisation_suite(void)
{
	Suite *s = suite_create("serialisation");

	TCase *tc_smoke = tcase_create("smoke");
	tcase_add_checked_fixture(tc_smoke, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_smoke, 10);
	tcase_add_test(tc_smoke, test_init_free_no_crash);
	suite_add_tcase(s, tc_smoke);

	TCase *tc_persistent = tcase_create("persistent_entities");
	tcase_add_checked_fixture(tc_persistent, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_persistent, 10);
	tcase_add_test(tc_persistent, test_persistent_entities_includes_named);
	tcase_add_test(tc_persistent, test_persistent_entities_excludes_unnamed);
	tcase_add_test(tc_persistent, test_persistent_entities_mixed);
	tcase_add_test(tc_persistent, test_persistent_entities_empty_world);
	suite_add_tcase(s, tc_persistent);

	TCase *tc_components = tcase_create("components_list");
	tcase_add_checked_fixture(tc_components, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_components, 10);
	tcase_add_test(tc_components, test_components_list_empty_world);
	tcase_add_test(tc_components, test_components_list_single_component);
	tcase_add_test(tc_components, test_components_list_multiple_components);
	tcase_add_test(tc_components, test_components_list_returns_unique_ids);
	suite_add_tcase(s, tc_components);

	TCase *tc_dump = tcase_create("dump_to_buffer");
	tcase_add_checked_fixture(tc_dump, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_dump, 10);
	tcase_add_test(tc_dump, test_dump_to_buffer_returns_true_on_success);
	tcase_add_test(tc_dump, test_dump_to_buffer_contains_whisker_save_header);
	tcase_add_test(tc_dump, test_dump_to_buffer_contains_version_line);
	tcase_add_test(tc_dump, test_dump_to_buffer_contains_created_timestamp);
	tcase_add_test(tc_dump, test_dump_to_buffer_entities_count_matches);
	tcase_add_test(tc_dump, test_dump_to_buffer_components_count_matches);
	tcase_add_test(tc_dump, test_dump_to_buffer_entity_commands_written);
	tcase_add_test(tc_dump, test_dump_to_buffer_entities_saved_matches_count);
	suite_add_tcase(s, tc_dump);

	TCase *tc_comp_set = tcase_create("component_set_commands");
	tcase_add_checked_fixture(tc_comp_set, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_comp_set, 10);
	tcase_add_test(tc_comp_set, test_dump_component_set_command_written);
	tcase_add_test(tc_comp_set, test_dump_component_type_name_in_set_command);
	tcase_add_test(tc_comp_set, test_dump_entity_name_in_set_command);
	tcase_add_test(tc_comp_set, test_dump_hook_params_in_set_command);
	tcase_add_test(tc_comp_set, test_dump_multiple_components_multiple_set_commands);
	tcase_add_test(tc_comp_set, test_dump_empty_params_no_crash);
	tcase_add_test(tc_comp_set, test_dump_component_no_hook_still_writes_set);
	suite_add_tcase(s, tc_comp_set);

	TCase *tc_types = tcase_create("types");
	tcase_add_checked_fixture(tc_types, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_types, 10);
	tcase_add_test(tc_types, test_bool_true_serialises_as_true);
	tcase_add_test(tc_types, test_bool_false_serialises_as_false);
	tcase_add_test(tc_types, test_bool_set_command_format);
	suite_add_tcase(s, tc_types);

	TCase *tc_restore = tcase_create("restore_from_buffer");
	tcase_add_checked_fixture(tc_restore, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_restore, 10);
	tcase_add_test(tc_restore, test_restore_returns_true_on_valid_buffer);
	tcase_add_test(tc_restore, test_restore_fails_without_header);
	tcase_add_test(tc_restore, test_restore_parses_version);
	tcase_add_test(tc_restore, test_restore_creates_entities);
	tcase_add_test(tc_restore, test_restore_entity_malformed_quote);
	tcase_add_test(tc_restore, test_restore_set_bool_true);
	tcase_add_test(tc_restore, test_restore_set_bool_false);
	tcase_add_test(tc_restore, test_restore_set_unknown_entity_fails);
	tcase_add_test(tc_restore, test_restore_set_unknown_type_fails);
	tcase_add_test(tc_restore, test_restore_unparsed_lines_collected);
	tcase_add_test(tc_restore, test_restore_multiple_entities_and_components);
	suite_add_tcase(s, tc_restore);

	TCase *tc_e2e = tcase_create("end_to_end");
	tcase_add_checked_fixture(tc_e2e, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_e2e, 10);
	tcase_add_test(tc_e2e, test_e2e_serialize_deserialize_bool);
	tcase_add_test(tc_e2e, test_e2e_entity_ids_may_differ);
	suite_add_tcase(s, tc_e2e);

	TCase *tc_migration = tcase_create("migration");
	tcase_add_checked_fixture(tc_migration, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_migration, 10);
	tcase_add_test(tc_migration, test_migration_hooks_run_on_version_0);
	tcase_add_test(tc_migration, test_migration_hooks_skipped_when_version_current);
	tcase_add_test(tc_migration, test_migration_hooks_partial_skip);
	tcase_add_test(tc_migration, test_migration_hooks_version_exceeds_hook_count);
	suite_add_tcase(s, tc_migration);

	TCase *tc_post_load = tcase_create("post_load");
	tcase_add_checked_fixture(tc_post_load, serialisation_setup, serialisation_teardown);
	tcase_set_timeout(tc_post_load, 10);
	tcase_add_test(tc_post_load, test_post_load_hooks_all_run_on_version_0);
	tcase_add_test(tc_post_load, test_post_load_hooks_all_run_regardless_of_version);
	tcase_add_test(tc_post_load, test_post_load_hooks_run_after_migration_hooks);
	suite_add_tcase(s, tc_post_load);

	return s;
}

int main(void)
{
	Suite *s = serialisation_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
