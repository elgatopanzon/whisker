/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_system_group
 * @created     : Friday Mar 27, 2026 14:22:00 CST
 * @description : system groups as a state machine with root groups and sub-group states
 */

#include "whisker_std.h"
#include "whisker_system_group.h"


void w_system_group_registry_init(struct w_system_group_registry *registry, struct w_arena *arena)
{
	registry->arena = arena;

	w_array_init_t(registry->root_groups, 16);
	registry->root_groups_length = 0;

	w_array_init_t(registry->group_systems, 16);
	registry->group_systems_length = 0;

	w_array_init_t(registry->group_callbacks, 16);
	registry->group_callbacks_length = 0;

	w_array_init_t(registry->change_queue, 16);
	registry->change_queue_length = 0;

	w_sparse_bitset_init(&registry->temp_systems, arena, W_SPARSE_BITSET_PAGE_SHIFT);

	w_hashmap_t_init(&registry->group_names, arena, 16, w_hashmap_hash_str, w_hashmap_eq_str);
}

void w_system_group_registry_free(struct w_system_group_registry *registry)
{
	// free each root group struct and its sub_group_ids array
	for (size_t i = 0; i < registry->root_groups_length; i++) {
		if (registry->root_groups[i]) {
			free_null(registry->root_groups[i]->sub_group_ids);
			free(registry->root_groups[i]);
			registry->root_groups[i] = NULL;
		}
	}

	free_null(registry->root_groups);
	registry->root_groups_length = 0;

	// free each group's bitset
	for (size_t i = 0; i < registry->group_systems_length; i++) {
		w_sparse_bitset_free(&registry->group_systems[i]);
	}

	free_null(registry->group_systems);
	registry->group_systems_length = 0;

	// free each group's callback arrays
	for (size_t i = 0; i < registry->group_callbacks_length; i++) {
		if (registry->group_callbacks[i]) {
			free_null(registry->group_callbacks[i]->on_enter);
			free_null(registry->group_callbacks[i]->on_exit);
			free(registry->group_callbacks[i]);
			registry->group_callbacks[i] = NULL;
		}
	}

	free_null(registry->group_callbacks);
	registry->group_callbacks_length = 0;

	free_null(registry->change_queue);
	registry->change_queue_length = 0;

	w_sparse_bitset_free(&registry->temp_systems);

	w_hashmap_t_free(&registry->group_names);
}

// allocate the next group ID slot in both parallel arrays
static size_t group_alloc_id_(struct w_system_group_registry *registry)
{
	size_t id = registry->group_systems_length;

	w_array_ensure_alloc_block_size(registry->root_groups, id + 1, 16);
	w_array_ensure_alloc_block_size(registry->group_systems, id + 1, 16);
	w_array_ensure_alloc_block_size(registry->group_callbacks, id + 1, 16);

	// init bitset for this group using the registry arena
	w_sparse_bitset_init(&registry->group_systems[id], registry->arena, W_SPARSE_BITSET_PAGE_SHIFT);

	// callbacks are NULL until registered
	registry->group_callbacks[id] = NULL;

	registry->root_groups_length = id + 1;
	registry->group_systems_length = id + 1;
	registry->group_callbacks_length = id + 1;

	return id;
}

size_t w_system_group_register_root(struct w_system_group_registry *registry, char *name)
{
	size_t id = group_alloc_id_(registry);

	// allocate root struct with initial capacity for sub-group IDs
	struct w_system_group_root *root = w_mem_xcalloc_t(1, struct w_system_group_root);
	w_array_init_t(root->sub_group_ids, 4);
	root->sub_group_ids_length = 0;

	root->current_sub_id = SIZE_MAX;
	registry->root_groups[id] = root;

	w_hashmap_t_set(&registry->group_names, name, id);

	return id;
}

size_t w_system_group_register_sub(struct w_system_group_registry *registry, char *root_name, char *sub_name)
{
	size_t *root_id_ptr;
	w_hashmap_t_get(&registry->group_names, root_name, root_id_ptr);
	if (!root_id_ptr) return SIZE_MAX;

	size_t root_id = *root_id_ptr;
	struct w_system_group_root *root = registry->root_groups[root_id];
	if (!root) return SIZE_MAX;

	size_t id = group_alloc_id_(registry);

	// sub-groups have NULL in root_groups (they are not roots)
	registry->root_groups[id] = NULL;

	w_hashmap_t_set(&registry->group_names, sub_name, id);

	// add this sub-group ID to the root's list
	w_array_ensure_alloc_block_size(root->sub_group_ids, root->sub_group_ids_length + 1, 4);
	root->sub_group_ids[root->sub_group_ids_length++] = id;

	return id;
}

void w_system_group_assign_system(struct w_system_group_registry *registry, size_t group_id, size_t system_id)
{
	if (registry->group_systems_length <= group_id) return;
	w_sparse_bitset_set(&registry->group_systems[group_id], system_id);
}

void w_system_group_exclude_system(struct w_system_group_registry *registry, size_t group_id, size_t system_id)
{
	if (registry->group_systems_length <= group_id) return;
	w_sparse_bitset_clear(&registry->group_systems[group_id], system_id);
}

// ensure a callbacks struct exists for group_id, allocating if needed
static struct w_system_group_callbacks *get_or_alloc_callbacks_(
	struct w_system_group_registry *registry, size_t group_id)
{
	if (group_id >= registry->group_callbacks_length) return NULL;
	if (!registry->group_callbacks[group_id]) {
		struct w_system_group_callbacks *cb = w_mem_xcalloc_t(1, struct w_system_group_callbacks);
		w_array_init_t(cb->on_enter, 4);
		cb->on_enter_length = 0;
		w_array_init_t(cb->on_exit, 4);
		cb->on_exit_length = 0;
		registry->group_callbacks[group_id] = cb;
	}
	return registry->group_callbacks[group_id];
}

void w_system_group_register_on_enter(struct w_system_group_registry *registry, size_t group_id, size_t system_id)
{
	struct w_system_group_callbacks *cb = get_or_alloc_callbacks_(registry, group_id);
	if (!cb) return;
	w_array_ensure_alloc_block_size(cb->on_enter, cb->on_enter_length + 1, 4);
	cb->on_enter[cb->on_enter_length++] = system_id;
}

void w_system_group_register_on_exit(struct w_system_group_registry *registry, size_t group_id, size_t system_id)
{
	struct w_system_group_callbacks *cb = get_or_alloc_callbacks_(registry, group_id);
	if (!cb) return;
	w_array_ensure_alloc_block_size(cb->on_exit, cb->on_exit_length + 1, 4);
	cb->on_exit[cb->on_exit_length++] = system_id;
}

struct w_system_group_changes w_system_group_get_changes(
	struct w_system_group_registry *registry,
	size_t root_group_id,
	size_t current_sub_id,
	size_t new_sub_id
)
{
	struct w_system_group_changes changes = {0};
	w_array_init_t(changes.enable, 8);
	changes.enable_length = 0;
	w_array_init_t(changes.disable, 8);
	changes.disable_length = 0;

	struct w_sparse_bitset *root_bs = (root_group_id < registry->group_systems_length)
		? &registry->group_systems[root_group_id] : NULL;

	struct w_sparse_bitset *old_bs = (current_sub_id != SIZE_MAX
		&& current_sub_id < registry->group_systems_length)
		? &registry->group_systems[current_sub_id] : NULL;

	struct w_sparse_bitset *new_bs = (new_sub_id != SIZE_MAX
		&& new_sub_id < registry->group_systems_length)
		? &registry->group_systems[new_sub_id] : NULL;

	// systems to enable: in new_sub but NOT in old_sub
	if (new_bs) {
		w_sparse_bitset_for_each(new_bs) {
			bool in_old = old_bs ? w_sparse_bitset_get(old_bs, i) : false;
			if (!in_old) {
				w_array_ensure_alloc_block_size(changes.enable, changes.enable_length + 1, 8);
				changes.enable[changes.enable_length++] = (size_t)i;
			}
		}
	}

	// systems to disable: in old_sub but NOT in new_sub AND NOT in root
	// root membership protects a system from being disabled on sub-group transitions
	if (old_bs) {
		w_sparse_bitset_for_each(old_bs) {
			bool in_new = new_bs ? w_sparse_bitset_get(new_bs, i) : false;
			bool in_root = root_bs ? w_sparse_bitset_get(root_bs, i) : false;
			if (!in_new && !in_root) {
				w_array_ensure_alloc_block_size(changes.disable, changes.disable_length + 1, 8);
				changes.disable[changes.disable_length++] = (size_t)i;
			}
		}
	}

	return changes;
}

void w_system_group_changes_free(struct w_system_group_changes *changes)
{
	free_null(changes->enable);
	changes->enable_length = 0;
	free_null(changes->disable);
	changes->disable_length = 0;
}

static struct w_system_group_changes make_empty_changes_(void)
{
	struct w_system_group_changes ch = {0};
	w_array_init_t(ch.enable, 8);
	ch.enable_length = 0;
	w_array_init_t(ch.disable, 8);
	ch.disable_length = 0;
	return ch;
}

void w_system_group_queue_change(struct w_system_group_registry *registry, size_t system_id, bool enable)
{
	w_array_ensure_alloc_block_size(registry->change_queue, registry->change_queue_length + 1, 16);
	registry->change_queue[registry->change_queue_length++] = (struct w_system_group_queue_entry){
		.system_id = system_id,
		.enable = enable,
	};
}

void w_system_group_process_queue(struct w_system_group_registry *registry, struct w_system_registry *sys_registry)
{
	for (size_t i = 0; i < registry->change_queue_length; i++) {
		struct w_system_group_queue_entry *e = &registry->change_queue[i];
		w_system_set_system_state(sys_registry, e->system_id, e->enable);
	}
	registry->change_queue_length = 0;
}

struct w_system_group_changes w_system_group_change_sub(
	struct w_system_group_registry *registry,
	struct w_system_registry *sys_registry,
	size_t root_id,
	size_t new_sub_id
)
{
	if (root_id >= registry->group_systems_length) return make_empty_changes_();
	struct w_system_group_root *root = registry->root_groups[root_id];
	if (!root) return make_empty_changes_();
	if (new_sub_id == root->current_sub_id) return make_empty_changes_();

	size_t old_sub_id = root->current_sub_id;

	struct w_system_group_changes ch = w_system_group_get_changes(
		registry, root_id, old_sub_id, new_sub_id);

	// queue permanent enable/disable for next frame application
	for (size_t i = 0; i < ch.enable_length; i++)
		w_system_group_queue_change(registry, ch.enable[i], true);
	for (size_t i = 0; i < ch.disable_length; i++)
		w_system_group_queue_change(registry, ch.disable[i], false);

	// enable on-exit callbacks for old sub immediately (temporary: run this frame only)
	if (old_sub_id != SIZE_MAX && old_sub_id < registry->group_callbacks_length) {
		struct w_system_group_callbacks *cb = registry->group_callbacks[old_sub_id];
		if (cb) {
			for (size_t i = 0; i < cb->on_exit_length; i++) {
				size_t sid = cb->on_exit[i];
				w_system_set_system_state(sys_registry, sid, true);
				w_sparse_bitset_set(&registry->temp_systems, sid);
			}
		}
	}

	// enable on-enter callbacks for new sub immediately (temporary: run this frame only)
	if (new_sub_id != SIZE_MAX && new_sub_id < registry->group_callbacks_length) {
		struct w_system_group_callbacks *cb = registry->group_callbacks[new_sub_id];
		if (cb) {
			for (size_t i = 0; i < cb->on_enter_length; i++) {
				size_t sid = cb->on_enter[i];
				w_system_set_system_state(sys_registry, sid, true);
				w_sparse_bitset_set(&registry->temp_systems, sid);
			}
		}
	}

	root->current_sub_id = new_sub_id;

	return ch;
}

size_t w_system_group_get_active_sub_id(struct w_system_group_registry *registry, size_t root_id)
{
	if (root_id >= registry->group_systems_length) return SIZE_MAX;
	struct w_system_group_root *root = registry->root_groups[root_id];
	if (!root) return SIZE_MAX;
	return root->current_sub_id;
}

bool w_system_group_is_active_group_name(struct w_system_group_registry *registry, size_t root_id, char *sub_name)
{
	size_t active = w_system_group_get_active_sub_id(registry, root_id);
	if (active == SIZE_MAX) return false;
	size_t *sid;
	w_hashmap_t_get(&registry->group_names, sub_name, sid);
	if (!sid) return false;
	return active == *sid;
}

void w_system_group_update_begin_hook_(void *ctx, void *data)
{
	(void)data;
	struct w_ecs_world *world = ctx;
	struct w_system_group_registry *reg = wm_system_group_get_registry(world);
	if (!reg) return;

	// disable all temporary systems from the previous frame
	w_sparse_bitset_for_each(&reg->temp_systems) {
		w_system_set_system_state(&world->systems, (size_t)i, false);
	}
	// clear the temp tracking set
	// free and reinit to reset the bitset without leaving stale bits
	w_sparse_bitset_free(&reg->temp_systems);
	w_sparse_bitset_init(&reg->temp_systems, reg->arena, W_SPARSE_BITSET_PAGE_SHIFT);

	// apply all queued enable/disable changes
	w_system_group_process_queue(reg, &world->systems);
}


/*****************************
*  module API                *
*****************************/

void wm_system_group_init(struct w_ecs_world *world)
{
	if (wm_system_group_get_registry(world)) return;

	struct w_system_group_registry *reg = w_mem_xcalloc_t(1, struct w_system_group_registry);
	w_system_group_registry_init(reg, world->arena);
	w_ecs_set_module_resource(world, W_SYSTEM_GROUP_MODULE_RESOURCE_ID, reg);

	w_ecs_register_update_hook(world, W_WORLD_HOOK_UPDATE_BEGIN, w_system_group_update_begin_hook_);
}

void wm_system_group_free(struct w_ecs_world *world)
{
	struct w_system_group_registry *reg = wm_system_group_get_registry(world);
	if (!reg) return;

	w_system_group_registry_free(reg);
	free(reg);
	w_ecs_clear_module_resource(world, W_SYSTEM_GROUP_MODULE_RESOURCE_ID);
}

struct w_system_group_registry *wm_system_group_get_registry(struct w_ecs_world *world)
{
	return w_ecs_get_module_resource(world, W_SYSTEM_GROUP_MODULE_RESOURCE_ID);
}
