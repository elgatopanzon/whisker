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

	*systems = s;

	return E_WHISKER_ECS_SYS_OK;
}

void whisker_ecs_s_free_systems(whisker_ecs_systems *systems)
{
	for (int i = 0; i < warr_length(systems->systems); ++i)
	{
		whisker_ecs_s_free_system(&systems->systems[i]);
	}

	warr_free(systems->systems);

	free(systems);
}

whisker_ecs_system* whisker_ecs_s_register_system(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system system)
{
	// init component name dict
	wdict_create(&system.component_name_index, int, 0);

	// create array for cached component IDs
	warr_create(whisker_ecs_entity_id*, 0, &system.components_cache_entities);

	// add system to main systems list
	warr_push(&systems->systems, &system);

	return &systems->systems[warr_length(systems->systems) - 1];
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
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time)
{
	// matches, excluding updates for those which don't match
	size_t systems_count = warr_length(systems->systems);

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
