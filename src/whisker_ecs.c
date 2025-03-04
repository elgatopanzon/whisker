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

	// set component of its type on itself
	whisker_ecs_set_component(ecs->entities, ecs->components, system_name, sizeof(bool), e, &(bool){0});

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
	// for each deferred deleted entity remove all its components
	for (size_t i = 0; i < ecs->entities->deferred_actions->length; ++i)
	{
		whisker_ecs_entity_deferred_action *action = &ecs->entities->deferred_actions->arr[i];

		if (action->action == WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY)
		{
			for (int ci = 0; ci < ecs->components->components_length; ++ci)
			{
				whisker_ecs_entity_id component_id = whisker_ecs_e_id(ci);

				if (whisker_ecs_c_has_component(ecs->components, component_id, action->id))
				{
					whisker_ecs_c_remove_component(ecs->components, component_id, action->id, false);

					if (!wss_contains(ecs->components->changed_components, component_id.id))
					{
						wss_set(ecs->components->changed_components, component_id.id, &component_id);
					}
				}
			}
		}
	}

	// process and sort changed components
	while (ecs->components->changed_components->sparse_index->length > 0) 
	{
		for (int i = 0; i < ecs->components->changed_components->sparse_index->length; ++i)
		{
			whisker_ecs_entity_id component_id = {.id = ecs->components->changed_components->sparse_index->arr[i]};

			whisker_ecs_c_sort_component_array(ecs->components, component_id);

			whisker_ss_set_dense_index(ecs->components->changed_components, component_id.id, UINT64_MAX);
		}

		ecs->components->changed_components->sparse_index->length = 0;
    	warr_header(ecs->components->changed_components->dense)->length = 0;
	}
	
	// process entity actions
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
	entities->entities->arr[e.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	return e;
}

whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(whisker_ecs_entities *entities, char* name)
{
	whisker_ecs_entity_id e = whisker_ecs_create_entity(entities);

	// set the entity to dead and add it to the deferred entities
	entities->entities->arr[e.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	return e;
}

bool whisker_ecs_destroy_entity_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	entities->entities->arr[entity_id.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = entity_id, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY});

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

void *whisker_ecs_get_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	return whisker_ecs_c_get_component(components, component_id, entity_id);
}

void *whisker_ecs_set_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, size_t component_size, whisker_ecs_entity_id entity_id, void *value)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	E_WHISKER_ECS_COMP err = whisker_ecs_c_set_component(components, component_id, component_size, entity_id, value, false);

	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return NULL;
	}

	if (!wss_contains(components->changed_components, component_id.id))
	{
		wss_set(components->changed_components, component_id.id, &component_id);
	}

	return whisker_ecs_get_component(entities, components, component_name, entity_id);
}

bool whisker_ecs_remove_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);

	if (!wss_contains(components->changed_components, component_id.id))
	{
		wss_set(components->changed_components, component_id.id, &component_id);
	}

	return whisker_ecs_c_remove_component(components, component_id, entity_id, false) == E_WHISKER_ECS_COMP_OK;
}


bool whisker_ecs_has_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	return whisker_ecs_c_has_component(components, component_id, entity_id);
}
