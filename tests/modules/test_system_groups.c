/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_system_groups
 * @created     : Friday Mar 27, 2026 14:22:00 CST
 * @description : tests for whisker_system_group.h system group registry
 */

#include "whisker_std.h"
#include "whisker_arena.h"
#include "whisker_hook_registry.h"
#include "whisker_system_group.h"
#include "whisker_ecs_world.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_arena g_arena;

static void setup(void)
{
	w_arena_init(&g_arena, 4096);
	g_world.arena = &g_arena;
	w_array_init_t(g_world.module_resources, W_MODULE_RESOURCES_INITIAL_CAPACITY);
	for (size_t i = 0; i < W_MODULE_RESOURCES_INITIAL_CAPACITY; i++)
		g_world.module_resources[i] = NULL;
	w_system_registry_init(&g_world.systems, &g_arena);
	wm_system_group_init(&g_world);
}

static void teardown(void)
{
	wm_system_group_free(&g_world);
	w_system_registry_free(&g_world.systems);
	free_null(g_world.module_resources);
	w_arena_free(&g_arena);
}

/* helper: register a dummy system and return its ID */
static size_t reg_sys(const char *name)
{
	struct w_system sys = {.phase_id = 0, .update = NULL};
	return w_system_register_system(&g_world.systems, (char *)name, &sys);
}


/*****************************
*  init and free             *
*****************************/

START_TEST(test_init_arrays_allocated)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	ck_assert_ptr_nonnull(reg->root_groups);
	ck_assert_ptr_nonnull(reg->group_systems);
}
END_TEST

START_TEST(test_init_arrays_empty)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	ck_assert_int_eq(reg->root_groups_length, 0);
	ck_assert_int_eq(reg->group_systems_length, 0);
}
END_TEST

START_TEST(test_free_nulls_pointers)
{
	struct w_arena a;
	w_arena_init(&a, 4096);
	struct w_ecs_world w;
	memset(&w, 0, sizeof(w));
	w.arena = &a;
	w_array_init_t(w.module_resources, W_MODULE_RESOURCES_INITIAL_CAPACITY);
	for (size_t i = 0; i < W_MODULE_RESOURCES_INITIAL_CAPACITY; i++)
		w.module_resources[i] = NULL;
	wm_system_group_init(&w);
	wm_system_group_free(&w);
	ck_assert_ptr_null(wm_system_group_get_registry(&w));
	free_null(w.module_resources);
	w_hook_registry_free(&w.hooks[W_WORLD_HOOK_TYPE_UPDATE]);
	w_arena_free(&a);
}
END_TEST

START_TEST(test_free_empty_noop)
{
	struct w_arena a;
	w_arena_init(&a, 4096);
	struct w_ecs_world w;
	memset(&w, 0, sizeof(w));
	w.arena = &a;
	w_array_init_t(w.module_resources, W_MODULE_RESOURCES_INITIAL_CAPACITY);
	for (size_t i = 0; i < W_MODULE_RESOURCES_INITIAL_CAPACITY; i++)
		w.module_resources[i] = NULL;
	wm_system_group_init(&w);
	/* should not crash */
	wm_system_group_free(&w);
	free_null(w.module_resources);
	w_hook_registry_free(&w.hooks[W_WORLD_HOOK_TYPE_UPDATE]);
	w_arena_free(&a);
}
END_TEST


/*****************************
*  group registration        *
*****************************/

START_TEST(test_register_root_returns_id)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t id = w_system_group_register_root(reg, "root_a");
	ck_assert_int_eq(id, 0);
}
END_TEST

START_TEST(test_register_root_increments_length)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	ck_assert_int_eq(reg->group_systems_length, 0);
	w_system_group_register_root(reg, "root_a");
	ck_assert_int_eq(reg->group_systems_length, 1);
	w_system_group_register_root(reg, "root_b");
	ck_assert_int_eq(reg->group_systems_length, 2);
}
END_TEST

START_TEST(test_register_root_name_lookup)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t id = w_system_group_register_root(reg, "root_a");
	size_t *found = w_system_group_get_id_by_name(&g_world, "root_a");
	ck_assert_ptr_nonnull(found);
	ck_assert_int_eq(*found, id);
}
END_TEST

START_TEST(test_register_root_has_root_struct)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t id = w_system_group_register_root(reg, "root_a");
	ck_assert_ptr_nonnull(reg->root_groups[id]);
}
END_TEST

START_TEST(test_register_sub_returns_id)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	ck_assert_int_eq(sub_id, 1);
}
END_TEST

START_TEST(test_register_sub_increments_length)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	ck_assert_int_eq(reg->group_systems_length, 1);
	w_system_group_register_sub(reg, "root_a", "sub_a");
	ck_assert_int_eq(reg->group_systems_length, 2);
}
END_TEST

START_TEST(test_register_sub_name_lookup)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t *found = w_system_group_get_id_by_name(&g_world, "sub_a");
	ck_assert_ptr_nonnull(found);
	ck_assert_int_eq(*found, sub_id);
}
END_TEST

START_TEST(test_register_sub_null_root_struct)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	ck_assert_ptr_null(reg->root_groups[sub_id]);
}
END_TEST

START_TEST(test_register_sub_added_to_root)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	struct w_system_group_root *root = reg->root_groups[root_id];
	ck_assert_int_eq(root->sub_group_ids_length, 1);
	ck_assert_int_eq(root->sub_group_ids[0], sub_id);
}
END_TEST

START_TEST(test_register_multiple_subs_added_to_root)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sub_c = w_system_group_register_sub(reg, "root_a", "sub_c");
	struct w_system_group_root *root = reg->root_groups[root_id];
	ck_assert_int_eq(root->sub_group_ids_length, 3);
	ck_assert_int_eq(root->sub_group_ids[0], sub_a);
	ck_assert_int_eq(root->sub_group_ids[1], sub_b);
	ck_assert_int_eq(root->sub_group_ids[2], sub_c);
}
END_TEST

START_TEST(test_register_sub_unknown_root_returns_max)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t sub_id = w_system_group_register_sub(reg, "no_such_root", "sub_a");
	ck_assert_int_eq(sub_id, SIZE_MAX);
}
END_TEST

START_TEST(test_register_unknown_name_lookup_null)
{
	size_t *found = w_system_group_get_id_by_name(&g_world, "does_not_exist");
	ck_assert_ptr_null(found);
}
END_TEST


/*****************************
*  system assignment         *
*****************************/

START_TEST(test_assign_to_root_group)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, root_id, sys_id);
	ck_assert(w_sparse_bitset_get(&reg->group_systems[root_id], sys_id));
}
END_TEST

START_TEST(test_assign_to_sub_group)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);
	ck_assert(w_sparse_bitset_get(&reg->group_systems[sub_id], sys_id));
}
END_TEST

START_TEST(test_assign_does_not_affect_other_groups)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);
	ck_assert(!w_sparse_bitset_get(&reg->group_systems[root_id], sys_id));
}
END_TEST

START_TEST(test_assign_invalid_group_noop)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	reg_sys("sys_a");
	/* should not crash */
	w_system_group_assign_system(reg, 999, 0);
}
END_TEST

START_TEST(test_exclude_from_sub_group)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);
	ck_assert(w_sparse_bitset_get(&reg->group_systems[sub_id], sys_id));
	w_system_group_exclude_system(reg, sub_id, sys_id);
	ck_assert(!w_sparse_bitset_get(&reg->group_systems[sub_id], sys_id));
}
END_TEST

START_TEST(test_exclude_invalid_group_noop)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	/* should not crash */
	w_system_group_exclude_system(reg, 999, 0);
}
END_TEST

START_TEST(test_macro_assign_to_root_group)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	reg_sys("sys_a");
	w_system_group_assign_to_root_group(&g_world, "root_a", "sys_a");
	size_t *sid = w_system_get_id_by_name(&g_world.systems, "sys_a");
	ck_assert_ptr_nonnull(sid);
	ck_assert(w_sparse_bitset_get(&reg->group_systems[root_id], *sid));
}
END_TEST

START_TEST(test_macro_assign_to_sub_group)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	reg_sys("sys_a");
	w_system_group_assign_to_sub_group(&g_world, "sub_a", "sys_a");
	size_t *sid = w_system_get_id_by_name(&g_world.systems, "sys_a");
	ck_assert_ptr_nonnull(sid);
	ck_assert(w_sparse_bitset_get(&reg->group_systems[sub_id], *sid));
}
END_TEST

START_TEST(test_macro_assign_to_all_sub_groups)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	reg_sys("sys_a");
	w_system_group_assign_to_all_sub_groups(&g_world, "root_a", "sys_a");
	size_t *sid = w_system_get_id_by_name(&g_world.systems, "sys_a");
	ck_assert_ptr_nonnull(sid);
	ck_assert(w_sparse_bitset_get(&reg->group_systems[sub_a], *sid));
	ck_assert(w_sparse_bitset_get(&reg->group_systems[sub_b], *sid));
}
END_TEST

START_TEST(test_macro_exclude_from_sub_group)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	reg_sys("sys_a");
	w_system_group_assign_to_sub_group(&g_world, "sub_a", "sys_a");
	size_t *sid = w_system_get_id_by_name(&g_world.systems, "sys_a");
	ck_assert(w_sparse_bitset_get(&reg->group_systems[sub_id], *sid));
	w_system_group_exclude_from_sub_group(&g_world, "sub_a", "sys_a");
	ck_assert(!w_sparse_bitset_get(&reg->group_systems[sub_id], *sid));
}
END_TEST


/*****************************
*  state changes             *
*****************************/

START_TEST(test_get_changes_initial_entry_enables_sub_systems)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, SIZE_MAX, sub_id);
	ck_assert_int_eq(ch.enable_length, 1);
	ck_assert_int_eq(ch.enable[0], sys_id);
	ck_assert_int_eq(ch.disable_length, 0);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_get_changes_exit_to_no_sub_disables)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, sub_id, SIZE_MAX);
	ck_assert_int_eq(ch.disable_length, 1);
	ck_assert_int_eq(ch.disable[0], sys_id);
	ck_assert_int_eq(ch.enable_length, 0);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_get_changes_transition_enables_new_disables_old)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_a = reg_sys("sys_a");
	size_t sys_b = reg_sys("sys_b");
	w_system_group_assign_system(reg, sub_a, sys_a);
	w_system_group_assign_system(reg, sub_b, sys_b);

	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, sub_a, sub_b);
	ck_assert_int_eq(ch.enable_length, 1);
	ck_assert_int_eq(ch.enable[0], sys_b);
	ck_assert_int_eq(ch.disable_length, 1);
	ck_assert_int_eq(ch.disable[0], sys_a);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_get_changes_shared_system_no_change)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_shared = reg_sys("sys_shared");
	w_system_group_assign_system(reg, sub_a, sys_shared);
	w_system_group_assign_system(reg, sub_b, sys_shared);

	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, sub_a, sub_b);
	ck_assert_int_eq(ch.enable_length, 0);
	ck_assert_int_eq(ch.disable_length, 0);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_get_changes_root_protects_from_disable)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_pinned = reg_sys("sys_pinned");
	/* assign sys_pinned to root AND sub_a */
	w_system_group_assign_system(reg, root_id, sys_pinned);
	w_system_group_assign_system(reg, sub_a, sys_pinned);

	/* transitioning from sub_a to sub_b: sys_pinned is in root, must NOT disable */
	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, sub_a, sub_b);
	ck_assert_int_eq(ch.disable_length, 0);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_get_changes_root_system_not_in_enable)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_root = reg_sys("sys_root");
	size_t sys_b = reg_sys("sys_b");
	/* sys_root is in root and sub_b; sys_b is only in sub_b */
	w_system_group_assign_system(reg, root_id, sys_root);
	w_system_group_assign_system(reg, sub_b, sys_root);
	w_system_group_assign_system(reg, sub_b, sys_b);

	/* from sub_a: sys_root was already "active" via root, sys_b is new */
	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, sub_a, sub_b);
	/* sys_root in sub_b but not in sub_a: should be in enable */
	/* sys_b in sub_b but not in sub_a: should be in enable */
	ck_assert_int_eq(ch.enable_length, 2);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_get_changes_empty_subs_no_changes)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");

	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, sub_a, sub_b);
	ck_assert_int_eq(ch.enable_length, 0);
	ck_assert_int_eq(ch.disable_length, 0);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_get_changes_multiple_systems)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");

	/* sub_a: sys0, sys1, sys2 */
	/* sub_b: sys1, sys3, sys4 */
	/* shared: sys1 -> no change */
	/* disable: sys0, sys2 */
	/* enable: sys3, sys4 */
	size_t ids[5];
	char name[16];
	for (int i = 0; i < 5; i++) {
		snprintf(name, sizeof(name), "sys_%d", i);
		ids[i] = reg_sys(name);
	}
	w_system_group_assign_system(reg, sub_a, ids[0]);
	w_system_group_assign_system(reg, sub_a, ids[1]);
	w_system_group_assign_system(reg, sub_a, ids[2]);
	w_system_group_assign_system(reg, sub_b, ids[1]);
	w_system_group_assign_system(reg, sub_b, ids[3]);
	w_system_group_assign_system(reg, sub_b, ids[4]);

	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, sub_a, sub_b);
	ck_assert_int_eq(ch.enable_length, 2);
	ck_assert_int_eq(ch.disable_length, 2);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_changes_free_nulls_pointers)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	struct w_system_group_changes ch = w_system_group_get_changes(reg, root_id, SIZE_MAX, sub_id);
	w_system_group_changes_free(&ch);
	ck_assert_ptr_null(ch.enable);
	ck_assert_ptr_null(ch.disable);
	ck_assert_int_eq(ch.enable_length, 0);
	ck_assert_int_eq(ch.disable_length, 0);
}
END_TEST


/*****************************
*  active sub-group          *
*****************************/

START_TEST(test_change_sub_initial_returns_enable)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	ck_assert_int_eq(ch.enable_length, 1);
	ck_assert_int_eq(ch.enable[0], sys_id);
	ck_assert_int_eq(ch.disable_length, 0);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_change_sub_stores_active_id)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	size_t active = w_system_group_get_active_sub_id(reg, root_id);
	ck_assert_int_eq(active, sub_id);
}
END_TEST

START_TEST(test_change_sub_noop_same_state)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	struct w_system_group_changes first = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&first);

	struct w_system_group_changes second = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	ck_assert_int_eq(second.enable_length, 0);
	ck_assert_int_eq(second.disable_length, 0);
	w_system_group_changes_free(&second);
}
END_TEST

START_TEST(test_get_active_sub_id_initial)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	ck_assert_int_eq(w_system_group_get_active_sub_id(reg, root_id), SIZE_MAX);
}
END_TEST

START_TEST(test_is_active_group_name_match)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	ck_assert(w_system_group_is_active_group_name(reg, root_id, "sub_a"));
}
END_TEST

START_TEST(test_is_active_group_name_no_match)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	w_system_group_register_sub(reg, "root_a", "sub_b");

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_a);
	w_system_group_changes_free(&ch);

	ck_assert(!w_system_group_is_active_group_name(reg, root_id, "sub_b"));
}
END_TEST

START_TEST(test_change_group_macro)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	struct w_system_group_changes ch = w_system_group_change_group(&g_world, "root_a", "sub_a");
	ck_assert_int_eq(ch.enable_length, 1);
	ck_assert_int_eq(ch.enable[0], sys_id);
	w_system_group_changes_free(&ch);
}
END_TEST

START_TEST(test_get_active_group_id_macro)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");

	struct w_system_group_changes ch = w_system_group_change_group(&g_world, "root_a", "sub_a");
	w_system_group_changes_free(&ch);

	ck_assert_int_eq(w_system_group_get_active_group_id(&g_world, "root_a"), sub_id);
}
END_TEST

START_TEST(test_is_group_macro)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	w_system_group_register_sub(reg, "root_a", "sub_a");
	w_system_group_register_sub(reg, "root_a", "sub_b");

	struct w_system_group_changes ch = w_system_group_change_group(&g_world, "root_a", "sub_a");
	w_system_group_changes_free(&ch);

	ck_assert(w_system_group_is_group(&g_world, "root_a", "sub_a"));
	ck_assert(!w_system_group_is_group(&g_world, "root_a", "sub_b"));
}
END_TEST


/*****************************
*  registry apply            *
*****************************/

START_TEST(test_change_sub_enables_system_in_registry)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	/* pre-disable so the enable is observable */
	w_system_set_system_state(&g_world.systems, sys_id, false);
	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_id)->enabled);

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	/* queue must be processed to apply deferred changes */
	w_system_group_process_queue(reg, &g_world.systems);

	ck_assert(w_system_get_system_entry(&g_world.systems, sys_id)->enabled);
}
END_TEST

START_TEST(test_change_sub_disables_system_in_registry)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_a, sys_id);

	/* enter sub_a: sys_a should be enabled after processing */
	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);
	w_system_group_process_queue(reg, &g_world.systems);
	ck_assert(w_system_get_system_entry(&g_world.systems, sys_id)->enabled);

	/* transition to sub_b: sys_a not in sub_b or root, must be disabled after processing */
	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);
	w_system_group_process_queue(reg, &g_world.systems);
	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_id)->enabled);
}
END_TEST

START_TEST(test_change_sub_shared_system_stays_enabled)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_shared = reg_sys("sys_shared");
	w_system_group_assign_system(reg, sub_a, sys_shared);
	w_system_group_assign_system(reg, sub_b, sys_shared);

	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);
	w_system_group_process_queue(reg, &g_world.systems);

	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);
	w_system_group_process_queue(reg, &g_world.systems);

	/* shared system present in both subs: must remain enabled after transition */
	ck_assert(w_system_get_system_entry(&g_world.systems, sys_shared)->enabled);
}
END_TEST

START_TEST(test_change_sub_root_system_not_disabled_on_transition)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_root = reg_sys("sys_root");
	w_system_group_assign_system(reg, root_id, sys_root);
	w_system_group_assign_system(reg, sub_a, sys_root);

	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);
	w_system_group_process_queue(reg, &g_world.systems);

	/* transition: sys_root in root, must NOT be disabled even though not in sub_b */
	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);
	w_system_group_process_queue(reg, &g_world.systems);

	ck_assert(w_system_get_system_entry(&g_world.systems, sys_root)->enabled);
}
END_TEST


/*****************************
*  queue and deferred apply  *
*****************************/

START_TEST(test_queue_change_increases_length)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	ck_assert_int_eq(reg->change_queue_length, 0);
	w_system_group_queue_change(reg, 0, true);
	ck_assert_int_eq(reg->change_queue_length, 1);
	w_system_group_queue_change(reg, 1, false);
	ck_assert_int_eq(reg->change_queue_length, 2);
}
END_TEST

START_TEST(test_queue_process_enables_system)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t sys_id = reg_sys("sys_a");
	w_system_set_system_state(&g_world.systems, sys_id, false);

	w_system_group_queue_change(reg, sys_id, true);
	w_system_group_process_queue(reg, &g_world.systems);

	ck_assert(w_system_get_system_entry(&g_world.systems, sys_id)->enabled);
}
END_TEST

START_TEST(test_queue_process_disables_system)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t sys_id = reg_sys("sys_a");
	/* registered systems start enabled */
	ck_assert(w_system_get_system_entry(&g_world.systems, sys_id)->enabled);

	w_system_group_queue_change(reg, sys_id, false);
	w_system_group_process_queue(reg, &g_world.systems);

	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_id)->enabled);
}
END_TEST

START_TEST(test_queue_process_clears_queue)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t sys_id = reg_sys("sys_a");
	w_system_group_queue_change(reg, sys_id, false);
	ck_assert_int_eq(reg->change_queue_length, 1);

	w_system_group_process_queue(reg, &g_world.systems);
	ck_assert_int_eq(reg->change_queue_length, 0);
}
END_TEST

START_TEST(test_change_sub_does_not_apply_before_process)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	/* pre-disable the system */
	w_system_set_system_state(&g_world.systems, sys_id, false);

	/* change_sub queues the enable but does not apply immediately */
	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	/* system must still be disabled until queue is processed */
	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_id)->enabled);
}
END_TEST

START_TEST(test_change_sub_populates_queue)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);

	ck_assert_int_eq(reg->change_queue_length, 0);
	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	ck_assert_int_gt(reg->change_queue_length, 0);
}
END_TEST


/*****************************
*  callbacks: on-enter/exit  *
*****************************/

START_TEST(test_register_on_enter_creates_callback_struct)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_enter");

	ck_assert_ptr_null(reg->group_callbacks[sub_id]);
	w_system_group_register_on_enter(reg, sub_id, sys_id);
	ck_assert_ptr_nonnull(reg->group_callbacks[sub_id]);
	ck_assert_int_eq(reg->group_callbacks[sub_id]->on_enter_length, 1);
	ck_assert_int_eq(reg->group_callbacks[sub_id]->on_enter[0], sys_id);
}
END_TEST

START_TEST(test_register_on_exit_creates_callback_struct)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_exit");

	ck_assert_ptr_null(reg->group_callbacks[sub_id]);
	w_system_group_register_on_exit(reg, sub_id, sys_id);
	ck_assert_ptr_nonnull(reg->group_callbacks[sub_id]);
	ck_assert_int_eq(reg->group_callbacks[sub_id]->on_exit_length, 1);
	ck_assert_int_eq(reg->group_callbacks[sub_id]->on_exit[0], sys_id);
}
END_TEST

START_TEST(test_macro_register_on_enter_system)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	reg_sys("sys_enter");

	w_system_group_register_on_enter_system(&g_world, "sub_a", "sys_enter");

	size_t *sid = w_system_get_id_by_name(&g_world.systems, "sys_enter");
	ck_assert_ptr_nonnull(reg->group_callbacks[sub_id]);
	ck_assert_int_eq(reg->group_callbacks[sub_id]->on_enter_length, 1);
	ck_assert_int_eq(reg->group_callbacks[sub_id]->on_enter[0], *sid);
}
END_TEST

START_TEST(test_macro_register_on_exit_system)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	reg_sys("sys_exit");

	w_system_group_register_on_exit_system(&g_world, "sub_a", "sys_exit");

	size_t *sid = w_system_get_id_by_name(&g_world.systems, "sys_exit");
	ck_assert_ptr_nonnull(reg->group_callbacks[sub_id]);
	ck_assert_int_eq(reg->group_callbacks[sub_id]->on_exit_length, 1);
	ck_assert_int_eq(reg->group_callbacks[sub_id]->on_exit[0], *sid);
}
END_TEST

START_TEST(test_on_enter_enabled_immediately_on_change_sub)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_enter = reg_sys("sys_enter");

	/* register as on-enter callback; disable it first */
	w_system_group_register_on_enter(reg, sub_id, sys_enter);
	w_system_set_system_state(&g_world.systems, sys_enter, false);

	/* change_sub must enable the on-enter callback immediately */
	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	ck_assert(w_system_get_system_entry(&g_world.systems, sys_enter)->enabled);
}
END_TEST

START_TEST(test_on_exit_enabled_immediately_on_change_sub)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_exit = reg_sys("sys_exit");

	/* register as on-exit callback for sub_a */
	w_system_group_register_on_exit(reg, sub_a, sys_exit);
	w_system_set_system_state(&g_world.systems, sys_exit, false);

	/* enter sub_a first */
	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);

	/* re-disable (enter may have no callbacks) */
	w_system_set_system_state(&g_world.systems, sys_exit, false);

	/* transition to sub_b: on-exit for sub_a must fire immediately */
	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);

	ck_assert(w_system_get_system_entry(&g_world.systems, sys_exit)->enabled);
}
END_TEST

START_TEST(test_on_enter_tracked_as_temporary)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_enter = reg_sys("sys_enter");

	w_system_group_register_on_enter(reg, sub_id, sys_enter);
	w_system_set_system_state(&g_world.systems, sys_enter, false);

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	/* on-enter system must be in the temp tracking bitset */
	ck_assert(w_sparse_bitset_get(&reg->temp_systems, sys_enter));
}
END_TEST

START_TEST(test_on_exit_tracked_as_temporary)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_exit = reg_sys("sys_exit");

	w_system_group_register_on_exit(reg, sub_a, sys_exit);

	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);

	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);

	/* on-exit system must be tracked as temporary */
	ck_assert(w_sparse_bitset_get(&reg->temp_systems, sys_exit));
}
END_TEST


/*****************************
*  hook: frame boundary      *
*****************************/

START_TEST(test_hook_disables_temp_systems)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_enter = reg_sys("sys_enter");

	w_system_group_register_on_enter(reg, sub_id, sys_enter);

	/* change_sub enables sys_enter and tracks as temp */
	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);
	ck_assert(w_system_get_system_entry(&g_world.systems, sys_enter)->enabled);

	/* simulate frame boundary: hook disables temp systems and processes queue */
	w_system_group_update_begin_hook_(&g_world, NULL);

	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_enter)->enabled);
}
END_TEST

START_TEST(test_hook_clears_temp_bitset)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_enter = reg_sys("sys_enter");

	w_system_group_register_on_enter(reg, sub_id, sys_enter);

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);
	ck_assert(w_sparse_bitset_get(&reg->temp_systems, sys_enter));

	w_system_group_update_begin_hook_(&g_world, NULL);

	ck_assert(!w_sparse_bitset_get(&reg->temp_systems, sys_enter));
}
END_TEST

START_TEST(test_hook_applies_queued_changes)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_id = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sys_id = reg_sys("sys_a");
	w_system_group_assign_system(reg, sub_id, sys_id);
	w_system_set_system_state(&g_world.systems, sys_id, false);

	/* change_sub queues the enable */
	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);
	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_id)->enabled);

	/* hook applies the queue */
	w_system_group_update_begin_hook_(&g_world, NULL);

	ck_assert(w_system_get_system_entry(&g_world.systems, sys_id)->enabled);
}
END_TEST

START_TEST(test_hook_clears_queue)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	w_system_group_queue_change(reg, 0, true);
	ck_assert_int_gt(reg->change_queue_length, 0);

	w_system_group_update_begin_hook_(&g_world, NULL);

	ck_assert_int_eq(reg->change_queue_length, 0);
}
END_TEST

START_TEST(test_full_flow_callback_disabled_next_frame)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_world);
	size_t root_id = w_system_group_register_root(reg, "root_a");
	size_t sub_a = w_system_group_register_sub(reg, "root_a", "sub_a");
	size_t sub_b = w_system_group_register_sub(reg, "root_a", "sub_b");
	size_t sys_perm = reg_sys("sys_perm");
	size_t sys_enter = reg_sys("sys_enter");
	size_t sys_exit = reg_sys("sys_exit");

	/* sub_b has sys_perm as permanent system */
	w_system_group_assign_system(reg, sub_b, sys_perm);
	/* sub_b has sys_enter as on-enter callback */
	w_system_group_register_on_enter(reg, sub_b, sys_enter);
	/* sub_a has sys_exit as on-exit callback */
	w_system_group_register_on_exit(reg, sub_a, sys_exit);

	/* start in sub_a */
	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);
	w_system_group_update_begin_hook_(&g_world, NULL); /* frame 1 boundary */

	/* disable all to get a clean baseline */
	w_system_set_system_state(&g_world.systems, sys_perm, false);
	w_system_set_system_state(&g_world.systems, sys_enter, false);
	w_system_set_system_state(&g_world.systems, sys_exit, false);

	/* Frame N: transition sub_a -> sub_b */
	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_world.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);

	/* Frame N: callbacks run (enabled immediately), perm still off (queued) */
	ck_assert(w_system_get_system_entry(&g_world.systems, sys_enter)->enabled);
	ck_assert(w_system_get_system_entry(&g_world.systems, sys_exit)->enabled);
	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_perm)->enabled);

	/* Frame N+1 start: hook disables temporaries, applies queue */
	w_system_group_update_begin_hook_(&g_world, NULL);

	/* callbacks disabled, permanent system now on */
	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_enter)->enabled);
	ck_assert(!w_system_get_system_entry(&g_world.systems, sys_exit)->enabled);
	ck_assert(w_system_get_system_entry(&g_world.systems, sys_perm)->enabled);
}
END_TEST


/*****************************
*  integration helpers       *
*****************************/

/* counters incremented by integration test system functions */
static int g_int_enter_count;
static int g_int_exit_count;
static int g_int_perm_count;

static void int_sys_enter(void *ctx, double dt) { (void)ctx; (void)dt; g_int_enter_count++; }
static void int_sys_exit(void *ctx, double dt)  { (void)ctx; (void)dt; g_int_exit_count++; }
static void int_sys_perm(void *ctx, double dt)  { (void)ctx; (void)dt; g_int_perm_count++; }

/* integration fixture globals */
static struct w_ecs_world g_iworld;
static struct w_string_table g_itable;
static struct w_arena g_iarena;
static size_t g_int_phase_id;

static void int_setup(void)
{
	g_int_enter_count = 0;
	g_int_exit_count  = 0;
	g_int_perm_count  = 0;

	w_arena_init(&g_iarena, 64 * 1024);
	w_string_table_init(&g_itable, &g_iarena, 16, 64, NULL);
	w_ecs_world_init(&g_iworld, &g_itable, &g_iarena);

	/* one timestep: update_time_target == 0 means "always run once" */
	struct w_scheduler_time_step ts = {.enabled = true};
	size_t ts_id = w_ecs_register_system_time_step(&g_iworld, &ts);

	/* one phase tied to that timestep */
	struct w_scheduler_phase ph = {.enabled = true, .time_step_id = ts_id};
	g_int_phase_id = w_ecs_register_system_phase(&g_iworld, &ph);

	wm_system_group_init(&g_iworld);
}

static void int_teardown(void)
{
	wm_system_group_free(&g_iworld);
	w_ecs_world_free(&g_iworld);
	w_string_table_free(&g_itable);
	w_arena_free(&g_iarena);
}

/* register a system with a counter fn, disabled so the group controls it */
static size_t reg_int_sys(const char *name, void (*fn)(void *, double))
{
	struct w_system sys = {.phase_id = g_int_phase_id, .update = fn};
	size_t id = w_ecs_register_system(&g_iworld, (char *)name, &sys);
	w_ecs_set_system_state(&g_iworld, id, false);
	return id;
}

/* run one world tick, forcing job list rebuild so enabled-state changes are honoured */
static void run_tick(void)
{
	g_iworld.scheduler_jobs_dirty = true;
	w_ecs_update(&g_iworld);
}


/*****************************
*  integration tests         *
*****************************/

/* on-enter callback fires in the same frame as the transition, then never again */
START_TEST(test_integration_on_enter_runs_once)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_iworld);
	size_t root_id = w_system_group_register_root(reg, "root");
	size_t sub_id  = w_system_group_register_sub(reg, "root", "sub_a");
	size_t eid     = reg_int_sys("int_enter", int_sys_enter);
	w_system_group_register_on_enter(reg, sub_id, eid);

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_iworld.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	run_tick();
	ck_assert_int_eq(g_int_enter_count, 1);

	run_tick();
	run_tick();
	ck_assert_int_eq(g_int_enter_count, 1);
}
END_TEST

/* on-exit callback fires in the transition frame, then never again */
START_TEST(test_integration_on_exit_runs_once)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_iworld);
	size_t root_id = w_system_group_register_root(reg, "root");
	size_t sub_a   = w_system_group_register_sub(reg, "root", "sub_a");
	size_t sub_b   = w_system_group_register_sub(reg, "root", "sub_b");
	size_t xid     = reg_int_sys("int_exit", int_sys_exit);
	w_system_group_register_on_exit(reg, sub_a, xid);

	/* entering sub_a does not trigger its on-exit */
	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_iworld.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);
	run_tick();
	ck_assert_int_eq(g_int_exit_count, 0);

	/* transition out of sub_a fires on-exit exactly once */
	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_iworld.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);
	run_tick();
	ck_assert_int_eq(g_int_exit_count, 1);

	run_tick();
	run_tick();
	ck_assert_int_eq(g_int_exit_count, 1);
}
END_TEST

/* multiple ticks after state change: callback counter stays at 1 */
START_TEST(test_integration_callback_no_rerun_after_ticks)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_iworld);
	size_t root_id = w_system_group_register_root(reg, "root");
	size_t sub_id  = w_system_group_register_sub(reg, "root", "sub_a");
	size_t eid     = reg_int_sys("int_enter2", int_sys_enter);
	w_system_group_register_on_enter(reg, sub_id, eid);

	struct w_system_group_changes ch = w_system_group_change_sub(reg, &g_iworld.systems, root_id, sub_id);
	w_system_group_changes_free(&ch);

	run_tick();
	ck_assert_int_eq(g_int_enter_count, 1);

	for (int i = 0; i < 5; i++) run_tick();
	ck_assert_int_eq(g_int_enter_count, 1);
}
END_TEST

/* transition -> tick -> transition -> tick: callback fires once per transition */
START_TEST(test_integration_each_transition_fires_once)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_iworld);
	size_t root_id = w_system_group_register_root(reg, "root");
	size_t sub_a   = w_system_group_register_sub(reg, "root", "sub_a");
	size_t sub_b   = w_system_group_register_sub(reg, "root", "sub_b");
	size_t eid     = reg_int_sys("int_enter3", int_sys_enter);
	w_system_group_register_on_enter(reg, sub_a, eid);
	w_system_group_register_on_enter(reg, sub_b, eid);

	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_iworld.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);
	run_tick();
	ck_assert_int_eq(g_int_enter_count, 1);

	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_iworld.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);
	run_tick();
	ck_assert_int_eq(g_int_enter_count, 2);
}
END_TEST

/* regular (permanent) systems run each tick while active, stop after group change */
START_TEST(test_integration_regular_system_runs_when_active)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(&g_iworld);
	size_t root_id = w_system_group_register_root(reg, "root");
	size_t sub_a   = w_system_group_register_sub(reg, "root", "sub_a");
	size_t sub_b   = w_system_group_register_sub(reg, "root", "sub_b");
	size_t pid     = reg_int_sys("int_perm", int_sys_perm);
	w_system_group_assign_system(reg, sub_a, pid);

	/* transition to sub_a: enable is queued, not yet applied */
	struct w_system_group_changes ch1 = w_system_group_change_sub(reg, &g_iworld.systems, root_id, sub_a);
	w_system_group_changes_free(&ch1);

	/* tick 1: hook applies the queued enable; perm not in job list yet */
	run_tick();
	ck_assert_int_eq(g_int_perm_count, 0);

	/* tick 2 and 3: perm enabled in job list, dispatched each tick */
	run_tick();
	ck_assert_int_eq(g_int_perm_count, 1);
	run_tick();
	ck_assert_int_eq(g_int_perm_count, 2);

	/* transition to sub_b: disable is queued */
	struct w_system_group_changes ch2 = w_system_group_change_sub(reg, &g_iworld.systems, root_id, sub_b);
	w_system_group_changes_free(&ch2);

	/* tick 4: hook applies disable; perm was in job list so runs one last time */
	run_tick();
	ck_assert_int_eq(g_int_perm_count, 3);

	/* tick 5: perm excluded from rebuilt job list */
	run_tick();
	ck_assert_int_eq(g_int_perm_count, 3);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_system_group_suite(void)
{
	Suite *s = suite_create("whisker_system_group");

	TCase *tc_init = tcase_create("init_free");
	tcase_add_checked_fixture(tc_init, setup, teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_arrays_allocated);
	tcase_add_test(tc_init, test_init_arrays_empty);
	tcase_add_test(tc_init, test_free_nulls_pointers);
	tcase_add_test(tc_init, test_free_empty_noop);
	suite_add_tcase(s, tc_init);

	TCase *tc_reg = tcase_create("group_register");
	tcase_add_checked_fixture(tc_reg, setup, teardown);
	tcase_set_timeout(tc_reg, 10);
	tcase_add_test(tc_reg, test_register_root_returns_id);
	tcase_add_test(tc_reg, test_register_root_increments_length);
	tcase_add_test(tc_reg, test_register_root_name_lookup);
	tcase_add_test(tc_reg, test_register_root_has_root_struct);
	tcase_add_test(tc_reg, test_register_sub_returns_id);
	tcase_add_test(tc_reg, test_register_sub_increments_length);
	tcase_add_test(tc_reg, test_register_sub_name_lookup);
	tcase_add_test(tc_reg, test_register_sub_null_root_struct);
	tcase_add_test(tc_reg, test_register_sub_added_to_root);
	tcase_add_test(tc_reg, test_register_multiple_subs_added_to_root);
	tcase_add_test(tc_reg, test_register_sub_unknown_root_returns_max);
	tcase_add_test(tc_reg, test_register_unknown_name_lookup_null);
	suite_add_tcase(s, tc_reg);

	TCase *tc_assign = tcase_create("system_assign");
	tcase_add_checked_fixture(tc_assign, setup, teardown);
	tcase_set_timeout(tc_assign, 10);
	tcase_add_test(tc_assign, test_assign_to_root_group);
	tcase_add_test(tc_assign, test_assign_to_sub_group);
	tcase_add_test(tc_assign, test_assign_does_not_affect_other_groups);
	tcase_add_test(tc_assign, test_assign_invalid_group_noop);
	tcase_add_test(tc_assign, test_exclude_from_sub_group);
	tcase_add_test(tc_assign, test_exclude_invalid_group_noop);
	tcase_add_test(tc_assign, test_macro_assign_to_root_group);
	tcase_add_test(tc_assign, test_macro_assign_to_sub_group);
	tcase_add_test(tc_assign, test_macro_assign_to_all_sub_groups);
	tcase_add_test(tc_assign, test_macro_exclude_from_sub_group);
	suite_add_tcase(s, tc_assign);

	TCase *tc_changes = tcase_create("state_changes");
	tcase_add_checked_fixture(tc_changes, setup, teardown);
	tcase_set_timeout(tc_changes, 10);
	tcase_add_test(tc_changes, test_get_changes_initial_entry_enables_sub_systems);
	tcase_add_test(tc_changes, test_get_changes_exit_to_no_sub_disables);
	tcase_add_test(tc_changes, test_get_changes_transition_enables_new_disables_old);
	tcase_add_test(tc_changes, test_get_changes_shared_system_no_change);
	tcase_add_test(tc_changes, test_get_changes_root_protects_from_disable);
	tcase_add_test(tc_changes, test_get_changes_root_system_not_in_enable);
	tcase_add_test(tc_changes, test_get_changes_empty_subs_no_changes);
	tcase_add_test(tc_changes, test_get_changes_multiple_systems);
	tcase_add_test(tc_changes, test_changes_free_nulls_pointers);
	suite_add_tcase(s, tc_changes);

	TCase *tc_active = tcase_create("active_sub");
	tcase_add_checked_fixture(tc_active, setup, teardown);
	tcase_set_timeout(tc_active, 10);
	tcase_add_test(tc_active, test_change_sub_initial_returns_enable);
	tcase_add_test(tc_active, test_change_sub_stores_active_id);
	tcase_add_test(tc_active, test_change_sub_noop_same_state);
	tcase_add_test(tc_active, test_get_active_sub_id_initial);
	tcase_add_test(tc_active, test_is_active_group_name_match);
	tcase_add_test(tc_active, test_is_active_group_name_no_match);
	tcase_add_test(tc_active, test_change_group_macro);
	tcase_add_test(tc_active, test_get_active_group_id_macro);
	tcase_add_test(tc_active, test_is_group_macro);
	suite_add_tcase(s, tc_active);

	TCase *tc_apply = tcase_create("registry_apply");
	tcase_add_checked_fixture(tc_apply, setup, teardown);
	tcase_set_timeout(tc_apply, 10);
	tcase_add_test(tc_apply, test_change_sub_enables_system_in_registry);
	tcase_add_test(tc_apply, test_change_sub_disables_system_in_registry);
	tcase_add_test(tc_apply, test_change_sub_shared_system_stays_enabled);
	tcase_add_test(tc_apply, test_change_sub_root_system_not_disabled_on_transition);
	suite_add_tcase(s, tc_apply);

	TCase *tc_queue = tcase_create("queue");
	tcase_add_checked_fixture(tc_queue, setup, teardown);
	tcase_set_timeout(tc_queue, 10);
	tcase_add_test(tc_queue, test_queue_change_increases_length);
	tcase_add_test(tc_queue, test_queue_process_enables_system);
	tcase_add_test(tc_queue, test_queue_process_disables_system);
	tcase_add_test(tc_queue, test_queue_process_clears_queue);
	tcase_add_test(tc_queue, test_change_sub_does_not_apply_before_process);
	tcase_add_test(tc_queue, test_change_sub_populates_queue);
	suite_add_tcase(s, tc_queue);

	TCase *tc_callbacks = tcase_create("callbacks");
	tcase_add_checked_fixture(tc_callbacks, setup, teardown);
	tcase_set_timeout(tc_callbacks, 10);
	tcase_add_test(tc_callbacks, test_register_on_enter_creates_callback_struct);
	tcase_add_test(tc_callbacks, test_register_on_exit_creates_callback_struct);
	tcase_add_test(tc_callbacks, test_macro_register_on_enter_system);
	tcase_add_test(tc_callbacks, test_macro_register_on_exit_system);
	tcase_add_test(tc_callbacks, test_on_enter_enabled_immediately_on_change_sub);
	tcase_add_test(tc_callbacks, test_on_exit_enabled_immediately_on_change_sub);
	tcase_add_test(tc_callbacks, test_on_enter_tracked_as_temporary);
	tcase_add_test(tc_callbacks, test_on_exit_tracked_as_temporary);
	suite_add_tcase(s, tc_callbacks);

	TCase *tc_hook = tcase_create("hook");
	tcase_add_checked_fixture(tc_hook, setup, teardown);
	tcase_set_timeout(tc_hook, 10);
	tcase_add_test(tc_hook, test_hook_disables_temp_systems);
	tcase_add_test(tc_hook, test_hook_clears_temp_bitset);
	tcase_add_test(tc_hook, test_hook_applies_queued_changes);
	tcase_add_test(tc_hook, test_hook_clears_queue);
	tcase_add_test(tc_hook, test_full_flow_callback_disabled_next_frame);
	suite_add_tcase(s, tc_hook);

	TCase *tc_integration = tcase_create("integration");
	tcase_add_checked_fixture(tc_integration, int_setup, int_teardown);
	tcase_set_timeout(tc_integration, 10);
	tcase_add_test(tc_integration, test_integration_on_enter_runs_once);
	tcase_add_test(tc_integration, test_integration_on_exit_runs_once);
	tcase_add_test(tc_integration, test_integration_callback_no_rerun_after_ticks);
	tcase_add_test(tc_integration, test_integration_each_transition_fires_once);
	tcase_add_test(tc_integration, test_integration_regular_system_runs_when_active);
	suite_add_tcase(s, tc_integration);

	return s;
}

int main(void)
{
	Suite *s = whisker_system_group_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
