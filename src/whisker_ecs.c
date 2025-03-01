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
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system*), char *system_name)
{
	debug_printf("ecs:registering system: %s\n", system_name);

	// create an entity for this system with it's name
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_named_(ecs->entities, system_name, &e);

	// register the system with the system scheduler
	whisker_ecs_system *system = whisker_ecs_s_register_system(ecs->systems, ecs->components, (whisker_ecs_system) {
		.entity_id = e,
		.system_ptr = system_ptr,
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

	// process deferred actions
	whisker_ecs_e_process_deferred(ecs->entities);

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

whisker_ecs_entity_id whisker_ecs_create_entity_deferred(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_id e = whisker_ecs_create_entity(entities);

	// set the entity to dead and add it to the deferred entities
	entities->entities[e.index].destroyed = false;
	warr_push(&entities->deferred_actions, (&(whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE}));

	return e;
}

whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(whisker_ecs_entities *entities, char* name)
{
	whisker_ecs_entity_id e = whisker_ecs_create_entity(entities);

	// set the entity to dead and add it to the deferred entities
	entities->entities[e.index].destroyed = false;
	warr_push(&entities->deferred_actions, (&(whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE}));

	return e;
}

bool whisker_ecs_destroy_entity_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	warr_push(&entities->deferred_actions, (&(whisker_ecs_entity_deferred_action){.id = entity_id, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY}));

	return false;
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
