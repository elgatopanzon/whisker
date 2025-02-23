/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:29 CST
 */

#include "whisker_std.h"

#include "whisker_debug.h"
#include "whisker_array.h"
#include "whisker_block_array.h"
#include "whisker_string.h"
#include "whisker_ecs.h"

E_WHISKER_ECS whisker_ecs_create(whisker_ecs **ecs)
{
	whisker_ecs *new = calloc(1, sizeof(*new));
	if (new == NULL)
	{
		return E_WHISKER_ECS_MEM;
	}

	whisker_ecs_entities *e;
	if (whisker_ecs_e_create_entities(&e) != E_WHISKER_ECS_ENTITY_OK)
	{
		free(new);

		return E_WHISKER_ECS_ARR;
	}

	whisker_ecs_components *c;
	if (whisker_ecs_c_create_components(&c) != E_WHISKER_ECS_COMP_OK)
	{
		free(new);
		whisker_ecs_e_free_entities(e);

		return E_WHISKER_ECS_ARR;
	}

	whisker_ecs_systems *s;
	if (whisker_ecs_s_create_systems(&s) != E_WHISKER_ECS_COMP_OK)
	{
		free(new);
		whisker_ecs_e_free_entities(e);
		whisker_ecs_c_free_components(c);

		return E_WHISKER_ECS_ARR;
	}

	new->entities = e;
	new->components = c;
	new->systems = s;

	*ecs = new;

	return E_WHISKER_ECS_OK;
}

void whisker_ecs_free(whisker_ecs *ecs)
{
	// free ecs state
	whisker_ecs_e_free_entities(ecs->entities);
	whisker_ecs_c_free_components(ecs->components);
	whisker_ecs_s_free_systems(ecs->systems);

	free(ecs);
}


/**********************
*  system functions  *
**********************/
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system_update), char *system_name, char *read_component_archetype_names, char *write_component_archetype_names)
{
	debug_printf("ecs:registering system: %s\n", system_name);

	// create an entity for this system with it's name
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_named_(ecs->entities, system_name, &e);

	// attach the id to the system's entity archetype
	// allows a system to read/write to an alias component with it's own name
	whisker_ecs_archetype_set(ecs->entities, e, e);

	// register the system with the system scheduler
	char* read_components;
	char* write_components;
	wstr(read_component_archetype_names, &read_components);
	wstr(write_component_archetype_names, &write_components);
	whisker_ecs_system *system = whisker_ecs_s_register_system(ecs->systems, ecs->components, (whisker_ecs_system) {
		.entity_id = e,
		.system_ptr = system_ptr,
		.read_archetype = whisker_ecs_archetype_from_named_entities(ecs, read_component_archetype_names),
		.write_archetype = whisker_ecs_archetype_from_named_entities(ecs, write_component_archetype_names),
		.read_component_names = read_components,
		.write_component_names = write_components,
		.components = ecs->components,
		.entities = ecs->entities,
	});

	return system;
}

E_WHISKER_ECS whisker_ecs_update(whisker_ecs *ecs, double delta_time)
{
	E_WHISKER_ECS_SYS execute_err = whisker_ecs_s_update_systems(ecs->systems, ecs->entities, delta_time);

	if (execute_err != E_WHISKER_ECS_SYS_OK)
	{
		return E_WHISKER_ECS_UPDATE_SYSTEM;
	}

	return E_WHISKER_ECS_OK;
}


/*******************************
*  entity shortcut functions  *
*******************************/
whisker_ecs_entity_id whisker_ecs_create_entity(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_(entities, &e);

	return e;
}

whisker_ecs_entity_id whisker_ecs_create_named_entity(whisker_ecs_entities *entities, char* name)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_named_(entities, name, &e);

	return e;
}

bool whisker_ecs_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	return (whisker_ecs_e_destroy(entities, entity_id) == E_WHISKER_ECS_ENTITY_OK);
}

bool whisker_ecs_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_e_is_alive(entities, entity_id);
}


/*************************
*  component functions  *
*************************/
whisker_ecs_entity_id whisker_ecs_component_id(whisker_ecs_entities *entities, char* component_name)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_named_(entities, component_name, &e);

	return e;
}

whisker_block_array* whisker_ecs_get_components(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size)
{
	whisker_block_array *components_array;
	whisker_ecs_c_get_component_array(components, whisker_ecs_component_id(entities, component_name), component_size, (void**)&components_array);

	return components_array;
}

whisker_block_array* whisker_ecs_get_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id)
{
	whisker_block_array *components_array = whisker_ecs_get_components(entities, components, component_name, component_size);

	return wbarr_get(components_array, entity_id.index);
}

E_WHISKER_ECS whisker_ecs_set_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id, void* value)
{
	whisker_block_array *components_array = whisker_ecs_get_components(entities, components, component_name, component_size);

	if (value != NULL)
	{
		wbarr_set(components_array, entity_id.index, value);
		whisker_ecs_archetype_set(entities, entity_id, whisker_ecs_component_id(entities, component_name));
	}
	else
	{
		whisker_ecs_archetype_remove(entities, entity_id, whisker_ecs_component_id(entities, component_name));
	}

	return E_WHISKER_ECS_OK;
}

E_WHISKER_ECS whisker_ecs_remove_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_set_component(entities, components, component_name, component_size, entity_id, NULL);
}


/*************************
*  archetype functions  *
*************************/
whisker_ecs_entity_id* whisker_ecs_archetype_from_named_entities(whisker_ecs *ecs, char* entity_names)
{
	return whisker_ecs_e_from_named_entities(ecs->entities, entity_names);
}

E_WHISKER_ECS whisker_ecs_archetype_set(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id)
{
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);

	if (whisker_ecs_a_set(&e->archetype, archetype_id) != E_WHISKER_ECS_ARCH_OK)
	{
		// TODO: figure out what can fail during this call
		return E_WHISKER_ECS_UNKNOWN;
	}

	return E_WHISKER_ECS_OK;
}
E_WHISKER_ECS whisker_ecs_archetype_remove(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id)
{
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);

	if (whisker_ecs_a_remove(&e->archetype, archetype_id) != E_WHISKER_ECS_ARCH_OK)
	{
		// TODO: figure out what can fail during this call
		return E_WHISKER_ECS_UNKNOWN;
	}

	return E_WHISKER_ECS_OK;
}

E_WHISKER_ECS whisker_ecs_set_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	return whisker_ecs_archetype_set(entities, entity_id, component_id);
}

E_WHISKER_ECS whisker_ecs_remove_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	return whisker_ecs_archetype_remove(entities, entity_id, component_id);
}

bool whisker_ecs_has_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	whisker_ecs_entity_id *archetype = whisker_ecs_e(entities, entity_id)->archetype;

	return (whisker_ecs_a_has_id(archetype, component_id) != -1);
}
