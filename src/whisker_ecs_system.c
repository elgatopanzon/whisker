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
	whisker_ecs_s_set_archetype_components(systems, components, &system);

	// init component name trie
	// TODO: better error checking here and return the pointer to the system
	// instead
	wdict_create(&system.component_name_index, int, 0);
	warr_create(whisker_ecs_entity_id*, 0, &system.custom_archetypes);
	warr_create(whisker_ecs_entity_id*, 0, &system.components_cache_archetypes);
	warr_create(whisker_ecs_entity_id*, 0, &system.archetype_entities);
	warr_push(&systems->systems, &system);

	return &systems->systems[warr_length(systems->systems) - 1];
}

E_WHISKER_ECS_SYS whisker_ecs_s_set_archetype_components(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system *system)
{
	if (system->components_cache == NULL)
	{
		whisker_ecs_c_create_components(&system->components_cache);
	}

	return E_WHISKER_ECS_SYS_OK;
}

void whisker_ecs_s_free_system(whisker_ecs_system *system)
{
	if (system->read_archetype != NULL)
	{
		warr_free(system->read_archetype);
	}
	if (system->write_archetype != NULL)
	{
		warr_free(system->write_archetype);
	}
	if (system->components_cache != NULL)
	{
		warr_free(system->components_cache->components);
		free(system->components_cache);
	}
	if (system->components_cache_archetypes != NULL)
	{
		warr_free(system->components_cache_archetypes);
	}
	if (system->component_name_index != NULL)
	{
		wdict_free(system->component_name_index);
	}

	if (system->read_component_names != NULL)
	{
		wstr_free(system->read_component_names);
	}
	if (system->write_component_names != NULL)
	{
		wstr_free(system->write_component_names);
	}

	if (system->custom_archetypes != NULL)
	{
		for (int i = 0; i < warr_length(system->custom_archetypes); ++i)
		{
			warr_free(system->custom_archetypes[i]);
		}
		warr_free(system->custom_archetypes);
	}
	if (system->archetype_entities != NULL)
	{
		warr_free(system->archetype_entities);
	}
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time)
{
	// iterate over all systems, processing each entity and it's archetype for
	// matches, excluding updates for those which don't match
	size_t systems_count = warr_length(systems->systems);

	for (size_t si = 0; si < systems_count; ++si)
	{
		whisker_ecs_system *system = &systems->systems[si];
		system->delta_time = delta_time;
		size_t entity_count = warr_length(system->archetype_entities);

		for (size_t ei = 0; ei < entity_count; ++ei)
		{
			whisker_ecs_entity_id e = system->archetype_entities[ei];
			/* if (!entities->entities[e.index].alive) */
			/* { */
			/* 	continue; */
			/* } */

			/* whisker_ecs_entity_id *entity_archetype = entities->entities[e.index].archetype; */
			/* if (whisker_ecs_a_match(system->read_archetype, entity_archetype)) */
			/* { */
				whisker_ecs_s_update_system(system, entities, system->components_cache, e);				
			/* } */
		}
	}

	return E_WHISKER_ECS_SYS_OK;
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_system(whisker_ecs_system *system, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id entity_id)
{
	system->system_ptr(system);

	return E_WHISKER_ECS_SYS_OK;
}

/* E_WHISKER_ECS_SYS whisker_ecs_s_sync_system_archetype_entities(whisker_ecs_systems *systems, whisker_ecs_entities *entities) */
/* { */
/* 	// check for changed entity archetypes */
/* 	whisker_ecs_entity_archetype_change change; */
/* 	while (warr_pop(&entities->archetype_changes, &change) == E_WHISKER_ARR_OK)  */
/* 	{ */
/* 		#<{(| debug_printf("ecs:sys:syncing changes: e=%zu a=%zu t=%d\n", change.entity_id.id, change.archetype_id.id, change.change_type); |)}># */
/*  */
/* 		bool entity_alive = entities->entities[change.entity_id.index].alive; */
/*  */
/* 		for (int si = 0; si < warr_length(systems->systems); ++si) */
/* 		{ */
/* 			whisker_ecs_system *system = &systems->systems[si]; */
/*  */
/* 			#<{(| debug_printf("ecs:sys:syncing changes: archetype %zu in system %zu (alive: %d)\n", change.archetype_id.id, system->entity_id.id, entity_alive); |)}># */
/*  */
/* 			// determine if we need to set it or unset it in the system's */
/* 			// archetype */
/* 			// -1 = nothing */
/* 			// 0 = unset */
/* 			// 1 = set */
/* 			int action = (!entity_alive || !whisker_ecs_a_match(system->read_archetype, entities->entities[change.entity_id.index].archetype)) ? 0 : 1; */
/*  */
/* 			#<{(| debug_printf("ecs:sys:syncing archetype changes: entity %zu archetype %zu in system %zu (action: %d)\n", change.entity_id.id, change.archetype_id.id, system->entity_id.id, action); |)}># */
/*  */
/* 			if (action == 0) */
/* 			{ */
/* 				whisker_ecs_a_remove(&system->archetype_entities, change.entity_id); */
/* 			} */
/* 			else if (action == 1) */
/* 			{ */
/* 				whisker_ecs_a_set(&system->archetype_entities, change.entity_id); */
/* 			} */
/* 		} */
/* 	} */
/* 	 */
/* 	return E_WHISKER_ECS_SYS_OK; */
/* } */

whisker_ecs_entity_id *whisker_ecs_s_get_custom_archetype(whisker_ecs_system *system, int index)
{
	return system->custom_archetypes[index];
}
whisker_ecs_entity_id *whisker_ecs_s_set_custom_archetype(whisker_ecs_system *system, int index, whisker_ecs_entity_id *archetype)
{
	debug_printf("setting system custom archetype %d %d\n", system->entity_id.index, index);

	warr_resize(&system->custom_archetypes, index + 1);
	system->custom_archetypes[index] = archetype;
	return archetype;
}

void *whisker_ecs_s_get_component_by_name_or_index(whisker_ecs_system *system, char *name, int index, size_t size, whisker_ecs_entity_id entity_id, bool read_or_write, bool init, bool init_archetype)
{
	// no longer supporting negative index to trigger manual name lookup since
	// the names are now usually empty anyway
	if (index == -1)
	{
		return NULL;
	}

	// if specifically requesting init, then init the cache
	if (init)
	{
		E_WHISKER_ECS_SYS init_err = whisker_ecs_s_init_component_cache(system, name, index, size, read_or_write, init_archetype);
		if (init_err != E_WHISKER_ECS_SYS_OK)
		{
			return NULL;
		}
	}

	// return NULL for a 0 size component
	if (size == 0)
	{
		return NULL;
	}

	return whisker_ecs_s_get_component(system, index, size, entity_id, read_or_write);
}

E_WHISKER_ECS_SYS whisker_ecs_s_init_component_cache(whisker_ecs_system *system, char *name, int index, size_t size, bool read_or_write, bool update_archetype)
{
	debug_printf("sys %zu:init component cache %s index %d\n", system->entity_id.id, name, index);

	// get the system's components cache
	whisker_ecs_components *components = system->components_cache;

	// set the correct archetype based on read/write
	whisker_ecs_entity_id *archetype;
	if (read_or_write == 0)
	{
		archetype = system->read_archetype;
	}
	else
	{
		archetype = system->write_archetype;
	}

	// get the component ID and set the index in the components cache archetypes
	whisker_ecs_entity_id component_id = whisker_ecs_e_create_named(system->entities, name);

	if (update_archetype)
	{
		whisker_ecs_a_set(&archetype, component_id);
	}

	// ensure the size of the cache archetypes can fit the index
	warr_resize(&system->components_cache_archetypes, index + 1);
	system->components_cache_archetypes[index] = component_id;

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
		whisker_ecs_entity_id archetype_id = system->components_cache_archetypes[index];

		whisker_sparse_set *component_array;
		whisker_ecs_c_get_component_array(system->components, archetype_id, size, &component_array);

		system->components_cache->components[index] = component_array;
	}

	// set the archetype array back on the system
	if (read_or_write == 0)
	{
		system->read_archetype = archetype;
	}
	else
	{
		system->write_archetype = archetype;
	}
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
