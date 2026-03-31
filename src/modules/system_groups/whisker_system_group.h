/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_system_group
 * @created     : Friday Mar 27, 2026 14:22:00 CST
 * @description : system groups as a state machine with root groups and sub-group states
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_memory.h"
#include "whisker_hashmap.h"
#include "whisker_sparse_bitset.h"
#include "whisker_system_registry.h"
#include "whisker_ecs_world.h"

#ifndef WHISKER_SYSTEM_GROUP_H
#define WHISKER_SYSTEM_GROUP_H

// module resource ID for fast world lookup
#include "modules/whisker_module_ids.h"

/* index 0: the system group registry singleton */
#define WM_SYSTEM_GROUP_MODULE_RESOURCE_ID WM_MODULE_RESOURCE_ID(SYSTEM_GROUPS, 0)

// root group: holds list of sub-group IDs registered under it
struct w_system_group_root
{
	w_array_declare(size_t, sub_group_ids);
	size_t current_sub_id; // SIZE_MAX = no active sub-group
};

// per-sub-group callback system lists (on-enter and on-exit run for 1 frame)
struct w_system_group_callbacks
{
	w_array_declare(size_t, on_enter);
	w_array_declare(size_t, on_exit);
};

// result of computing system state changes for a sub-group transition
struct w_system_group_changes
{
	w_array_declare(size_t, enable);
	w_array_declare(size_t, disable);
};

// a single queued enable or disable action for the system registry
struct w_system_group_queue_entry
{
	size_t system_id;
	bool enable;
};

// name to group ID map
w_hashmap_t_declare(char*, size_t, w_system_group_name_map);

// registry holding all system groups
struct w_system_group_registry
{
	// name to group ID map
	struct w_system_group_name_map group_names;

	// per-group root structs: indexed by group ID
	// non-NULL only for root groups, NULL for sub-groups
	w_array_declare(struct w_system_group_root *, root_groups);

	// per-group system bitsets: indexed by group ID
	// length doubles as total group count
	w_array_declare(struct w_sparse_bitset, group_systems);

	// per-group callback lists: indexed by group ID, NULL if no callbacks registered
	w_array_declare(struct w_system_group_callbacks *, group_callbacks);

	// deferred enable/disable queue: processed at schedule start
	w_array_declare(struct w_system_group_queue_entry, change_queue);

	// bitset of temporary systems (on-enter/on-exit) to disable at next schedule start
	struct w_sparse_bitset temp_systems;

	struct w_arena *arena;
};

// initialize registry with arena
void w_system_group_registry_init(struct w_system_group_registry *registry, struct w_arena *arena);

// free all registry allocations
void w_system_group_registry_free(struct w_system_group_registry *registry);

// register a root group by name, returns group ID
size_t w_system_group_register_root(struct w_system_group_registry *registry, char *name);

// register a sub-group under an existing root group, returns group ID
// returns SIZE_MAX if root_name not found
size_t w_system_group_register_sub(struct w_system_group_registry *registry, char *root_name, char *sub_name);

// assign system_id to the bitset of group_id
void w_system_group_assign_system(struct w_system_group_registry *registry, size_t group_id, size_t system_id);

// clear system_id from the bitset of group_id
void w_system_group_exclude_system(struct w_system_group_registry *registry, size_t group_id, size_t system_id);

// register system_id as an on-enter callback for group_id (enabled for 1 frame when entering)
void w_system_group_register_on_enter(struct w_system_group_registry *registry, size_t group_id, size_t system_id);

// register system_id as an on-exit callback for group_id (enabled for 1 frame when exiting)
void w_system_group_register_on_exit(struct w_system_group_registry *registry, size_t group_id, size_t system_id);

// compute which systems to enable/disable when transitioning from current_sub to new_sub
// within the given root group. use SIZE_MAX for current_sub on initial entry.
// caller must free result with w_system_group_changes_free
struct w_system_group_changes w_system_group_get_changes(
	struct w_system_group_registry *registry,
	size_t root_group_id,
	size_t current_sub_id,
	size_t new_sub_id
);

// free arrays inside a changes struct
void w_system_group_changes_free(struct w_system_group_changes *changes);

// push a deferred enable/disable entry onto the change queue
void w_system_group_queue_change(struct w_system_group_registry *registry, size_t system_id, bool enable);

// apply all queued enable/disable actions to sys_registry then clear the queue
void w_system_group_process_queue(struct w_system_group_registry *registry, struct w_system_registry *sys_registry);

// change active sub-group for root_id to new_sub_id
// queues enable/disable for permanent systems (applied next frame)
// immediately enables temporary on-exit/on-enter callbacks for this frame
// marks those temporaries so they are disabled at the next schedule start
// no-op (returns empty changes) if new_sub_id == current active
// caller must free result with w_system_group_changes_free
struct w_system_group_changes w_system_group_change_sub(
	struct w_system_group_registry *registry,
	struct w_system_registry *sys_registry,
	size_t root_id,
	size_t new_sub_id
);

// get current active sub-group ID for root_id (SIZE_MAX if none or invalid)
size_t w_system_group_get_active_sub_id(struct w_system_group_registry *registry, size_t root_id);

// return true if sub_name resolves to the current active sub-group for root_id
bool w_system_group_is_active_group_name(struct w_system_group_registry *registry, size_t root_id, char *sub_name);

// hook registered at W_WORLD_HOOK_UPDATE_BEGIN: disables temp systems then processes queue
void w_system_group_update_begin_hook_(void *ctx, void *data);


/*****************************
*  module API                *
*****************************/

// initialize the system groups module on world (idempotent)
void wm_system_group_init(struct w_ecs_world *world);

// free the system groups module from world
void wm_system_group_free(struct w_ecs_world *world);

// get the registry from world module resources
struct w_system_group_registry *wm_system_group_get_registry(struct w_ecs_world *world);


/*****************************
*  convenience macros        *
*****************************/

// get group ID by name (returns pointer to size_t, or NULL if not found)
#define w_system_group_get_id_by_name(world, name) \
	({ \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		size_t *_out = NULL; \
		if (_reg) w_hashmap_t_get(&(_reg)->group_names, (name), _out); \
		_out; \
	})

// assign system (by name) to root group (always active systems)
#define w_system_group_assign_to_root_group(world, root_name, system_name) \
	do { \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		if (!_reg) break; \
		size_t *_gid; \
		w_hashmap_t_get(&(_reg)->group_names, (root_name), _gid); \
		size_t *_sid = w_system_get_id_by_name(&(world)->systems, system_name); \
		if (_gid && _sid) w_system_group_assign_system(_reg, *_gid, *_sid); \
	} while (0)

// assign system (by name) to a specific sub-group (active only in that state)
#define w_system_group_assign_to_sub_group(world, sub_name, system_name) \
	do { \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		if (!_reg) break; \
		size_t *_gid; \
		w_hashmap_t_get(&(_reg)->group_names, (sub_name), _gid); \
		size_t *_sid = w_system_get_id_by_name(&(world)->systems, system_name); \
		if (_gid && _sid) w_system_group_assign_system(_reg, *_gid, *_sid); \
	} while (0)

// assign system (by name) to all sub-groups of a root group
#define w_system_group_assign_to_all_sub_groups(world, root_name, system_name) \
	do { \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		if (!_reg) break; \
		size_t *_gid; \
		w_hashmap_t_get(&(_reg)->group_names, (root_name), _gid); \
		size_t *_sid = w_system_get_id_by_name(&(world)->systems, system_name); \
		if (_gid && _sid) { \
			struct w_system_group_root *_root = (_reg)->root_groups[*_gid]; \
			if (_root) { \
				for (size_t _i = 0; _i < _root->sub_group_ids_length; _i++) { \
					w_system_group_assign_system(_reg, _root->sub_group_ids[_i], *_sid); \
				} \
			} \
		} \
	} while (0)

// change active sub-group using names; queues enable/disable; returns w_system_group_changes (caller must free)
#define w_system_group_change_group(world, root_name, sub_name) \
	({ \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		size_t *_rid = NULL; \
		size_t *_sid = NULL; \
		if (_reg) { \
			w_hashmap_t_get(&(_reg)->group_names, (root_name), _rid); \
			w_hashmap_t_get(&(_reg)->group_names, (sub_name), _sid); \
		} \
		struct w_system_group_changes _ch = {0}; \
		if (_reg && _rid && _sid) _ch = w_system_group_change_sub(_reg, &(world)->systems, *_rid, *_sid); \
		_ch; \
	})

// get current active sub-group ID for a root group by name (SIZE_MAX if none)
#define w_system_group_get_active_group_id(world, root_name) \
	({ \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		size_t *_rid = NULL; \
		if (_reg) w_hashmap_t_get(&(_reg)->group_names, (root_name), _rid); \
		size_t _result = SIZE_MAX; \
		if (_rid) _result = w_system_group_get_active_sub_id(_reg, *_rid); \
		_result; \
	})

// return true if sub_name is the current active sub-group for root_name
#define w_system_group_is_group(world, root_name, sub_name) \
	({ \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		size_t *_rid = NULL; \
		if (_reg) w_hashmap_t_get(&(_reg)->group_names, (root_name), _rid); \
		bool _result = false; \
		if (_rid) _result = w_system_group_is_active_group_name(_reg, *_rid, sub_name); \
		_result; \
	})

// clear system (by name) from a specific sub-group (by name)
#define w_system_group_exclude_from_sub_group(world, sub_name, system_name) \
	do { \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		if (!_reg) break; \
		size_t *_gid; \
		w_hashmap_t_get(&(_reg)->group_names, (sub_name), _gid); \
		size_t *_sid = w_system_get_id_by_name(&(world)->systems, system_name); \
		if (_gid && _sid) w_system_group_exclude_system(_reg, *_gid, *_sid); \
	} while (0)

// register system (by name) as on-enter callback for a sub-group (by name)
#define w_system_group_register_on_enter_system(world, sub_name, system_name) \
	do { \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		if (!_reg) break; \
		size_t *_gid; \
		w_hashmap_t_get(&(_reg)->group_names, (sub_name), _gid); \
		size_t *_sid = w_system_get_id_by_name(&(world)->systems, system_name); \
		if (_gid && _sid) w_system_group_register_on_enter(_reg, *_gid, *_sid); \
	} while (0)

// register system (by name) as on-exit callback for a sub-group (by name)
#define w_system_group_register_on_exit_system(world, sub_name, system_name) \
	do { \
		struct w_system_group_registry *_reg = wm_system_group_get_registry(world); \
		if (!_reg) break; \
		size_t *_gid; \
		w_hashmap_t_get(&(_reg)->group_names, (sub_name), _gid); \
		size_t *_sid = w_system_get_id_by_name(&(world)->systems, system_name); \
		if (_gid && _sid) w_system_group_register_on_exit(_reg, *_gid, *_sid); \
	} while (0)

#endif /* WHISKER_SYSTEM_GROUP_H */
