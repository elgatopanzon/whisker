/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_iterator
 * @created     : Tuesday Apr 01, 2025 19:50:48 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"

/*************************
*  iterator functions   *
*************************/
static bool itor_is_iteration_active(whisker_ecs_system_iterator *itor);
static void itor_increment_cursor(whisker_ecs_system_iterator *itor);
static void itor_set_master_components(whisker_ecs_system_iterator *itor);
static int itor_find_and_set_cursor_components(whisker_ecs_system_iterator *itor);
static void itor_update_master_index(whisker_ecs_system_iterator *itor);

// create and init an iterator instance
whisker_ecs_system_iterator *whisker_ecs_create_iterator()
{
	whisker_ecs_system_iterator *itor_new = whisker_mem_xcalloc_t(1, *itor_new);

	// create sparse sets for component pointers
	whisker_arr_init_t(itor_new->component_arrays, 1);
	whisker_arr_init_t(itor_new->component_arrays_cursors, 1);

	return itor_new;
}

// free instance of iterator and all data
void whisker_ecs_free_iterator(whisker_ecs_system_iterator *itor)
{
	free_null(itor->component_ids_rw);
	free_null(itor->component_ids_w);
	free_null(itor->component_ids_opt);
	free_null(itor->component_arrays);
	free_null(itor->component_arrays_cursors);
}

// get an iterator instance with the given itor_index
// note: this will init the iterator if one does not exist at the index
whisker_ecs_system_iterator *whisker_ecs_query(whisker_ecs_system_context *context, size_t itor_index, char *read_components, char *write_components, char *optional_components)
{
	whisker_ecs_system_iterator *itor;

	// check if iterator index is set
	if (!whisker_ss_contains(context->iterators, itor_index))
	{
		itor = whisker_ecs_create_iterator();
		whisker_ss_set(context->iterators, itor_index, itor);

		free(itor);
		itor = whisker_ss_get(context->iterators, itor_index);
		whisker_ecs_init_iterator(context, itor, read_components, write_components, optional_components);
	}

	itor = whisker_ss_get(context->iterators, itor_index);
	if (itor == NULL)
	{
		// TODO: panic here
		return NULL;
	}

	// reset iterator state
	itor->master_index = UINT64_MAX;
	itor->cursor = UINT64_MAX;
	itor->cursor_max = 0;
	itor->count = 0;
	itor->entity_id = whisker_ecs_entity_id_from_raw(UINT64_MAX);

	// find the master iterator by finding the smallest set
	for (int i = 0; i < itor->component_ids_rw_length; ++i)
	{
		whisker_sparse_set *component_array;
		if (itor->component_arrays[i] == NULL)
		{
			component_array = whisker_ecs_get_component_array(context->world, itor->component_ids_rw[i]);
			if (component_array == NULL)
			{
				itor->master_index = UINT64_MAX;
				break;
			}

			itor->component_arrays[i] = component_array;
		}
		else
		{
			component_array = itor->component_arrays[i];
		}

		if (component_array->sparse_index_length < itor->count || itor->count == 0)
		{
			itor->count = component_array->sparse_index_length;
			itor->cursor_max = itor->count;
			itor->master_index = i;
		}
	}

	/* debug_printf("ecs:sys:itor master selected: %zu count %zu components %zu\n", itor->master_index, itor->count, itor->component_ids->length); */

	// try to cache optional components when they are NULL
	for (int i = 0; i < itor->component_ids_opt_length; ++i)
	{
		whisker_sparse_set *component_array;
		size_t array_offset = itor->component_ids_rw_length + i;
		if (itor->component_arrays[array_offset] == NULL)
		{
			component_array = whisker_ecs_get_component_array(context->world, itor->component_ids_opt[i]);
			if (component_array == NULL)
			{
				continue;
			}

			itor->component_arrays[array_offset] = component_array;
		}
		itor->component_arrays_cursors[i] = 0;
	}

	// calculate cursor start and end point from thread context
	if (context->thread_max > 1)
	{
    	size_t context_thread_max = context->thread_max;
    	size_t thread_context_chunk_size = itor->count / context_thread_max;
    	itor->cursor = (context->thread_id * thread_context_chunk_size) - 1;
    	itor->cursor_max = (context->thread_id == context_thread_max - 1) ? itor->count : (context->thread_id * thread_context_chunk_size) + thread_context_chunk_size;

    	if (itor->cursor_max == itor->cursor + 1)
    	{
    		itor->count = 0;
    		itor->master_index = UINT64_MAX;
    		return itor;
    	}

    	/* if (context->thread_max > 1) { */
        /* 	debug_printf("itor: thread stats system %zu [t:%zu of %zu][cs:%zu m:%zu][c:%zu-%zu]\n", context->system_entity_id.id, context->thread_id + 1, context_thread_max, thread_context_chunk_size, itor->count, itor->cursor + 1, itor->cursor_max); */
    	/* } */
	}

	// if thread_max is uint64_max, assume we don't want to process anything
	// note: this is a hack added to allow an iterator to complete an
	// initialisation but ignore any matched entities
	else if (context->thread_max == UINT64_MAX)
	{
		itor->count = 0;
	}

	return itor;
}

// init the provided iterator and cache the given components
void whisker_ecs_init_iterator(whisker_ecs_system_context *context, whisker_ecs_system_iterator *itor, char *read_components, char *write_components, char *optional_components)
{
	itor->world = context->world;

	// convert read and write component names to component sparse sets
	char *combined_components;
	combined_components = whisker_mem_xmalloc(strlen(read_components) + strlen(write_components) + 2);
	if (combined_components == NULL)
	{
		return;
	}

	strcpy(combined_components, read_components);
	if (strlen(write_components) > 0)
	{
		strcat(combined_components, ",");
		strcat(combined_components, write_components);
	}

	/* debug_printf("ecs:sys:itor init: read: %s write: %s\n", read_components, write_components); */
	/* debug_printf("ecs:sys:itor init: combined: %s\n", combined_components); */

	// rw components include read and write component IDs
	if (itor->component_ids_rw == NULL)
	{
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_batch_create_named_entities(context->world, combined_components);
		itor->component_ids_rw = e_arr->arr;
		itor->component_ids_rw_length = e_arr->arr_length;
		itor->component_ids_rw_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_rw == NULL)
		{
			// TODO: panic here
			free(combined_components);
			return;
		}

		whisker_arr_ensure_alloc(itor->component_arrays, itor->component_ids_rw_length);
		whisker_arr_ensure_alloc(itor->component_arrays_cursors, itor->component_ids_rw_length);
	}
	free(combined_components);

	// w components only includes write component IDs
	// note: these do not resize the main component arrays
	if (itor->component_ids_w == NULL)
	{
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_batch_create_named_entities(context->world, write_components);
		itor->component_ids_w = e_arr->arr;
		itor->component_ids_w_length = e_arr->arr_length;
		itor->component_ids_w_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_w == NULL)
		{
			// TODO: panic here
			return;
		}
	}

	// opt components only include optional component IDs
	// note: these belong at the end of the component arrays
	if (itor->component_ids_opt == NULL)
	{
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_batch_create_named_entities(context->world, optional_components);
		itor->component_ids_opt = e_arr->arr;
		itor->component_ids_opt_length = e_arr->arr_length;
		itor->component_ids_opt_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_opt == NULL)
		{
			// TODO: panic here
			return;
		}
		whisker_arr_ensure_alloc(itor->component_arrays, (itor->component_ids_rw_length + itor->component_ids_opt_length));
		whisker_arr_ensure_alloc(itor->component_arrays_cursors, (itor->component_ids_rw_length + itor->component_ids_opt_length));
	}
}


// step through an iterator incrementing the cursor
// note: returns false to indicate the iteration has reached the end
bool whisker_ecs_iterate(whisker_ecs_system_iterator *itor) 
{
    if (!itor_is_iteration_active(itor)) { return false; }

    itor_increment_cursor(itor);
    itor_set_master_components(itor);

    int cursor_state = itor_find_and_set_cursor_components(itor);
    itor_update_master_index(itor);

    return cursor_state == 0;
}

static bool itor_is_iteration_active(whisker_ecs_system_iterator *itor) {
    return (itor->master_index != UINT64_MAX && itor->count > 0);
}

static void itor_increment_cursor(whisker_ecs_system_iterator *itor) {
    itor->cursor++;
}

static void itor_set_master_components(whisker_ecs_system_iterator *itor) {
	while (itor->cursor < itor->cursor_max) 
	{
    	whisker_sparse_set *master_set = itor->component_arrays[itor->master_index];
    	itor->entity_id.id = master_set->sparse_index[itor->cursor];

		// skip entities marked as destroyed
		// note: if an entity is marked destroyed but still has components, then it's
		// an entity with destroyed state managed externally
        if (whisker_ecs_get_entity(itor->world, itor->entity_id)->unmanaged)
        {
        	itor_increment_cursor(itor);
        	continue;
        }

        break;
	}

}

static int itor_find_and_set_cursor_components(whisker_ecs_system_iterator *itor) {
    int cursor_state = 0;
    size_t rw_length = itor->component_ids_rw_length;
    for (int ci = 0; ci < rw_length; ++ci) {
        whisker_sparse_set *set = itor->component_arrays[ci];
        whisker_ecs_entity_id_raw cursor_entity = set->sparse_index[itor->cursor];


        if (cursor_entity == itor->entity_id.id) {
            itor->component_arrays_cursors[ci] = itor->cursor;
            cursor_state = 0;
            continue;
        }

        cursor_state = -1;
        size_t set_length = set->sparse_index_length;
        for (int i = itor->cursor; i < set_length; ++i) {
            cursor_entity = set->sparse_index[i];

            if (cursor_entity == itor->entity_id.id) {
            	itor->component_arrays_cursors[ci] = i;
                cursor_state = 0;
                break;
            }

            if (cursor_entity > itor->entity_id.id) {
                cursor_state = 1;
                break;
            }
        }

        if (cursor_state != 0) {
            break;
        }
    }
    return cursor_state;
}

static void itor_update_master_index(whisker_ecs_system_iterator *itor) {
    if (itor->cursor + 1 >= itor->cursor_max) {
        itor->master_index = UINT64_MAX;
    }
}
