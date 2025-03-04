/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:56 CST
 */

#include "whisker_std.h"
#include "whisker_debug.h"
#include "whisker_array.h"
#include "whisker_string.h"
#include "whisker_dict.h"

#include "whisker_ecs_system.h"

E_WHISKER_ECS_SYS whisker_ecs_s_create_systems(whisker_ecs_systems **systems)
{
	whisker_ecs_systems *s = calloc(1, sizeof(*s));
	if (s == NULL)
	{
		return E_WHISKER_ECS_SYS_MEM;
	}

	if (warr_create(whisker_ecs_system, 0, &s->systems) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_SYS_ARR;
	s->systems_length = 0;

	*systems = s;

	return E_WHISKER_ECS_SYS_OK;
}

void whisker_ecs_s_free_systems(whisker_ecs_systems *systems)
{
	for (int i = 0; i < systems->systems_length; ++i)
	{
		whisker_ecs_s_free_system(&systems->systems[i]);
	}

	warr_free(systems->systems);

	free(systems);
}

whisker_ecs_system* whisker_ecs_s_register_system(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system system)
{
	// init component name dict
	E_WHISKER_DICT name_idx_err = wdict_create(&system.component_name_index, int, 0);
	if (name_idx_err != E_WHISKER_DICT_OK)
	{
		return NULL;
	}

	// create array for cached component IDs
	E_WHISKER_ARR cc_err = warr_create(whisker_ecs_entity_id*, 0, &system.components_cache_entities);
	if (cc_err != E_WHISKER_ARR_OK)
	{
		return NULL;
	}

	// create iterators sparse set
	E_WHISKER_SS itors_err = whisker_ss_create_t(&system.iterators, whisker_ecs_iterator);
	if (itors_err != E_WHISKER_SS_OK)
	{
		return NULL;
	}

	// add system to main systems list
	warr_push(&systems->systems, &system);
	systems->systems_length++;

	return &systems->systems[systems->systems_length - 1];
}

void whisker_ecs_s_free_system(whisker_ecs_system *system)
{
	if (system->components_cache != NULL)
	{
		warr_free(system->components_cache->components);
		free(system->components_cache);
	}
	if (system->components_cache_entities != NULL)
	{
		warr_free(system->components_cache_entities);
	}
	if (system->component_name_index != NULL)
	{
		wdict_free(system->component_name_index);
	}

	if (system->iterators != NULL)
	{
		for (int i = 0; i < system->iterators->sparse_index->length; ++i)
		{
			whisker_ecs_iterator itor = ((whisker_ecs_iterator*)system->iterators->dense)[i];
			if (itor.read != NULL)
			{
				whisker_arr_free_void_(itor.read);
			}
			if (itor.write != NULL)
			{
				whisker_arr_free_void_(itor.write);
			}
			if (itor.opt != NULL)
			{
				whisker_arr_free_void_(itor.opt);
			}

			if (itor.component_ids_rw != NULL)
			{
				whisker_arr_free_whisker_ecs_entity_id(itor.component_ids_rw);
			}
			if (itor.component_ids_w != NULL)
			{
				whisker_arr_free_whisker_ecs_entity_id(itor.component_ids_w);
			}
			if (itor.component_ids_opt != NULL)
			{
				whisker_arr_free_whisker_ecs_entity_id(itor.component_ids_opt);
			}
			if (itor.component_arrays != NULL)
			{
				whisker_arr_free_void_(itor.component_arrays);
			}
		}

		whisker_ss_free(system->iterators);
	}
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time)
{
	// matches, excluding updates for those which don't match
	size_t systems_count = systems->systems_length;

	for (size_t si = 0; si < systems_count; ++si)
	{
		whisker_ecs_system *system = &systems->systems[si];
		system->delta_time = delta_time;

		whisker_ecs_s_update_system(system);				
	}

	return E_WHISKER_ECS_SYS_OK;
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_system(whisker_ecs_system *system)
{
	system->system_ptr(system);

	return E_WHISKER_ECS_SYS_OK;
}

/* void *whisker_ecs_s_get_component_by_name_or_index(whisker_ecs_system *system, char *name, int index, size_t size, whisker_ecs_entity_id entity_id, bool read_or_write, bool init, bool init_archetype) */
/* { */
/* 	// no longer supporting negative index to trigger manual name lookup since */
/* 	// the names are now usually empty anyway */
/* 	if (index == -1) */
/* 	{ */
/* 		return NULL; */
/* 	} */
/*  */
/* 	// if specifically requesting init, then init the cache */
/* 	if (init) */
/* 	{ */
/* 		E_WHISKER_ECS_SYS init_err = whisker_ecs_s_init_component_cache(system, name, index, size, read_or_write, init_archetype); */
/* 		if (init_err != E_WHISKER_ECS_SYS_OK) */
/* 		{ */
/* 			return NULL; */
/* 		} */
/* 	} */
/*  */
/* 	// return NULL for a 0 size component */
/* 	if (size == 0) */
/* 	{ */
/* 		return NULL; */
/* 	} */
/*  */
/* 	return whisker_ecs_s_get_component(system, index, size, entity_id, read_or_write); */
/* } */

E_WHISKER_ECS_SYS whisker_ecs_s_init_component_cache(whisker_ecs_system *system, char *name, int index, size_t size, bool read_or_write)
{
	debug_printf("sys %zu:init component cache %s index %d\n", system->entity_id.id, name, index);

	// get the system's components cache
	whisker_ecs_components *components = system->components_cache;

	/* // set the correct archetype based on read/write */
	/* whisker_ecs_entity_id *archetype; */
	/* if (read_or_write == 0) */
	/* { */
	/* 	archetype = system->read_archetype; */
	/* } */
	/* else */
	/* { */
	/* 	archetype = system->write_archetype; */
	/* } */

	// get the component ID and set the index in the components cache archetypes
	whisker_ecs_entity_id component_id = whisker_ecs_e_create_named(system->entities, name);

	/* if (update_archetype) */
	/* { */
	/* 	whisker_ecs_a_set(&archetype, component_id); */
	/* } */

	// ensure the size of the cache archetypes can fit the index
	warr_resize(&system->components_cache_entities, index + 1);
	system->components_cache_entities[index] = component_id;

	// only care about the components if the size is set
	// note: this allows 0 size components to act as tags, purely an
	// archetype
	if (size > 0)
	{
		// grow components to index size if required
		E_WHISKER_ECS_COMP grow_err = whisker_ecs_c_grow_components_(components, index + 1);
		if (grow_err != E_WHISKER_ECS_COMP_OK)
		{
			return E_WHISKER_ECS_SYS_ARR;
		}

		// cache the component array
		whisker_ecs_entity_id archetype_id = system->components_cache_entities[index];

		whisker_sparse_set *component_array;
		whisker_ecs_c_get_component_array(system->components, archetype_id, &component_array);

		system->components_cache->components[index] = component_array;
	}

	/* // set the archetype array back on the system */
	/* if (read_or_write == 0) */
	/* { */
	/* 	system->read_archetype = archetype; */
	/* } */
	/* else */
	/* { */
	/* 	system->write_archetype = archetype; */
	/* } */
}

void *whisker_ecs_s_get_component(whisker_ecs_system *system, size_t index, size_t size, whisker_ecs_entity_id entity_id, bool read_or_write)
{
	// note: this will crash if the array for this index hasn't been initialised
	return wss_get(system->components_cache->components[index], entity_id.index);
}

int whisker_ecs_s_get_component_name_index(whisker_ecs_system *system, char* component_names, char* component_name) {
	int *component_name_index = whisker_dict_get_strk(system->component_name_index, component_name);

	if (component_name_index == NULL)
	{
    	size_t search_index = 0;
    	int name_index = 0;
    	size_t name_length = strlen(component_name);

    	while (component_names[search_index]) {
        	if (memcmp(component_names + search_index, component_name, name_length) == 0 &&
            	(component_names[search_index + name_length] == ',' || component_names[search_index + name_length] == '\0')) {

				// malloc value for the index and set the trie value
				// this should trigger the trie search next time
            	whisker_dict_set_strk(&system->component_name_index, component_name, &name_index);

            	return name_index;
        	}
        	while (component_names[search_index] && component_names[search_index] != ',') {
            	search_index++;
        	}
        	if (component_names[search_index] == ',') {
            	search_index++;
        	}
        	name_index++;
    	}
	}
	else
	{
		return *component_name_index;
	}

    return -1;
}

/*************************
*  iterator functions   *
*************************/
E_WHISKER_ECS_SYS whisker_ecs_s_create_iterator(whisker_ecs_iterator **itor)
{
	whisker_ecs_iterator *itor_new;
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(*itor_new), (void**)&itor_new);
	if (err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_ECS_SYS_MEM;
	}

	// create sparse sets for component pointers
	whisker_arr_create_void_(&itor_new->read, 0);
	whisker_arr_create_void_(&itor_new->write, 0);
	whisker_arr_create_void_(&itor_new->opt, 0);

	whisker_arr_create_void_(&itor_new->component_arrays, 0);

	*itor = itor_new;

	return E_WHISKER_ECS_SYS_OK;
}

whisker_ecs_iterator *whisker_ecs_s_get_iterator(whisker_ecs_system *system, size_t itor_index, char *read_components, char *write_components, char *optional_components)
{
	whisker_ecs_iterator *itor;

	// check if iterator index is set
	if (!wss_contains(system->iterators, itor_index))
	{
		whisker_ecs_s_create_iterator(&itor);
		wss_set(system->iterators, itor_index, itor);

		free(itor);
		itor = wss_get(system->iterators, itor_index);
		whisker_ecs_s_init_iterator(system, itor, read_components, write_components, optional_components);
	}

	itor = wss_get(system->iterators, itor_index);

	// reset iterator state
	itor->master_index = UINT64_MAX;
	itor->cursor = UINT64_MAX;
	itor->count = 0;
	itor->entity_id = whisker_ecs_e_id(UINT64_MAX);

	// find the master iterator by finding the smallest set
	for (int i = 0; i < itor->component_ids_rw->length; ++i)
	{
		whisker_sparse_set *component_array;
		if (itor->component_arrays->arr[i] == NULL)
		{
			whisker_ecs_c_get_component_array(system->components, itor->component_ids_rw->arr[i], &component_array);
			if (component_array == NULL)
			{
				itor->master_index = UINT64_MAX;
				break;
			}

			itor->component_arrays->arr[i] = component_array;
		}
		else
		{
			component_array = itor->component_arrays->arr[i];
		}

		if (component_array->sparse_index->length < itor->count || itor->count == 0)
		{
			itor->count = component_array->sparse_index->length;
			itor->master_index = i;
		}
	}

	/* debug_printf("ecs:sys:itor master selected: %zu count %zu components %zu\n", itor->master_index, itor->count, itor->component_ids->length); */

	// try to cache optional components when they are NULL
	for (int i = 0; i < itor->component_ids_opt->length; ++i)
	{
		whisker_sparse_set *component_array;
		size_t array_offset = itor->component_ids_rw->length + i;
		if (itor->component_arrays->arr[array_offset] == NULL)
		{
			whisker_ecs_c_get_component_array(system->components, itor->component_ids_opt->arr[i], &component_array);
			if (component_array == NULL)
			{
				continue;
			}

			itor->component_arrays->arr[array_offset] = component_array;
		}
	}

	return itor;
}

E_WHISKER_ECS_SYS whisker_ecs_s_init_iterator(whisker_ecs_system *system, whisker_ecs_iterator *itor, char *read_components, char *write_components, char *optional_components)
{
	// convert read and write component names to component sparse sets
	char *combined_components;
	combined_components = malloc(strlen(read_components) + strlen(write_components) + 2);
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
		itor->component_ids_rw = whisker_ecs_e_from_named_entities(system->entities, combined_components);
		whisker_arr_resize_void_(itor->read, itor->component_ids_rw->length, true);
		whisker_arr_resize_void_(itor->write, itor->component_ids_rw->length, true);
		whisker_arr_resize_void_(itor->component_arrays, itor->component_ids_rw->length, true);
	}

	// w components only includes write component IDs
	// note: these do not resize the main component arrays
	if (itor->component_ids_w == NULL)
	{
		itor->component_ids_w = whisker_ecs_e_from_named_entities(system->entities, write_components);
	}

	// opt components only include optional component IDs
	// note: these belong at the end of the component arrays
	if (itor->component_ids_opt == NULL)
	{
		itor->component_ids_opt = whisker_ecs_e_from_named_entities(system->entities, optional_components);
		whisker_arr_resize_void_(itor->opt, itor->component_ids_opt->length, true);
		whisker_arr_resize_void_(itor->component_arrays, itor->component_ids_rw->length + itor->component_ids_opt->length, true);
	}

	// free combined string
	free(combined_components);

	return E_WHISKER_ECS_SYS_OK;
}

bool whisker_ecs_s_iterate(whisker_ecs_system *system, whisker_ecs_iterator *itor)
{
	// if the master index is invalid then there is nothing to iterate
	bool iteration_active = (itor->master_index != UINT64_MAX);
	if (!iteration_active)
	{
		return iteration_active;
	}

	itor->cursor++;

	// set the master components
	whisker_sparse_set *master_set = itor->component_arrays->arr[itor->master_index];
	whisker_ecs_entity_id master_entity = whisker_ecs_e_id(master_set->sparse_index->arr[itor->cursor]);
	itor->entity_id = master_entity;
	itor->read->arr[itor->master_index] = master_set->dense + (itor->cursor * master_set->element_size);
	itor->write->arr[itor->master_index] = master_set->dense + (itor->cursor * master_set->element_size);

	/* debug_printf("ecs:sys:itor master: index %zu entity %zu cursor [%zu/%zu]\n", itor->master_index, master_entity.id, itor->cursor, itor->count - 1); */
    /*  */
	/* // print out all component entities */
	/* for (int ci = 0; ci < itor->component_arrays->length; ++ci) */
	/* { */
	/* 	whisker_sparse_set *set = itor->component_arrays->arr[ci]; */
	/* 	debug_printf("ecs:sys:itor [%zu/%zu] component %d entities: ", itor->cursor, itor->count - 1, ci); */
	/* 	for (int i = 0; i < set->sparse_index->length; ++i) */
	/* 	{ */
	/* 		debug_printf("%zu ", set->sparse_index->arr[i]); */
	/* 	} */
	/* 	debug_printf("\n"); */
	/* } */

	int cursor_state = 0;
	for (size_t ci = 0; ci < itor->component_arrays->length; ++ci)
	{
    	if (ci == itor->master_index) continue;

    	cursor_state = -1;
    	whisker_sparse_set *set = itor->component_arrays->arr[ci];
    	bool optional = (ci + 1 > itor->component_ids_rw->length);

		// optional only: allow NULL set
    	if (optional)
    	{
        	cursor_state = 0;
    	}
    	if (set == NULL && optional)
    	{
        	continue;
    	}

		// begin cursor iteration
    	for (size_t i = itor->cursor; i < set->sparse_index->length; ++i)
    	{
        	whisker_ecs_entity_id cursor_entity = whisker_ecs_e_id(master_set->sparse_index->arr[i]);

			// reached end of valid entities
        	if (cursor_entity.index > master_entity.index)
        	{
            	cursor_state = optional ? 0 : 1;
            	break;
        	}

        	if (system->entities->entities->length > cursor_entity.index + 1 && system->entities->entities->arr[cursor_entity.index].destroyed) continue;

			// matched entity at cursor
        	if (cursor_entity.index == master_entity.index)
        	{
            	size_t offset = i * set->element_size;
            	if (!optional)
            	{
                	itor->read->arr[ci] = set->dense + offset;
                	itor->write->arr[ci] = set->dense + offset;
            	}
            	else
            	{
                	itor->opt->arr[ci - itor->component_ids_rw->length] = set->dense + offset;
            	}

            	cursor_state = 0;
            	break;
        	}
    	}

    	if (cursor_state != 0) break;
	}

	/* debug_printf("ecs:sys:itor cursor %zu state %d\n", itor->cursor, cursor_state); */

	if (itor->cursor + 1 >= master_set->sparse_index->length)
	{
		/* debug_printf("ecs:sys:itor reached end of master %zu with entity %zu\n", itor->master_index, itor->entity_id); */
		itor->master_index = UINT64_MAX;
	}

	return cursor_state == 0;
}

