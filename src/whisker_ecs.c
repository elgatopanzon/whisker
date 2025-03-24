/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:29 CST
 */

#include "whisker_std.h"

#include "whisker_debug.h"
#include "whisker_array.h"
#include "whisker_ecs.h"

// create and initialise an instance of whisker_ecs to hold the ECS's world
// state (entities, components, systems)
whisker_ecs *whisker_ecs_create()
{
	whisker_ecs *new = whisker_mem_xcalloc(1, sizeof(*new));

	new->entities = whisker_ecs_e_create_and_init_entities();
	new->components = whisker_ecs_c_create_and_init_components();
	new->systems = whisker_ecs_s_create_and_init_systems();

	// reserve 1 entity for system use
	whisker_ecs_e_create(new->entities);

	// create and register a dummy system to use by the system scheduler
	// note: this is currently to use as a holder for the system iterator used
	// in the scheduler
	whisker_ecs_system system = {
		.entity_id = 0,
		.process_phase_id = 0,
		.system_ptr = NULL,
		.thread_id = 0,
		.last_update = 0,
		.delta_time = 0,
		.entities = new->entities,
		.components = new->components,
	};
	whisker_ecs_s_register_system(new->systems, new->components, system);

	// note: for now the first system will be ID 0 and reserved for this dummy
	// system instance
	new->systems->system_id = 0;

	// register default system process phases to allow bundled systems and
	// modules and a standard default processing phase group set
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_LOAD, WHISKER_ECS_PROCESS_PHASE_PRE_LOAD_RATE);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE, WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, WHISKER_ECS_PROCESS_PHASE_ON_UPDATE_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_UPDATE, WHISKER_ECS_PROCESS_PHASE_POST_UPDATE_RATE);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_RENDER, WHISKER_ECS_PROCESS_PHASE_PRE_RENDER_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_RENDER, WHISKER_ECS_PROCESS_PHASE_ON_RENDER_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_RENDER, WHISKER_ECS_PROCESS_PHASE_POST_RENDER_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER, WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER_RATE);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_PHASE_FINAL_RATE);

	// create thread pool for general work tasks
	new->general_thread_pool = whisker_tp_create_and_init(0, "ecs_general_tasks");

	return new;
}

// deallocate an instance of whisker_ecs and it's complete state
void whisker_ecs_free(whisker_ecs *ecs)
{
	// free ecs state
	whisker_ecs_e_free_entities_all(ecs->entities);
	whisker_ecs_c_free_components_all(ecs->components);
	whisker_ecs_s_free_systems_all(ecs->systems);

	// free thread pool
	whisker_tp_free_all(ecs->general_thread_pool);

	free(ecs);
}


/**********************
*  system functions  *
**********************/
// register a system function with a name and desired process phase group name to execute in
// note: the process phase group has to be registered or it will not be scheduled for execution
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system_context*), char *system_name, char *process_phase_name, size_t thread_count)
{
	debug_log(DEBUG, ecs:register_system, "registering system %s process phase %s", system_name, process_phase_name);

	// get the entity for the process phase
	whisker_ecs_entity_id phase_e = whisker_ecs_create_named_entity(ecs->entities, process_phase_name);
	// set the component on the system

	// create an entity for this system with it's name
	whisker_ecs_entity_id e = whisker_ecs_create_named_entity(ecs->entities, system_name);

	// add process phase component to system
	whisker_ecs_set_named_component(ecs->entities, ecs->components, process_phase_name, sizeof(bool), e, &(bool){0});

	// set component of its type on itself
	whisker_ecs_set_named_component(ecs->entities, ecs->components, system_name, sizeof(bool), e, &(bool){0});

	// register the system with the system scheduler
	whisker_ecs_system *system = whisker_ecs_s_register_system(ecs->systems, ecs->components, (whisker_ecs_system) {
		.entity_id = e,
		.process_phase_id = phase_e,
		.system_ptr = system_ptr,
		.thread_count = thread_count,
		.components = ecs->components,
		.entities = ecs->entities,
	});

	// add the system index component to the system entity
	whisker_ecs_set_named_component(ecs->entities, ecs->components, "w_ecs_system_idx", sizeof(int), e, &(int){ecs->systems->systems_length - 1});

	// HACK: do a single execution of the system to initialise the iterator
	// why: this ensures the system's iterators initialise their component
	// strings and underlying entities in a thread-safe way before
	// multi-threading the system execution
	whisker_ecs_system_context *exec_context = system->thread_contexts[0];
	exec_context->system_ptr = system->system_ptr;
	exec_context->entities = system->entities;
	exec_context->components = system->components;
	uint64_t thread_max_back = exec_context->thread_max;
	exec_context->thread_max = UINT64_MAX;
	whisker_ecs_s_update_system(system, exec_context);
	exec_context->thread_max = thread_max_back;

	return system;
}

// issue an update of all registered systems on all matching world entities
void whisker_ecs_update(whisker_ecs *ecs, double delta_time)
{
	// run the actual system update
	whisker_ecs_s_update_systems(ecs->systems, ecs->entities, delta_time);

	// process deferred actions
	// for each deferred deleted entity remove all its components
	for (size_t i = 0; i < ecs->entities->deferred_actions_length; ++i)
	{
		whisker_ecs_entity_deferred_action *action = &ecs->entities->deferred_actions[i];

		if (action->action == WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY)
		{
			for (int ci = 0; ci < ecs->components->components_length; ++ci)
			{
				whisker_ecs_entity_id component_id = whisker_ecs_e_id(ci);

				if (whisker_ecs_c_has_component(ecs->components, component_id, action->id))
				{
					whisker_ecs_c_remove_component(ecs->components, component_id, action->id, false);

					if (!whisker_ss_contains(ecs->components->changed_components, component_id.id))
					{
						whisker_ss_set(ecs->components->changed_components, component_id.id, &component_id);
					}
				}
			}
		}
	}

	// process and sort changed components
	while (ecs->components->changed_components->sparse_index_length > 0) 
	{
		for (int i = 0; i < ecs->components->changed_components->sparse_index_length; ++i)
		{
			whisker_ecs_entity_id component_id = {.id = ecs->components->changed_components->sparse_index[i]};

			whisker_ecs_c_sort_component_array(ecs->components, component_id);
			whisker_ss_set_dense_index(ecs->components->changed_components, component_id.id, UINT64_MAX);
		}

		ecs->components->changed_components->sparse_index_length = 0;
    	ecs->components->changed_components->dense_length = 0;
	}
	
	// process entity actions
	whisker_ecs_e_process_deferred(ecs->entities);
}

// register a process phase for use by the system scheduler
// note: update_rate_sec set to 0 = uncapped processing with variable delta time
whisker_ecs_entity_id whisker_ecs_register_process_phase(whisker_ecs *ecs, char *phase_name, double update_rate_sec)
{
	whisker_ecs_entity_id component_id = whisker_ecs_create_named_entity(ecs->entities, phase_name);

	// add component ID to system's process phase list
	whisker_ecs_s_register_process_phase(ecs->systems, component_id, update_rate_sec);

	return component_id;
}

/*******************************
*  entity shortcut functions  *
*******************************/
// request an entity ID to be created or recycled
whisker_ecs_entity_id whisker_ecs_create_entity(whisker_ecs_entities *entities)
{
	return whisker_ecs_e_create_(entities);
}

// request an entity ID to be created or recycled, providing a name
// note: names are unique, creating an entity with the same name returns the existing entity if it exists
whisker_ecs_entity_id whisker_ecs_create_named_entity(whisker_ecs_entities *entities, char* name)
{
	return whisker_ecs_e_create_named_(entities, name);
}

// immediately destroy the given entity ID
void whisker_ecs_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_e_destroy(entities, entity_id);
}

// check whether the given entity ID is still alive
// note: this performs a version check using the full entity 64 bit ID
bool whisker_ecs_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_e_is_alive(entities, entity_id);
}

// request to create an entity, deferring the creation until end of current frame
whisker_ecs_entity_id whisker_ecs_create_entity_deferred(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_id e = whisker_ecs_create_entity(entities);
	if (e.id == 0)
	{
		return e;
	}

	// set the entity to dead and add it to the deferred entities
	entities->entities[e.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	return e;
}

// request to create an entity with a name, deferring the creation until end of current frame
whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(whisker_ecs_entities *entities, char* name)
{
	whisker_ecs_entity_id e = whisker_ecs_create_entity(entities);
	if (e.id == 0)
	{
		return e;
	}

	// set the entity to dead and add it to the deferred entities
	entities->entities[e.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	return e;
}

// request to destroy the provided entity ID at the end of current frame
void whisker_ecs_destroy_entity_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = entity_id, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY});
}

/*************************
*  component functions  *
*************************/
// get the component entity ID for the given component name
whisker_ecs_entity_id whisker_ecs_component_id(whisker_ecs_entities *entities, char* component_name)
{
	return whisker_ecs_e_named(entities, component_name)->id;
}

// get a named component for an entity
// note: the component has to exist on the entity first
void *whisker_ecs_get_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}

	return whisker_ecs_get_component(components, component_id, entity_id);
}

// set a named component on an entity
// note: this will handle the creation of the underlying component array
void *whisker_ecs_set_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, size_t component_size, whisker_ecs_entity_id entity_id, void *value)
{
	whisker_ecs_entity_id component_id = whisker_ecs_e_create_named_(entities, component_name);;
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}
	return whisker_ecs_set_component(components, component_id, component_size, entity_id, value);
}

// remove a named component from an entity
void whisker_ecs_remove_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		return;
	}

	whisker_ecs_remove_component(components, component_id, entity_id);
}

// check whether an entity has a named component attached
bool whisker_ecs_has_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}
	return whisker_ecs_has_component(components, component_id, entity_id);
}

// get the component by ID for the given entity
// note: the component has to exist on the entity
void *whisker_ecs_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_c_get_component(components, component_id, entity_id);
}

// set the component by ID on the given entity
// note: this will handle the creation of the underlying component array
void *whisker_ecs_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value)
{
	whisker_ecs_c_set_component(components, component_id, component_size, entity_id, value, false);

	if (!whisker_ss_contains(components->changed_components, component_id.id))
	{
		whisker_ss_set(components->changed_components, component_id.id, &component_id);
	}

	return value;
}

// remove the component by ID from the given entity
void whisker_ecs_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	if (!whisker_ss_contains(components->changed_components, component_id.id))
	{
		whisker_ss_set(components->changed_components, component_id.id, &component_id);
	}

	whisker_ecs_c_remove_component(components, component_id, entity_id, false);
}

// check if an entity has the given component by ID
bool whisker_ecs_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_c_has_component(components, component_id, entity_id);
}
