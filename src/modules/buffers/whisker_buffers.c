/**
 * @author      : ElGatoPanzon
 * @file        : whisker_buffers
 * @created     : Thursday Mar 26, 2026 20:32:23 CST
 * @description : ECS buffer module -- rental-based contiguous buffer allocation
 */

#include "whisker_buffers.h"
#include "whisker_serialisation.h"

void w_buffers_init(struct w_ecs_world *world)
{
	/* ensure meta component type exists */
	w_ecs_get_component_by_name(world, W_BUFFER_META_COMPONENT_NAME);
}

void w_buffers_free(struct w_ecs_world *world)
{
	(void)world;
}

w_pack32x2 w_buffer_create(struct w_ecs_world *world, char *name,
	uint type_id, size_t type_size, uint32_t count)
{
	if (!name || type_size == 0 || count == 0) return W_BUFFER_HANDLE_INVALID;

	/* get or create the component entity for this buffer name */
	w_entity_id buffer_id = w_ecs_get_component_by_name(world, name);

	/* set metadata if first time */
	w_entity_id meta_comp = w_ecs_get_component_by_name(world, W_BUFFER_META_COMPONENT_NAME);
	if (!w_ecs_get_component_(world, meta_comp, buffer_id))
	{
		struct w_buffer_meta meta = {
			.type_id = type_id,
			.type_size = type_size,
		};
		w_ecs_set_component_(world, 0, meta_comp, buffer_id,
			(void *)&meta, sizeof(struct w_buffer_meta));

		/* tag this component for ID-based serialisation */
		w_ecs_set_tag_str(world, WM_SERIALISATION_SERIALISE_AS_ID_TAG_NAME, buffer_id);
	}

	/* ensure the component entry exists */
	struct w_component_entry *entry = w_component_registry_get_entry(
		&world->components, buffer_id);
	uint64_t offset = 0;

	if (!entry)
	{
		/* bootstrap entry by setting a zero value at the last index
		 * this creates the component entry with correct type info */
		unsigned char *zero = w_mem_xcalloc(1, type_size);
		w_ecs_set_component_(world, type_id, buffer_id,
			(w_entity_id)(count - 1), zero, type_size);
		free(zero);
		entry = w_component_registry_get_entry(&world->components, buffer_id);
	}
	else
	{
		/* find contiguous free range in the existing buffer */
		uint64_t data_slots = entry->data_size / type_size;
		uint64_t max_search = data_slots + count;
		offset = w_sparse_bitset_find_contiguous_clear(
			&entry->data_bitset, count, max_search);
		if (offset == UINT64_MAX) return W_BUFFER_HANDLE_INVALID;
	}

	/* grow data array to fit the full range */
	uint64_t needed_bytes = (offset + count) * type_size;
	w_array_ensure_alloc(entry->data, needed_bytes);

	/* reserve the full contiguous range in the bitset */
	w_sparse_bitset_set_contiguous_range(&entry->data_bitset, offset, count);

	/* zero-initialize the data range */
	memset(entry->data + offset * type_size, 0, (size_t)(count * type_size));

	return (w_pack32x2){ .left = (uint32_t)offset, .right = count };
}

void w_buffer_return(struct w_ecs_world *world, char *name, w_pack32x2 handle)
{
	w_entity_id buffer_id = w_ecs_get_component_by_name(world, name);
	struct w_component_entry *entry = w_component_registry_get_entry(
		&world->components, buffer_id);
	if (!entry) return;

	uint32_t offset = W_BUFFER_HANDLE_OFFSET(handle);
	uint32_t length = W_BUFFER_HANDLE_LENGTH(handle);

	w_sparse_bitset_clear_contiguous_range(&entry->data_bitset, offset, length);
}

void w_buffer_destroy(struct w_ecs_world *world, char *name)
{
	w_entity_id buffer_id = w_ecs_get_component_by_name(world, name);

	/* remove metadata component */
	w_entity_id meta_comp = w_ecs_get_component_by_name(world, W_BUFFER_META_COMPONENT_NAME);
	w_ecs_remove_component_(world, meta_comp, buffer_id);

	/* free the component entry (data array + bitset) */
	struct w_component_entry *entry = w_component_registry_get_entry(
		&world->components, buffer_id);
	if (entry)
	{
		w_sparse_bitset_free(&entry->data_bitset);
		free_null(entry->data);
		entry->data_length = 0;
		entry->data_size = 0;
		entry->type_size = 0;
		w_sparse_bitset_clear(&world->components.entries_bitset, buffer_id);
	}
}

void *w_buffer_get_ptr(struct w_ecs_world *world, char *name, w_pack32x2 handle)
{
	w_entity_id buffer_id = w_ecs_get_component_by_name(world, name);
	struct w_component_entry *entry = w_component_registry_get_entry(
		&world->components, buffer_id);
	if (!entry) return NULL;

	uint32_t offset = W_BUFFER_HANDLE_OFFSET(handle);
	return entry->data + offset * entry->type_size;
}

uint32_t w_buffer_get_length(w_pack32x2 handle)
{
	return W_BUFFER_HANDLE_LENGTH(handle);
}

struct w_component_entry *w_buffer_get_entry(struct w_ecs_world *world, char *name)
{
	w_entity_id buffer_id = w_ecs_get_component_by_name(world, name);
	return w_component_registry_get_entry(&world->components, buffer_id);
}

struct w_buffer_meta *w_buffer_get_meta(struct w_ecs_world *world, char *name)
{
	w_entity_id buffer_id = w_ecs_get_component_by_name(world, name);
	w_entity_id meta_comp = w_ecs_get_component_by_name(world, W_BUFFER_META_COMPONENT_NAME);
	return (struct w_buffer_meta *)w_ecs_get_component_(world, meta_comp, buffer_id);
}
