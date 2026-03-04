/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_component_registry
 * @created     : Tuesday Mar 03, 2026 13:14:46 CST
 */

#include "whisker_std.h"

#include "whisker_component_registry.h"

void w_component_registry_init(struct w_component_registry *registry, struct w_arena *arena, struct w_entity_registry *entities)
{
	registry->entities = entities;
	registry->arena = arena;

	// init entries array
	w_array_init_t(registry->entries, W_COMPONENT_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE);
	registry->entries_length = 0;

	// init entries bitset
	w_sparse_bitset_init(&registry->entries_bitset, arena, W_COMPONENT_REGISTRY_ENTRY_BITSET_PAGE_SIZE);
}

void w_component_registry_free(struct w_component_registry *registry)
{
	// free all component data
	struct w_sparse_bitset_intersect_cache intersect_cache = {0};
	intersect_cache.bitsets = w_mem_xmalloc_t(*intersect_cache.bitsets);
	intersect_cache.bitsets[0] = &registry->entries_bitset;
	intersect_cache.bitsets_length = 1;

	size_t components_set = w_sparse_bitset_intersect(&intersect_cache);
	for (size_t i = 0; i < components_set; i++)
	{
		struct w_component_entry *entry = &registry->entries[intersect_cache.indexes[i]];
		w_sparse_bitset_free(&entry->data_bitset);
		free_null(entry->data);
	}
	free_null(intersect_cache.bitsets);
	w_sparse_bitset_free(&registry->entries_bitset);
	w_sparse_bitset_intersect_free_cache(&intersect_cache);
	registry->entities = NULL;
	registry->arena = NULL;
	free_null(registry->entries);
	registry->entries_length = 0;
}

bool w_component_registry_has_entry(struct w_component_registry *registry, w_entity_id entity_type_id)
{
	return w_sparse_bitset_get(&registry->entries_bitset, entity_type_id);
}

struct w_component_entry *w_component_registry_get_entry(struct w_component_registry *registry, w_entity_id entity_type_id)
{
	return (w_component_registry_has_entry(registry, entity_type_id)) ? &registry->entries[entity_type_id] : NULL;
}


void *w_component_set_(struct w_component_registry *registry, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size)
{
	// ensure registry entries is sized for type_entity_id
	w_array_ensure_alloc_block_size(
		registry->entries, 
		type_entity_id + 1, 
		W_COMPONENT_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE
	);

	// ensure entry is init
	struct w_component_entry *entry = &registry->entries[type_entity_id];

	if (!w_component_registry_has_entry(registry, type_entity_id))
	{
		// set the entry bitset
		w_sparse_bitset_set(&registry->entries_bitset, type_entity_id);

		// init the entry
		struct w_component_entry *entry = &registry->entries[type_entity_id];
		entry->type_id = type_id;
		entry->type_size = data_size;

		w_sparse_bitset_init(&entry->data_bitset, registry->arena, W_COMPONENT_REGISTRY_DATA_BITSET_PAGE_SIZE);

		w_array_init_t(entry->data, W_COMPONENT_REGISTRY_DATA_REALLOC_BLOCK_SIZE_BASE * data_size);
		entry->data_length = 0;
	}

	// ensure entry data size is large enough for this entity
	w_array_ensure_alloc_block_size(
		entry->data,
		entity_id + 1,
		W_COMPONENT_REGISTRY_DATA_REALLOC_BLOCK_SIZE_BASE * data_size
	);

	// set the actual data
	memcpy(entry->data + (data_size * entity_id), data, data_size);
	w_sparse_bitset_set(&entry->data_bitset, entity_id);

	return entry->data + (data_size * entity_id);
}

void *w_component_get_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id)
{
	// early out if component entry doesn't exist
	if (!w_component_registry_has_entry(registry, type_entity_id)) return NULL;

	// early out if entry doesn't have the component
	if (!w_component_has_(registry, type_entity_id, entity_id)) return NULL;

	struct w_component_entry *entry = &registry->entries[type_entity_id];
	return &entry->data[entity_id * entry->type_size];
}

void w_component_remove_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id)
{
	// early out if component entry doesn't exist
	if (!w_component_registry_has_entry(registry, type_entity_id)) return;

	w_sparse_bitset_clear(&registry->entries[type_entity_id].data_bitset, entity_id);
}

bool w_component_has_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id)
{
	// early out if component entry doesn't exist
	if (!w_component_registry_has_entry(registry, type_entity_id)) return false;

	return w_sparse_bitset_get(&registry->entries[type_entity_id].data_bitset, entity_id);
}

// unsafe variants (skip bounds checks)

void *w_component_set_unsafe_(struct w_component_registry *registry, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size)
{
	struct w_component_entry *entry = &registry->entries[type_entity_id];
	memcpy(entry->data + (data_size * entity_id), data, data_size);
	w_sparse_bitset_set(&entry->data_bitset, entity_id);
	return entry->data + (data_size * entity_id);
}

void *w_component_get_unsafe_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id)
{
	struct w_component_entry *entry = &registry->entries[type_entity_id];
	return &entry->data[entity_id * entry->type_size];
}

bool w_component_has_unsafe_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id)
{
	return w_sparse_bitset_get(&registry->entries[type_entity_id].data_bitset, entity_id);
}


w_entity_id w_component_get_id(struct w_component_registry *registry, char *name)
{
	w_entity_id id = w_entity_lookup_by_name(registry->entities, name);

	// create and set name if doesn't exist
	// not thread-safe!
	if (id == W_ENTITY_INVALID)
	{
		id = w_entity_request(registry->entities);
		w_entity_set_name(registry->entities, id, name);
	}

	return id;
}

char *w_component_get_name(struct w_component_registry *registry, w_entity_id type_entity_id)
{
	// invalid entity, cannot have a name
	if (type_entity_id >= registry->entities->next_id) return NULL;

	// will return null if there's no name
	return w_entity_get_name(registry->entities, type_entity_id);
}
