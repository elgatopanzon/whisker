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
	w_array_init_t(registry->recycled_stack, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);

	registry->entity_to_name_length = 0;
	registry->name_to_entity_length = 0;
	atomic_store(&registry->recycled_stack_length, 0);

	atomic_store(&registry->next_id, 0);
	registry->name_table = name_table;
}

void w_entity_registry_free(struct w_entity_registry *registry)
{
	free_null(registry->entity_to_name);
	free_null(registry->name_to_entity);
	free_null(registry->recycled_stack);

	atomic_store(&registry->next_id, 0);
	atomic_store(&registry->recycled_stack_length, 0);
	registry->name_table = NULL;
}

w_entity_id w_entity_request(struct w_entity_registry *registry)
{
	w_entity_id id;

	// try to pop from recycled stack (thread-safe via CAS)
	while (true)
	{
		size_t current_len = atomic_load(&registry->recycled_stack_length);
		if (current_len > 0)
		{
			// try CAS decrement to claim a slot
			if (atomic_compare_exchange_weak(&registry->recycled_stack_length, &current_len, current_len - 1))
			{
				// success - we own slot at current_len - 1
				id = registry->recycled_stack[current_len - 1];
				break;
			}
			// CAS failed, retry
		}
		else
		{
			// stack empty, allocate new ID atomically
			id = atomic_fetch_add(&registry->next_id, 1);
			break;
		}
	}

	return id;
}

void w_entity_return(struct w_entity_registry *registry, w_entity_id id)
{
	// push to recycled stack (thread-safe via CAS)
	while (true)
	{
		size_t current_len = atomic_load(&registry->recycled_stack_length);
		// ensure capacity before CAS
		w_array_ensure_alloc_block_size(registry->recycled_stack, current_len + 1, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);
		// try CAS increment to claim a slot
		if (atomic_compare_exchange_weak(&registry->recycled_stack_length, &current_len, current_len + 1))
		{
			// success - we own slot at current_len
			registry->recycled_stack[current_len] = id;
			break;
		}
		// CAS failed, retry
	}

	// clear name if it has one
	w_entity_clear_name(registry, id);
}

void w_entity_set_name(struct w_entity_registry *registry, w_entity_id id, char *name)
{
	w_entity_clear_name(registry, id);

	w_array_ensure_alloc_block_size(registry->entity_to_name, id + 1, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);
	if (id + 1 > registry->entity_to_name_length) {
		registry->entity_to_name_length = id + 1;
	}

	registry->entity_to_name[id] = w_string_table_intern_str(registry->name_table, name);

	w_string_table_id string_id = registry->entity_to_name[id];
	w_array_ensure_alloc_block_size(registry->name_to_entity, string_id + 1, WHISKER_ENTITY_REGISTRY_REALLOC_BLOCK_SIZE);
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
