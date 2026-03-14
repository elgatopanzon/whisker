/**
 * @author      : ElGatoPanzon
 * @file        : whisker_relationships
 * @created     : Friday Mar 13, 2026 17:44:31 CST
 * @description : Entity relationship tracking with pair-map and adjacency indices
 */

#include "whisker_relationships.h"


/*****************************
*  internal helpers          *
*****************************/

// add entity_id to the adjacency list for 'entity' in the adjacency map
static void adjacency_add_(struct w_relationship_registry *reg,
	w_entity_id entity, w_entity_id related)
{
	struct w_relationship_adjacency_list *adj = NULL;
	w_hashmap_t_get(&reg->adjacency, entity, adj);

	if (!adj)
	{
		struct w_relationship_adjacency_list empty = {0};
		w_hashmap_t_set(&reg->adjacency, entity, empty);
		w_hashmap_t_get(&reg->adjacency, entity, adj);
	}

	// check for duplicate
	for (size_t i = 0; i < adj->entities_length; ++i)
	{
		if (adj->entities[i] == related) return;
	}

	w_array_ensure_alloc_block_size(adj->entities,
		adj->entities_length + 1,
		W_RELATIONSHIP_ADJACENCY_REALLOC_BLOCK_SIZE);

	adj->entities[adj->entities_length++] = related;
}

// remove entity_id from the adjacency list for 'entity'
static void adjacency_remove_(struct w_relationship_registry *reg,
	w_entity_id entity, w_entity_id related)
{
	struct w_relationship_adjacency_list *adj = NULL;
	w_hashmap_t_get(&reg->adjacency, entity, adj);
	if (!adj) return;

	for (size_t i = 0; i < adj->entities_length; ++i)
	{
		if (adj->entities[i] == related)
		{
			// swap-remove
			if (i < adj->entities_length - 1)
				adj->entities[i] = adj->entities[adj->entities_length - 1];
			adj->entities_length--;
			return;
		}
	}
}

// check if any pair_map entries still reference the (entity, related) pair
static bool pair_has_entries_(struct w_relationship_registry *reg,
	w_entity_id a, w_entity_id b)
{
	struct w_relationship_entry_list *list = NULL;
	uint64_t key = w_relationship_pack_pair(a, b);
	w_hashmap_t_get(&reg->pair_map, key, list);
	return list && list->entries_length > 0;
}

// free all dynamic arrays inside pair_map buckets
static void free_pair_map_arrays_(struct w_relationship_pair_map *map)
{
	for (size_t b = 0; b < map->buckets_length; ++b)
	{
		for (size_t e = 0; e < map->buckets[b].entries_length; ++e)
		{
			struct w_relationship_entry_list *list = &map->buckets[b].entries[e].value;
			free_null(list->entries);
		}
	}
}

// free all dynamic arrays inside adjacency map buckets
static void free_adjacency_map_arrays_(struct w_relationship_adjacency_map *map)
{
	for (size_t b = 0; b < map->buckets_length; ++b)
	{
		for (size_t e = 0; e < map->buckets[b].entries_length; ++e)
		{
			struct w_relationship_adjacency_list *adj = &map->buckets[b].entries[e].value;
			free_null(adj->entities);
		}
	}
}


/*****************************
*  registry implementation   *
*****************************/

void w_relationship_registry_init(struct w_relationship_registry *reg, struct w_arena *arena)
{
	reg->arena = arena;
	w_hashmap_t_init(&reg->pair_map, arena,
		W_RELATIONSHIP_PAIR_MAP_BUCKET_COUNT, w_xxhash64_hash, NULL);
	w_hashmap_t_init(&reg->adjacency, arena,
		W_RELATIONSHIP_ADJACENCY_BUCKET_COUNT, w_xxhash64_hash, NULL);
}

void w_relationship_registry_free(struct w_relationship_registry *reg)
{
	free_pair_map_arrays_(&reg->pair_map);
	free_adjacency_map_arrays_(&reg->adjacency);
	w_hashmap_t_free(&reg->pair_map);
	w_hashmap_t_free(&reg->adjacency);
}

void w_relationship_registry_add(struct w_relationship_registry *reg,
	w_entity_id owner, w_entity_id target, w_entity_id component_id)
{
	uint64_t key = w_relationship_pack_pair(owner, target);

	// get or create pair entry list
	struct w_relationship_entry_list *list = NULL;
	w_hashmap_t_get(&reg->pair_map, key, list);

	if (!list)
	{
		struct w_relationship_entry_list empty = {0};
		w_hashmap_t_set(&reg->pair_map, key, empty);
		w_hashmap_t_get(&reg->pair_map, key, list);
	}

	// check for duplicate
	for (size_t i = 0; i < list->entries_length; ++i)
	{
		if (list->entries[i].owner == owner &&
			list->entries[i].target == target &&
			list->entries[i].component_id == component_id)
			return;
	}

	// append entry
	w_array_ensure_alloc_block_size(list->entries,
		list->entries_length + 1,
		W_RELATIONSHIP_ENTRIES_REALLOC_BLOCK_SIZE);

	list->entries[list->entries_length].owner = owner;
	list->entries[list->entries_length].target = target;
	list->entries[list->entries_length].component_id = component_id;
	list->entries_length++;

	// update adjacency for both directions
	adjacency_add_(reg, owner, target);
	adjacency_add_(reg, target, owner);
}

bool w_relationship_registry_remove(struct w_relationship_registry *reg,
	w_entity_id owner, w_entity_id target, w_entity_id component_id)
{
	uint64_t key = w_relationship_pack_pair(owner, target);

	struct w_relationship_entry_list *list = NULL;
	w_hashmap_t_get(&reg->pair_map, key, list);
	if (!list) return false;

	for (size_t i = 0; i < list->entries_length; ++i)
	{
		if (list->entries[i].owner == owner &&
			list->entries[i].target == target &&
			list->entries[i].component_id == component_id)
		{
			// swap-remove
			if (i < list->entries_length - 1)
				list->entries[i] = list->entries[list->entries_length - 1];
			list->entries_length--;

			// if no more entries for this pair, clean up adjacency
			if (!pair_has_entries_(reg, owner, target))
			{
				adjacency_remove_(reg, owner, target);
				adjacency_remove_(reg, target, owner);
			}
			return true;
		}
	}
	return false;
}

void w_relationship_registry_remove_entity(struct w_relationship_registry *reg, w_entity_id entity)
{
	struct w_relationship_adjacency_list *adj = NULL;
	w_hashmap_t_get(&reg->adjacency, entity, adj);
	if (!adj || adj->entities_length == 0) return;

	// iterate a snapshot of the adjacency list (copy entity IDs to stack)
	size_t count = adj->entities_length;
	w_entity_id related[count];
	memcpy(related, adj->entities, count * sizeof(w_entity_id));

	for (size_t r = 0; r < count; ++r)
	{
		uint64_t key = w_relationship_pack_pair(entity, related[r]);

		// remove all pair entries involving this entity
		struct w_relationship_entry_list *list = NULL;
		w_hashmap_t_get(&reg->pair_map, key, list);
		if (list)
		{
			free_null(list->entries);
			list->entries_length = 0;
			list->entries_size = 0;

			bool pair_removed;
			w_hashmap_t_remove(&reg->pair_map, key, pair_removed);
			(void)pair_removed;
		}

		// remove entity from the related entity's adjacency
		adjacency_remove_(reg, related[r], entity);
	}

	// clear own adjacency entry
	// re-fetch since the pointer might have changed due to removals above
	w_hashmap_t_get(&reg->adjacency, entity, adj);
	if (adj)
	{
		free_null(adj->entities);
		adj->entities_length = 0;
		adj->entities_size = 0;

		bool adj_removed;
		w_hashmap_t_remove(&reg->adjacency, entity, adj_removed);
		(void)adj_removed;
	}
}

struct w_relationship_entry_list *w_relationship_registry_get_pair(
	struct w_relationship_registry *reg, w_entity_id a, w_entity_id b)
{
	uint64_t key = w_relationship_pack_pair(a, b);
	struct w_relationship_entry_list *list = NULL;
	w_hashmap_t_get(&reg->pair_map, key, list);
	if (list && list->entries_length == 0) return NULL;
	return list;
}

struct w_relationship_adjacency_list *w_relationship_registry_get_adjacent(
	struct w_relationship_registry *reg, w_entity_id entity)
{
	struct w_relationship_adjacency_list *adj = NULL;
	w_hashmap_t_get(&reg->adjacency, entity, adj);
	if (adj && adj->entities_length == 0) return NULL;
	return adj;
}


/*****************************
*  world hook callbacks      *
*****************************/

static void component_set_hook_(void *world_, void *data_)
{
	struct w_component_action_payload *p = data_;
	if (p->type_id != W_COMPONENT_TYPE_w_entity_id) return;

	struct w_ecs_world *world = world_;
	w_entity_id target = *(w_entity_id *)((uint8_t *)data_ + sizeof(*p));

	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (!reg) return;

	w_relationship_registry_add(reg, p->entity_id, target, p->type_entity_id);
}

static void component_remove_hook_(void *world_, void *data_)
{
	struct w_component_action_payload *p = data_;
	if (p->type_id != W_COMPONENT_TYPE_w_entity_id) return;

	struct w_ecs_world *world = world_;

	// read current value before the component is removed
	w_entity_id *target = w_ecs_get_component_(world, p->type_entity_id, p->entity_id);
	if (!target) return;

	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (!reg) return;

	w_relationship_registry_remove(reg, p->entity_id, *target, p->type_entity_id);
}

static void entity_destroy_hook_(void *world_, void *entity_)
{
	struct w_ecs_world *world = world_;
	w_entity_id entity = *(w_entity_id *)entity_;

	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (!reg) return;

	struct w_relationship_adjacency_list *adj =
		w_relationship_registry_get_adjacent(reg, entity);
	if (!adj || adj->entities_length == 0) return;

	// queue buffered command to clean up relationships at next sync point
	// safe to queue during flush: cmd_return_entity has already copied its
	// payload to a local var, so realloc of payload_data is harmless
	w_command_buffer_queue(&world->command_buffer,
		wm_relationships_cmd_remove_entity_, world,
		&entity, sizeof(entity));
}


/*****************************
*  command buffer functions  *
*****************************/

void wm_relationships_cmd_remove_entity_(void *world_, void *payload)
{
	struct w_ecs_world *world = world_;
	w_entity_id entity = *(w_entity_id *)payload;

	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (!reg) return;

	w_relationship_registry_remove_entity(reg, entity);
}


/*****************************
*  module API                *
*****************************/

void wm_relationships_init(struct w_ecs_world *world)
{
	struct w_relationship_registry *reg =
		w_arena_malloc(world->arena, sizeof(*reg));
	w_relationship_registry_init(reg, world->arena);
	w_singleton_set(world, "relationships", reg);

	// register hooks for transparent relationship tracking
	w_register_component_set_hook(world, component_set_hook_);
	w_register_component_remove_hook(world, component_remove_hook_);
	w_register_entity_destroy_hook(world, entity_destroy_hook_);
}

void wm_relationships_free(struct w_ecs_world *world)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (reg) w_relationship_registry_free(reg);
}

void wm_relationships_add(struct w_ecs_world *world,
	w_entity_id owner, w_entity_id target, w_entity_id component_id)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (!reg) return;
	w_relationship_registry_add(reg, owner, target, component_id);
}

bool wm_relationships_remove(struct w_ecs_world *world,
	w_entity_id owner, w_entity_id target, w_entity_id component_id)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (!reg) return false;
	return w_relationship_registry_remove(reg, owner, target, component_id);
}

struct w_relationship_entry_list *wm_relationships_get_pair(
	struct w_ecs_world *world, w_entity_id a, w_entity_id b)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (!reg) return NULL;
	return w_relationship_registry_get_pair(reg, a, b);
}

struct w_relationship_adjacency_list *wm_relationships_get_adjacent(
	struct w_ecs_world *world, w_entity_id entity)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(world);
	if (!reg) return NULL;
	return w_relationship_registry_get_adjacent(reg, entity);
}

struct w_relationship_registry *wm_relationships_get_registry(struct w_ecs_world *world)
{
	return w_singleton_get(world, "relationships");
}
