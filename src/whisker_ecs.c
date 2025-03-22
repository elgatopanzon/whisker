/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:29 CST
 */

#include "whisker_std.h"

#include "whisker_debug.h"
#include "whisker_array.h"
#include "whisker_block_array.h"
#include "whisker_ecs.h"

// create and initialise an instance of whisker_ecs to hold the ECS's world
// state (entities, components, systems)
E_WHISKER_ECS whisker_ecs_create(whisker_ecs **ecs)
{
	whisker_ecs *new = whisker_mem_xcalloc(1, sizeof(*new));
	if (new == NULL)
	{
		return E_WHISKER_ECS_MEM;
	}

	whisker_ecs_entities *e = whisker_ecs_e_create_and_init_entities();
	
	whisker_ecs_components *c;
	if (whisker_ecs_c_create_components(&c) != E_WHISKER_ECS_COMP_OK)
	{
		free(new);
		whisker_ecs_e_free_entities_all(e);

		return E_WHISKER_ECS_ARR;
	}

	whisker_ecs_systems *s;
	if (whisker_ecs_s_create_systems(&s) != E_WHISKER_ECS_COMP_OK)
	{
		free(new);
		whisker_ecs_e_free_entities_all(e);
		whisker_ecs_c_free_components(c);

		return E_WHISKER_ECS_ARR;
	}

	new->entities = e;
	new->components = c;
	new->systems = s;


	// reserve 1 entity for system use
	whisker_ecs_e_create(e);

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
		.entities = e,
		.components = c,
	};
	whisker_ecs_system *sys = whisker_ecs_s_register_system(s, c, system);
	if (sys == NULL)
	{
		return E_WHISKER_ECS_MEM;
	}

	// note: for now the first system will be ID 0 and reserved for this dummy
	// system instance
	s->system_id = 0;

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

	*ecs = new;

	return E_WHISKER_ECS_OK;
}

// deallocate an instance of whisker_ecs and it's complete state
void whisker_ecs_free(whisker_ecs *ecs)
{
	// free ecs state
	whisker_ecs_e_free_entities_all(ecs->entities);
	whisker_ecs_c_free_components(ecs->components);
	whisker_ecs_s_free_systems(ecs->systems);

	free(ecs);
}


/**********************
*  system functions  *
**********************/
// register a system function with a name and desired process phase group name to execute in
// note: the process phase group has to be registered or it will not be scheduled for execution
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system_context*), char *system_name, char *process_phase_name, size_t thread_count)
{
	debug_printf("ecs:registering system: %s process phase %s\n", system_name, process_phase_name);

	// get the entity for the process phase
	whisker_ecs_entity_id phase_e = whisker_ecs_create_named_entity(ecs->entities, process_phase_name);
	// set the component on the system

	// create an entity for this system with it's name
	whisker_ecs_entity_id e = whisker_ecs_create_named_entity(ecs->entities, system_name);
	if (e.id == 0)
	{
		// TODO: panic here since it's an unrecoverable error
		return NULL;
	}

	// add process phase component to system
	void *phase_component = whisker_ecs_set_named_component(ecs->entities, ecs->components, process_phase_name, sizeof(bool), e, &(bool){0});
	if (phase_component == NULL)
	{
		// TODO: panic here since it's an unrecoverable error
		return NULL;
	}

	// set component of its type on itself
	void *name_component = whisker_ecs_set_named_component(ecs->entities, ecs->components, system_name, sizeof(bool), e, &(bool){0});
	if (name_component == NULL)
	{
		// TODO: panic here since it's an unrecoverable error
		return NULL;
	}

	// register the system with the system scheduler
	whisker_ecs_system *system = whisker_ecs_s_register_system(ecs->systems, ecs->components, (whisker_ecs_system) {
		.entity_id = e,
		.process_phase_id = phase_e,
		.system_ptr = system_ptr,
		.thread_count = thread_count,
		.components = ecs->components,
		.entities = ecs->entities,
	});

	if (system == NULL)
	{
		// TODO: panic here since it's an unrecoverable error
		return NULL;
	}

	// add the system index component to the system entity
	void *system_idx_component = whisker_ecs_set_named_component(ecs->entities, ecs->components, "w_ecs_system_idx", sizeof(int), e, &(int){ecs->systems->systems_length - 1});
	if (system_idx_component == NULL)
	{
		// TODO: panic here since it's an unrecoverable error
		return NULL;
	}

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
E_WHISKER_ECS whisker_ecs_update(whisker_ecs *ecs, double delta_time)
{
	// run the actual system update
	E_WHISKER_ECS_SYS execute_err = whisker_ecs_s_update_systems(ecs->systems, ecs->entities, delta_time);

	if (execute_err != E_WHISKER_ECS_SYS_OK)
	{
		return E_WHISKER_ECS_UPDATE_SYSTEM;
	}

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
					E_WHISKER_ECS_COMP remove_err = whisker_ecs_c_remove_component(ecs->components, component_id, action->id, false);
					if (remove_err != E_WHISKER_ECS_COMP_OK)
					{
						// TODO: panic here since it's unrecoverable
						// for now, just continue the loop
						continue;
					}

					if (!wss_contains(ecs->components->changed_components, component_id.id))
					{
						E_WHISKER_SS set_err = wss_set(ecs->components->changed_components, component_id.id, &component_id);
						if (set_err != E_WHISKER_SS_OK)
						{
							// TODO: panic here since it's unrecoverable
							continue;
						}
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

			E_WHISKER_ECS_COMP sort_err = whisker_ecs_c_sort_component_array(ecs->components, component_id);
			if (sort_err != E_WHISKER_ECS_COMP_OK)
			{
				// TODO: panic here
			}

			E_WHISKER_SS set_err = whisker_ss_set_dense_index(ecs->components->changed_components, component_id.id, UINT64_MAX);
			if (set_err != E_WHISKER_SS_OK)
			{
				// TODO: some kind of panic
				continue;
			}
		}

		ecs->components->changed_components->sparse_index->length = 0;
    	whisker_arr_header(ecs->components->changed_components->dense)->length = 0;
	}
	
	// process entity actions
	whisker_ecs_e_process_deferred(ecs->entities);

	return E_WHISKER_ECS_OK;
}

// register a process phase for use by the system scheduler
// note: update_rate_sec set to 0 = uncapped processing with variable delta time
whisker_ecs_entity_id whisker_ecs_register_process_phase(whisker_ecs *ecs, char *phase_name, double update_rate_sec)
{
	whisker_ecs_entity_id component_id = whisker_ecs_create_named_entity(ecs->entities, phase_name);
	if (component_id.id == 0)
	{
		// TODO: some kind of panic
		return component_id;
	}

	// add component ID to system's process phase list
	E_WHISKER_ECS_SYS add_err = whisker_ecs_s_register_process_phase(ecs->systems, component_id, update_rate_sec);
	if (add_err != E_WHISKER_ECS_SYS_OK)
	{
		// TODO: some kind of panic
		return (whisker_ecs_entity_id) { .id = 0 };
	}

	return component_id;
}

/*******************************
*  entity shortcut functions  *
*******************************/
// request an entity ID to be created or recycled
whisker_ecs_entity_id whisker_ecs_create_entity(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_id e;
	E_WHISKER_ECS_ENTITY err = whisker_ecs_e_create_(entities, &e);
	if (err != E_WHISKER_ECS_ENTITY_OK)
	{
		// note: make it more obvious elsewhere that entity ID 0 means the
		// entity failed to be created
		// TODO: whisker_error.h global error handler, setting the real error
		return (whisker_ecs_entity_id) { .id = 0 };
	}

	return e;
}

// request an entity ID to be created or recycled, providing a name
// note: names are unique, creating an entity with the same name returns the existing entity if it exists
whisker_ecs_entity_id whisker_ecs_create_named_entity(whisker_ecs_entities *entities, char* name)
{
	whisker_ecs_entity_id e;
	E_WHISKER_ECS_ENTITY err = whisker_ecs_e_create_named_(entities, name, &e);
	if (err != E_WHISKER_ECS_ENTITY_OK)
	{
		// note: make it more obvious elsewhere that entity ID 0 means the
		// entity failed to be created
		// TODO: whisker_error.h global error handler, setting the real error
		return (whisker_ecs_entity_id) { .id = 0 };
	}

	return e;
}

// immediately destroy the given entity ID
bool whisker_ecs_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	return (whisker_ecs_e_destroy(entities, entity_id) == E_WHISKER_ECS_ENTITY_OK);
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
	E_WHISKER_ECS_ENTITY err = whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});
	if (err != E_WHISKER_ECS_ENTITY_OK)
	{
		// TODO: whisker_error.h global error handler, setting the real error
		return (whisker_ecs_entity_id) { .id = 0 };
	}

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
	E_WHISKER_ECS_ENTITY err = whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});
	if (err != E_WHISKER_ECS_ENTITY_OK)
	{
		// TODO: whisker_error.h global error handler, setting the real error
		return (whisker_ecs_entity_id) { .id = 0 };
	}

	return e;
}

// request to destroy the provided entity ID at the end of current frame
bool whisker_ecs_destroy_entity_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	E_WHISKER_ECS_ENTITY err = whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = entity_id, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY});

	if (err != E_WHISKER_ECS_ENTITY_OK)
	{
		// TODO: whisker_error.h global error handler, setting the real error
		return false;
	}

	return true;
}

/*************************
*  component functions  *
*************************/
// get the component entity ID for the given component name
whisker_ecs_entity_id whisker_ecs_component_id(whisker_ecs_entities *entities, char* component_name)
{
	whisker_ecs_entity_id e;
	E_WHISKER_ECS_ENTITY err = whisker_ecs_e_create_named_(entities, component_name, &e);
	if (err != E_WHISKER_ECS_ENTITY_OK)
	{
		// note: make it more obvious elsewhere that entity ID 0 means the
		// entity failed to be created
		// TODO: whisker_error.h global error handler, setting the real error
		return (whisker_ecs_entity_id) { .id = 0 };
	}

	return e;
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
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}
	return whisker_ecs_set_component(components, component_id, component_size, entity_id, value);
}

// remove a named component from an entity
bool whisker_ecs_remove_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(entities, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}

	return whisker_ecs_remove_component(components, component_id, entity_id) == E_WHISKER_ECS_COMP_OK;
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
	E_WHISKER_ECS_COMP err = whisker_ecs_c_set_component(components, component_id, component_size, entity_id, value, false);

	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return NULL;
	}

	if (!wss_contains(components->changed_components, component_id.id))
	{
		E_WHISKER_SS err = wss_set(components->changed_components, component_id.id, &component_id);
		if (err != E_WHISKER_SS_OK)
		{
			// TODO: panic here
			return NULL;
		}
	}

	return value;
}

// remove the component by ID from the given entity
bool whisker_ecs_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	if (!wss_contains(components->changed_components, component_id.id))
	{
		E_WHISKER_SS err = wss_set(components->changed_components, component_id.id, &component_id);
		if (err != E_WHISKER_SS_OK)
		{
			// TODO: panic here
			return NULL;
		}
	}

	return whisker_ecs_c_remove_component(components, component_id, entity_id, false) == E_WHISKER_ECS_COMP_OK;
}

// check if an entity has the given component by ID
bool whisker_ecs_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_c_has_component(components, component_id, entity_id);
}

