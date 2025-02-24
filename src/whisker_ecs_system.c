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
	warr_push(&systems->systems, &system);

	return &systems->systems[warr_length(systems->systems) - 1];
}

E_WHISKER_ECS_SYS whisker_ecs_s_set_archetype_components(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system *system)
{
	// if archetype read/write components array is null, create a new one with just
	// pointers to this system's read/write components
	// note: this will require runtime system archetype changes to reset it
	if (system->read_components == NULL)
	{
		whisker_ecs_c_create_components(&system->read_components);
		whisker_ecs_c_grow_components_(system->read_components, warr_length(system->read_archetype));
	}
	if (system->write_components == NULL)
	{
		whisker_ecs_c_create_components(&system->write_components);
		whisker_ecs_c_grow_components_(system->write_components, warr_length(system->write_archetype));
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
	if (system->read_components != NULL)
	{
		warr_free(system->read_components->components);
		free(system->read_components);
	}
	if (system->write_components != NULL)
	{
		warr_free(system->write_components->components);
		free(system->write_components);
	}
	if (system->component_name_index != NULL)
	{
		wdict_free(system->component_name_index);
	}

	wstr_free(system->read_component_names);
	wstr_free(system->write_component_names);
	for (int i = 0; i < warr_length(system->custom_archetypes); ++i)
	{
		warr_free(system->custom_archetypes[i]);
	}
	warr_free(system->custom_archetypes);
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time)
{
	// iterate over all systems, processing each entity and it's archetype for
	// matches, excluding updates for those which don't match
	size_t entity_count = whisker_ecs_e_count(entities);
	size_t systems_count = warr_length(systems->systems);

	for (size_t si = 0; si < systems_count; ++si)
	{
		whisker_ecs_system *system = &systems->systems[si];
		system->delta_time = delta_time;

		for (size_t ei = 0; ei < entity_count; ++ei)
		{
			if (!entities->entities[ei].alive)
			{
				continue;
			}

			whisker_ecs_entity_id *entity_archetype = entities->entities[ei].archetype;
			whisker_ecs_entity_id entity_id = entities->entities[ei].id;

			if (whisker_ecs_a_match(system->read_archetype, entity_archetype))
			{
				whisker_ecs_s_update_system(system, entities, system->write_components, entity_id);				
			}
		}
	}

	return E_WHISKER_ECS_SYS_OK;
}

E_WHISKER_ECS_SYS whisker_ecs_s_update_system(whisker_ecs_system *system, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id entity_id)
{
	system->system_ptr((whisker_ecs_system_update) {
		.entities = entities,
		.components = components,
		.system = system,
		.entity_id = entity_id,
		});

	return E_WHISKER_ECS_SYS_OK;
}

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

void *whisker_ecs_s_get_component_by_name_or_index(whisker_ecs_system *system, char *name, int index, size_t size, whisker_ecs_entity_id entity_id, bool read_or_write)
{
	whisker_ecs_components *components;
	whisker_ecs_entity_id *archetype;
	char *component_names;
	if (read_or_write == 0)
	{
		components = system->read_components;
		archetype = system->read_archetype;
		component_names = system->read_component_names;
	}
	else
	{
		components = system->write_components;
		archetype = system->write_archetype;
		component_names = system->write_component_names;
	}

	// if the index is set, get the component directly from that index
	// note: needs to also resize and set the archetype
	if (index > -1)
	{
		// only care about the components if the size is set
		// note: this allows 0 size components to act as tags, purely an
		// archetype
		if (size > 0)
		{
			// grow components to index size if required
			E_WHISKER_ECS_COMP grow_err = whisker_ecs_c_grow_components_(components, index + 1);
			if (grow_err != E_WHISKER_ECS_COMP_OK)
			{
				return NULL;
			}
		}

		// grow and set archetype index manually
		if (warr_length(archetype) < index + 1)
		{
			warr_resize(&archetype, index + 1);

			if (read_or_write == 0)
			{
				system->read_archetype = archetype;
			}
			else
			{
				system->write_archetype = archetype;
			}
		}

		// set archetype index if it's set to 0
		// note: this might conflict with the idea that 0 == wildcard
		if (archetype[index].index == 0)
		{
			archetype[index] = whisker_ecs_e_create_named(system->entities, name);
		}
	}

	// if the index is -1, then perform the full name lookup
	else
	{
		int name_index = whisker_ecs_s_get_component_name_index(system, component_names, name);
		if (name_index == -1) {
			whisker_ecs_s_add_component_name_archetype(system, name, read_or_write);
			name_index = whisker_ecs_s_get_component_name_index(system, component_names, name);
		}

		index = name_index;
	}

	// return NULL for a 0 size component
	if (size == 0)
	{
		return NULL;
	}

	return whisker_ecs_s_get_component(system, index, size, entity_id, read_or_write);
}

E_WHISKER_ECS_SYS whisker_ecs_s_add_component_name_archetype(whisker_ecs_system *system, char *component_name, bool read_or_write)
{
	char* old_names;
	whisker_ecs_entity_id *archetype;
	whisker_ecs_components *components;

	if (read_or_write == 0)
	{
		old_names = system->read_component_names;
		archetype = system->read_archetype;
		components = system->read_components;
	}
	else
	{
		old_names = system->write_component_names;
		archetype = system->write_archetype;
		components = system->write_components;
	}

	// append new component name
	char *new_names = old_names;
	wstr_join(",", &new_names, component_name, NULL);
	wstr_free(old_names);

	if (read_or_write == 0)
		system->read_component_names = new_names;
	else
		system->write_component_names = new_names;

	// update archetype from new names
	warr_free(archetype);
	archetype = whisker_ecs_e_from_named_entities(system->entities, new_names);

	if (read_or_write == 0)
		system->read_archetype = archetype;
	else
		system->write_archetype = archetype;

	// grow components to new archetype size if required
	E_WHISKER_ECS_COMP grow_err = whisker_ecs_c_grow_components_(components, warr_length(archetype));
	if (grow_err != E_WHISKER_ECS_COMP_OK)
	{
		return E_WHISKER_ECS_SYS_ARR;
	}

	return E_WHISKER_ECS_SYS_OK;
}

void *whisker_ecs_s_get_component(whisker_ecs_system *system, size_t index, size_t size, whisker_ecs_entity_id entity_id, bool read_or_write)
{
	whisker_ecs_components *components;
	whisker_ecs_entity_id *archetype;
	if (read_or_write == 0)
	{
		components = system->read_components;
		archetype = system->read_archetype;
	}
	else
	{
		components = system->write_components;
		archetype = system->write_archetype;
	}

	// verify if the components array exists in the cache
	// if it's NULL, set the pointer to the real component array
	if (components->components[index] == NULL)
	{
		whisker_ecs_entity_id archetype_id = archetype[index];

		whisker_block_array *component_array;
		whisker_ecs_c_get_component_array(system->components, archetype_id, size, (void**)&component_array);
		components->components[index] = component_array;
	}

	return wbarr_get(components->components[index], entity_id.index);
}

int whisker_ecs_s_get_component_name_index(whisker_ecs_system *system, char* component_names, char* component_name) {
	int *component_name_index = wdict_get(system->component_name_index, component_name);

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
            	wdict_set(&system->component_name_index, component_name, &name_index);

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
