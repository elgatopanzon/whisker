/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_entity_registry
 * @created     : Sunday Mar 01, 2026 20:54:33 CST
 */

#include "whisker_std.h"

#include "whisker_entity_registry.h"

void w_entity_registry_init(struct w_entity_registry *registry, struct w_string_table *name_table)
{
	w_array_init_t(registry->entity_to_name, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);
	w_array_init_t(registry->name_to_entity, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);

	// initialize entity_to_name to invalid IDs (calloc zeros, but 0 is a valid string table ID)
	size_t entity_to_name_count = registry->entity_to_name_size / sizeof(*registry->entity_to_name);
	for (size_t i = 0; i < entity_to_name_count; i++) {
		registry->entity_to_name[i] = W_STRING_TABLE_INVALID_ID;
	}

	// initialize name_to_entity to invalid entity IDs (calloc zeros, but 0 is a valid entity ID)
	size_t name_to_entity_count = registry->name_to_entity_size / sizeof(*registry->name_to_entity);
	memset(registry->name_to_entity, 0xFF, name_to_entity_count * sizeof(*registry->name_to_entity));

	registry->entity_to_name_length = 0;
	registry->name_to_entity_length = 0;

	w_id_pool_init(&registry->id_pool, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);
	registry->name_table = name_table;
}

void w_entity_registry_free(struct w_entity_registry *registry)
{
	free_null(registry->entity_to_name);
	free_null(registry->name_to_entity);

	w_id_pool_free(&registry->id_pool);
	registry->name_table = NULL;
}

w_entity_id w_entity_request(struct w_entity_registry *registry)
{
	return w_id_pool_request(&registry->id_pool);
}

void w_entity_return(struct w_entity_registry *registry, w_entity_id id)
{
	w_id_pool_return(&registry->id_pool, id);

	// clear name if it has one
	w_entity_clear_name(registry, id);
}

void w_entity_set_name(struct w_entity_registry *registry, w_entity_id id, char *name)
{
	w_entity_clear_name(registry, id);

	size_t old_size = registry->entity_to_name_size;
	w_array_ensure_alloc_block_size(registry->entity_to_name, id + 1, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);

	// initialize new entries to invalid ID (calloc zeros, but 0 is a valid string table ID)
	if (registry->entity_to_name_size > old_size) {
		size_t old_count = old_size / sizeof(*registry->entity_to_name);
		size_t new_count = registry->entity_to_name_size / sizeof(*registry->entity_to_name);
		for (size_t i = old_count; i < new_count; i++) {
			registry->entity_to_name[i] = W_STRING_TABLE_INVALID_ID;
		}
	}

	if (id + 1 > registry->entity_to_name_length) {
		registry->entity_to_name_length = id + 1;
	}

	registry->entity_to_name[id] = w_string_table_intern_str(registry->name_table, name);

	w_string_table_id string_id = registry->entity_to_name[id];
	size_t old_name_size = registry->name_to_entity_size;
	w_array_ensure_alloc_block_size(registry->name_to_entity, string_id + 1, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);

	// initialize new entries to invalid entity ID (calloc zeros, but 0 is a valid entity ID)
	if (registry->name_to_entity_size > old_name_size) {
		size_t old_count = old_name_size / sizeof(*registry->name_to_entity);
		size_t new_count = registry->name_to_entity_size / sizeof(*registry->name_to_entity);
		memset(&registry->name_to_entity[old_count], 0xFF, (new_count - old_count) * sizeof(*registry->name_to_entity));
	}

	if (string_id + 1 > registry->name_to_entity_length) {
		registry->name_to_entity_length = string_id + 1;
	}

	registry->name_to_entity[string_id] = id;
}

void w_entity_clear_name(struct w_entity_registry *registry, w_entity_id id)
{
	if (registry->entity_to_name_length > id && registry->entity_to_name[id] != W_STRING_TABLE_INVALID_ID) { 
		w_string_table_id string_id = registry->entity_to_name[id];
		registry->entity_to_name[id] = W_STRING_TABLE_INVALID_ID; 

		registry->name_to_entity[string_id] = W_ENTITY_INVALID;
	}
}

char *w_entity_get_name(struct w_entity_registry *registry, w_entity_id id)
{
	if (registry->entity_to_name_length > id && registry->entity_to_name[id] != W_STRING_TABLE_INVALID_ID) { 
		return w_string_table_lookup(registry->name_table, registry->entity_to_name[id]);
	}
	return NULL;
}

w_entity_id w_entity_lookup_by_name(struct w_entity_registry *registry, char *name)
{
	w_string_table_id id = w_string_table_lookup_str(registry->name_table, name);
	if (id != W_STRING_TABLE_INVALID_ID  && registry->name_to_entity_length > id)
	{
		return registry->name_to_entity[id];
	}

	return W_ENTITY_INVALID;
}
