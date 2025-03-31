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

	// create a dummy system to use by the system context
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
	// init the system update context
	whisker_ecs_s_init_system_context(&new->system_update_context, &system);

	// register default system process phases to allow bundled systems and
	// modules and a standard default processing phase group set
	whisker_time_step default_time_step = whisker_time_step_create(
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_RATE,
			1,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_UNCAPPED,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_CLAMP,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_SNAP,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_AVERAGE,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_ACCUMULATION,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_ACCUMULATION_CLAMP
		);
	whisker_time_step fixed_update_time_step = whisker_time_step_create(
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_RATE,
			0,
			false,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_CLAMP,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_SNAP,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_AVERAGE,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION_CLAMP
		);
	whisker_time_step rendering_phase_time_step = whisker_time_step_create(
			0,
			0,
			WHISKER_ECS_PROCESS_PHASE_ON_RENDER_UNCAPPED,
			false,
			false,
			false,
			false,
			false
		);
	whisker_time_step reserved_time_step = whisker_time_step_create(
			0,
			0,
			true,
			false,
			false,
			false,
			false,
			false
		);
	size_t default_time_step_id = whisker_ecs_register_process_phase_time_step(new, default_time_step);
	size_t fixed_update_time_step_id = whisker_ecs_register_process_phase_time_step(new, fixed_update_time_step);
	size_t rendering_phase_time_step_id = whisker_ecs_register_process_phase_time_step(new, rendering_phase_time_step);
	size_t reserved_time_step_id = whisker_ecs_register_process_phase_time_step(new, reserved_time_step);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_LOAD, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE, fixed_update_time_step_id);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_UPDATE, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FINAL, default_time_step_id);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_RENDER, rendering_phase_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_RENDER, rendering_phase_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_RENDER, rendering_phase_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER, rendering_phase_time_step_id);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_RESERVED, reserved_time_step_id);

	// register 2 special process phases PRE_PHASE and POST_PHASE
	// then set them to externally managed so they don't get scheduled
	// TODO: implement this into the register process phase functions
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_PHASE_, reserved_time_step_id);
	new->process_phase_pre_idx = new->systems->process_phases_length - 1;
	new->systems->process_phases[new->process_phase_pre_idx].manual_scheduling = true;
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_PHASE_, reserved_time_step_id);
	new->process_phase_post_idx = new->systems->process_phases_length - 1;
	new->systems->process_phases[new->process_phase_post_idx].manual_scheduling = true;

	// register built-in systems
	whisker_ecs_register_system(new, whisker_ecs_system_deregister_startup_phase, "wecs_system_deregister_startup_phase", WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD);

	// create thread pool for general work tasks
	new->general_thread_pool = whisker_tp_create_and_init(0, "ecs_general_tasks");
	whisker_arr_init_t(new->component_sort_requests, 32);

	return new;
}

// deallocate an instance of whisker_ecs and it's complete state
void whisker_ecs_free(whisker_ecs *ecs)
{
	// free ecs state
	whisker_ecs_e_free_entities_all(ecs->entities);
	whisker_ecs_c_free_components_all(ecs->components);
	whisker_ecs_s_free_systems_all(ecs->systems);

	whisker_ecs_s_free_system_context(&ecs->system_update_context);

	// free thread pool
	whisker_tp_free_all(ecs->general_thread_pool);
	free(ecs->component_sort_requests);

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
	/* whisker_ecs_update_process_deferred_component_actions_(ecs); */

	// set component of its type on itself
	whisker_ecs_set_named_component(ecs->entities, ecs->components, system_name, sizeof(bool), e, &(bool){0});
	/* whisker_ecs_update_process_deferred_component_actions_(ecs); */

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
	/* whisker_ecs_update_process_deferred_component_actions_(ecs); */

	/* // HACK: do a single execution of the system to initialise the iterator */
	/* // why: this ensures the system's iterators initialise their component */
	/* // strings and underlying entities in a thread-safe way before */
	/* // multi-threading the system execution */
	/* whisker_ecs_system_context *exec_context = &system->thread_contexts[0]; */
	/* exec_context->system_ptr = system->system_ptr; */
	/* exec_context->entities = system->entities; */
	/* exec_context->components = system->components; */
	/* uint64_t thread_max_back = exec_context->thread_max; */
	/* exec_context->thread_max = UINT64_MAX; */
	/* whisker_ecs_s_update_system(system, exec_context); */
	/* exec_context->thread_max = thread_max_back; */

	return system;
}

// register a process phase time step to use when registering a process phase
size_t whisker_ecs_register_process_phase_time_step(whisker_ecs *ecs, whisker_time_step time_step)
{
	return whisker_ecs_s_register_process_phase_time_step(ecs->systems, time_step);
}

// register a process phase for use by the system scheduler
// note: update_rate_sec set to 0 = uncapped processing with variable delta time
whisker_ecs_entity_id whisker_ecs_register_process_phase(whisker_ecs *ecs, char *phase_name, size_t time_step_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_create_named_entity(ecs->entities, phase_name);

	// add component ID to system's process phase list
	whisker_ecs_s_register_process_phase(ecs->systems, component_id, time_step_id);

	return component_id;
}

// set the order of process phases in the form of a char ** array of names
// note: if a phase is specified which has not been created it will use the
// WHISKER_ECS_PROCESS_PHASE_DEFAULT_RATE for the update rate
void whisker_ecs_set_process_phase_order(whisker_ecs *ecs, char **phase_names, size_t phase_count)
{
	// backup existing process phase array
	whisker_arr_declare(whisker_ecs_system_process_phase, process_phases_backup);
	process_phases_backup = ecs->systems->process_phases;
	process_phases_backup_length = ecs->systems->process_phases_length;
	process_phases_backup_size = ecs->systems->process_phases_size;

	// reinit the process phases array
	whisker_arr_init_t(ecs->systems->process_phases, phase_count);
	ecs->systems->process_phases_length = 0;
	
	for (int i = 0; i < phase_count; ++i)
	{
		whisker_ecs_entity_id component_id = whisker_ecs_create_named_entity(ecs->entities, phase_names[i]);

		bool exists = false;

		// find existing phase in old list
		for (int pi = 0; pi < process_phases_backup_length; ++pi)
		{
			if (process_phases_backup[pi].id.id == component_id.id)
			{
				// re-register the process phase
				debug_log(DEBUG, ecs:set_process_phase_order, "re-registering phase %s", phase_names[i]);

				size_t idx = ecs->systems->process_phases_length++;
				ecs->systems->process_phases[idx].id = component_id;
				ecs->systems->process_phases[idx].time_step_id = process_phases_backup[pi].time_step_id;

				exists = true;
				break;
			}
		}

		// if it doesn't exist, create it using the defaults
		if (!exists)
		{
			debug_log(DEBUG, ecs:set_process_phase_order, "registering new phase %s", phase_names[i]);
			whisker_ecs_s_register_process_phase(ecs->systems, component_id, 0);
		}
	}

	// insert managed process phases
	// note: this assumes that the pre/post phase timestep is the same as the
	// reserved one
	whisker_ecs_register_process_phase(ecs, WHISKER_ECS_PROCESS_PHASE_RESERVED, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);

	whisker_ecs_register_process_phase(ecs, WHISKER_ECS_PROCESS_PHASE_PRE_PHASE_, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);
	ecs->process_phase_pre_idx = ecs->systems->process_phases_length - 1;
	ecs->systems->process_phases[ecs->process_phase_pre_idx].manual_scheduling = true;
	whisker_ecs_register_process_phase(ecs, WHISKER_ECS_PROCESS_PHASE_POST_PHASE_, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);
	ecs->process_phase_post_idx = ecs->systems->process_phases_length - 1;
	ecs->systems->process_phases[ecs->process_phase_post_idx].manual_scheduling = true;

	// free old process phases list
	free(process_phases_backup);
}

/*****************************
*  system update functions  *
*****************************/
// issue an update of all registered systems on all matching world entities
void whisker_ecs_update(whisker_ecs *ecs, double delta_time)
{
	// update each system phase then run deferred actions
    whisker_ecs_system_context *update_context = &ecs->system_update_context;

    for (int i = 0; i < ecs->systems->process_phases_length; ++i)
    {
    	if (ecs->systems->process_phases[i].manual_scheduling) { continue; }

    	// run pre_phase process phase
        whisker_ecs_s_update_process_phase(ecs->systems, ecs->entities, &ecs->systems->process_phases[ecs->process_phase_pre_idx], update_context);

        whisker_ecs_s_update_process_phase(ecs->systems, ecs->entities, &ecs->systems->process_phases[i], update_context);

    	// run post_phase process phase
        whisker_ecs_s_update_process_phase(ecs->systems, ecs->entities, &ecs->systems->process_phases[ecs->process_phase_post_idx], update_context);

		whisker_ecs_update_process_deferred_actions(ecs);
    }

	// reset process phase time steps to allow next frame to advance
	whisker_ecs_s_reset_process_phase_time_steps(ecs->systems);
}

// process any deferred actions queued since the previous update
void whisker_ecs_update_process_deferred_actions(whisker_ecs *ecs)
{
	// for each deferred deleted entity remove all its components
	for (size_t i = 0; i < ecs->entities->deferred_actions_length; ++i)
	{
		whisker_ecs_entity_deferred_action *action = &ecs->entities->deferred_actions[i];

		if (action->action == WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY)
		{
			// HACK: get pool managing the entity and create a deferred removal
			// request for any entity not contained within the pool's components
			whisker_ecs_entity *e = whisker_ecs_e(ecs->entities, action->id);
			if (e->managed_by != NULL)
			{ 
				whisker_ecs_pool *pool = e->managed_by;

				for (int ci = 0; ci < ecs->components->component_ids_length; ++ci)
				{
					whisker_ecs_entity_id component_id = ecs->components->component_ids[ci];
					if (!whisker_ecs_c_has_component(ecs->components, component_id, action->id)) { continue; }

					// if the pool components set contains this component, skip
					// destroying it
					if (whisker_ss_contains(pool->component_ids_set, component_id.index))
					{
						/* printf("deferred component: skip destroying matching pool component %zu (%s) from entity %zu\n", component_id, whisker_ecs_e(ecs->entities, component_id)->name, action->id); */
						continue;
					}

					/* printf("deferred component: destroying non-matching pool component %zu (%s) from entity %zu\n", component_id, whisker_ecs_e(ecs->entities, component_id)->name, action->id); */

					whisker_ecs_create_deferred_component_action(ecs->components, component_id, 0, action->id, NULL, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE);
				}

				if (ecs->entities->entities[action->id.index].destroyed)
				{
					ecs->entities->entities[action->id.index].destroyed = false;
					whisker_ecs_p_return_entity(pool, action->id);
				}

				continue;
			}

			/* whisker_ecs_c_remove_all_components(ecs->components, action->id); */
			whisker_ecs_create_deferred_component_action(ecs->components, action->id, 0, action->id, NULL, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE_ALL);

		}
	}

	// process deferred component actions
	whisker_ecs_update_process_deferred_component_actions_(ecs);

	// process and sort changed components
	whisker_ecs_update_process_changed_components_(ecs);
	
	// process entity actions
	whisker_ecs_update_process_deferred_entity_actions_(ecs);
}

void whisker_ecs_update_process_deferred_entity_actions_(whisker_ecs *ecs)
{
	while (ecs->entities->deferred_actions_length > 0) 
	{
		whisker_ecs_entity_deferred_action action = ecs->entities->deferred_actions[--ecs->entities->deferred_actions_length];

		whisker_ecs_entity *e = whisker_ecs_e(ecs->entities, action.id);

		switch (action.action) {
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE:
				ecs->entities->entities[action.id.index].destroyed = false;		
				break;

			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY:
				// exclude managed entities from being directly destroyed
				if (e->managed_by != NULL)
				{
					continue;
				}
				else
				{
					whisker_ecs_e_destroy(ecs->entities, action.id);
				}
		}
	}
}

void whisker_ecs_update_process_deferred_component_actions_(whisker_ecs *ecs)
{
	// process the deferred actions into real component actions
	if (ecs->components->deferred_actions_length > 0) 
	{
		for (int i = 0; i < ecs->components->deferred_actions_length; ++i)
		{
			struct whisker_ecs_component_deferred_action action = ecs->components->deferred_actions[i];

			switch (action.action) {
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET:
					whisker_ecs_c_set_component(ecs->components, action.component_id, action.data_size, action.entity_id, ecs->components->deferred_actions_data + action.data_offset);
					break;
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE:
					whisker_ecs_c_remove_component(ecs->components, action.component_id, action.entity_id);
					break;
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE_ALL:
					whisker_ecs_c_remove_all_components(ecs->components, action.entity_id);
					break;
				default:
					break;
			}
		}

		ecs->components->deferred_actions_length = 0;
		ecs->components->deferred_actions_data_length = 0;
	}
}

void whisker_ecs_update_process_changed_components_(whisker_ecs *ecs)
{
	whisker_arr_ensure_alloc(ecs->component_sort_requests, ecs->components->component_ids_length);

	for (int i = 0; i < ecs->components->component_ids_length; ++i)
	{
		whisker_ecs_entity_id component_id = ecs->components->component_ids[i];
		if (!ecs->components->components[component_id.index]->mutations_length)
		{
			continue;
		}

		size_t sort_request_idx = ecs->component_sort_requests_length++;
		ecs->component_sort_requests[sort_request_idx].components = ecs->components;
		ecs->component_sort_requests[sort_request_idx].component_id = component_id;

		whisker_tp_queue_work(ecs->general_thread_pool, whisker_ecs_sort_component_thread_func_, &ecs->component_sort_requests[sort_request_idx]);
	}

	if (ecs->component_sort_requests_length)
	{
		whisker_tp_wait_work(ecs->general_thread_pool);
    	ecs->component_sort_requests_length = 0;
	}
}

void whisker_ecs_sort_component_thread_func_(void *component_sort_request, whisker_thread_pool_context *t)
{
	struct whisker_ecs_component_sort_request *sort_request = component_sort_request;
	whisker_ecs_c_sort_component_array(sort_request->components, sort_request->component_id);

	sort_request->components->components[sort_request->component_id.index]->mutations_length = 0;
}


/*******************************
*  entity shortcut functions  *
*******************************/
// request an entity ID to be created or recycled
whisker_ecs_entity_id whisker_ecs_create_entity(whisker_ecs_entities *entities)
{
	return whisker_ecs_e_create(entities);
}

// request an entity ID to be created or recycled, providing a name
// note: names are unique, creating an entity with the same name returns the existing entity if it exists
whisker_ecs_entity_id whisker_ecs_create_named_entity(whisker_ecs_entities *entities, char* name)
{
	return whisker_ecs_e_create_named(entities, name);
}

// immediately destroy the given entity ID
void whisker_ecs_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	if (e->managed_by != NULL)
	{
		// HACK: for now we assume everything managed is managed by a pool
		whisker_ecs_pool *pool = e->managed_by;
		whisker_ecs_p_return_entity(pool, entity_id);
		return;
	}

	whisker_ecs_e_destroy(entities, entity_id);
}

// immediately soft-destroy the given entity ID with an atomic operation
void whisker_ecs_soft_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_e_make_unmanaged(entities, entity_id);
}

// immediately soft-revive the given entity ID with an atomic operation
void whisker_ecs_soft_revive_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_e_make_managed(entities, entity_id);
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
	return whisker_ecs_e_create_deferred(entities);
}

// request to create an entity with a name, deferring the creation until end of current frame
whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(whisker_ecs_entities *entities, char* name)
{
	return whisker_ecs_e_create_named_deferred(entities, name);
}

// request to destroy the provided entity ID at the end of current frame
void whisker_ecs_destroy_entity_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_e_destroy_deferred(entities, entity_id);
	return;
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
	whisker_ecs_entity_id component_id = whisker_ecs_e_create_named(entities, component_name);;
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
	whisker_ecs_create_deferred_component_action(
		components,
		component_id,
		component_size,
		entity_id,
		value,
		WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET
	);

	return value;
}

// remove the component by ID from the given entity
void whisker_ecs_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_create_deferred_component_action(
		components,
		component_id,
		0,
		entity_id,
		NULL,
		WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE
	);
}

// check if an entity has the given component by ID
bool whisker_ecs_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_c_has_component(components, component_id, entity_id);
}

// create a deferred component action to be processed later
void whisker_ecs_create_deferred_component_action(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value, enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action)
{
	whisker_ecs_c_create_deferred_action(components, component_id, entity_id, action, value, component_size, true);
}

/**********************
*  built-in systems  *
**********************/

// this system ensures the process phase WHISKER_ECS_PROCESS_PHASE_ON_STARTUP
// gets disabled after the first frame along with this system
void whisker_ecs_system_deregister_startup_phase(whisker_ecs_system_context *context)
{
	// destroy the process phase entity, and destroy this system's entity
	whisker_ecs_entity *e = whisker_ecs_e_named(context->entities, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP);

	if (e)
	{
		debug_log(DEBUG, ecs:system_deregister_startup_phase, "de-registering startup phase entity %d", e->id.index);

		whisker_ecs_soft_destroy_entity(context->entities, e->id);

		// destroy system entity to prevent running it again
		whisker_ecs_soft_destroy_entity(context->entities, context->system_entity_id);
	}
}
