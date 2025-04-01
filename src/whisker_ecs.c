/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:29 CST
 */

#include "whisker_std.h"

#include "whisker_debug.h"
#include "whisker_array.h"
#include "whisker_ecs.h"

/***************
*  ECS world  *
***************/

// allocate an ECS world object
struct whisker_ecs_world *whisker_ecs_world_create()
{
	return whisker_mem_xcalloc_t(1, struct whisker_ecs_world);
}

// allocate and init an ECS world object
struct whisker_ecs_world *whisker_ecs_world_create_and_init(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems)
{
	struct whisker_ecs_world *world = whisker_ecs_world_create();
	whisker_ecs_world_init(world, entities, components, systems);
	return world;
}

// init an ECS world object
void whisker_ecs_world_init(struct whisker_ecs_world *world, whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_systems *systems)
{
	world->entities = entities;
	world->components = components;
	world->systems = systems;
}


/*********
*  ECS  *
*********/

// create and initialise an instance of whisker_ecs to hold the ECS's world
// state (entities, components, systems)
whisker_ecs *whisker_ecs_create()
{
	whisker_ecs *new = whisker_mem_xcalloc(1, sizeof(*new));
	//
	// create world object
	new->world = whisker_ecs_world_create(new);

	new->world->entities = whisker_ecs_create_and_init_entities_container_();
	new->world->components = whisker_ecs_c_create_and_init_components();
	new->world->systems = whisker_ecs_s_create_and_init_systems();


	// reserve 1 entity for system use
	whisker_ecs_e_create(new->world->entities);

	// create a dummy system to use by the system context
	whisker_ecs_system system = {
		.entity_id = 0,
		.process_phase_id = 0,
		.system_ptr = NULL,
		.thread_id = 0,
		.last_update = 0,
		.delta_time = 0,
		.world = new->world,
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
	new->process_phase_pre_idx = new->world->systems->process_phases_length - 1;
	new->world->systems->process_phases[new->process_phase_pre_idx].manual_scheduling = true;
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_PHASE_, reserved_time_step_id);
	new->process_phase_post_idx = new->world->systems->process_phases_length - 1;
	new->world->systems->process_phases[new->process_phase_post_idx].manual_scheduling = true;

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
	whisker_ecs_free_entities_all_(ecs->world->entities);
	whisker_ecs_c_free_components_all(ecs->world->components);
	whisker_ecs_s_free_systems_all(ecs->world->systems);

	whisker_ecs_s_free_system_context(&ecs->system_update_context);

	// free thread pool
	whisker_tp_free_all(ecs->general_thread_pool);
	free(ecs->component_sort_requests);

	free(ecs->world);
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
	whisker_ecs_entity_id phase_e = whisker_ecs_create_named_entity(ecs->world, process_phase_name);
	// set the component on the system

	// create an entity for this system with it's name
	whisker_ecs_entity_id e = whisker_ecs_create_named_entity(ecs->world, system_name);

	// add process phase component to system
	whisker_ecs_set_named_component(ecs->world, process_phase_name, sizeof(bool), e, &(bool){0});
	/* whisker_ecs_update_process_deferred_component_actions_(ecs); */

	// set component of its type on itself
	whisker_ecs_set_named_component(ecs->world, system_name, sizeof(bool), e, &(bool){0});
	/* whisker_ecs_update_process_deferred_component_actions_(ecs); */

	// register the system with the system scheduler
	whisker_ecs_system *system = whisker_ecs_s_register_system(ecs->world->systems, ecs->world->components, (whisker_ecs_system) {
		.entity_id = e,
		.process_phase_id = phase_e,
		.system_ptr = system_ptr,
		.thread_count = thread_count,
		.world = ecs->world,
	});

	// add the system index component to the system entity
	whisker_ecs_set_named_component(ecs->world, "w_ecs_system_idx", sizeof(int), e, &(int){ecs->world->systems->systems_length - 1});
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
	return whisker_ecs_s_register_process_phase_time_step(ecs->world->systems, time_step);
}

// register a process phase for use by the system scheduler
// note: update_rate_sec set to 0 = uncapped processing with variable delta time
whisker_ecs_entity_id whisker_ecs_register_process_phase(whisker_ecs *ecs, char *phase_name, size_t time_step_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_create_named_entity(ecs->world, phase_name);

	// add component ID to system's process phase list
	whisker_ecs_s_register_process_phase(ecs->world->systems, component_id, time_step_id);

	return component_id;
}

// set the order of process phases in the form of a char ** array of names
// note: if a phase is specified which has not been created it will use the
// WHISKER_ECS_PROCESS_PHASE_DEFAULT_RATE for the update rate
void whisker_ecs_set_process_phase_order(whisker_ecs *ecs, char **phase_names, size_t phase_count)
{
	// backup existing process phase array
	whisker_arr_declare(whisker_ecs_system_process_phase, process_phases_backup);
	process_phases_backup = ecs->world->systems->process_phases;
	process_phases_backup_length = ecs->world->systems->process_phases_length;
	process_phases_backup_size = ecs->world->systems->process_phases_size;

	// reinit the process phases array
	whisker_arr_init_t(ecs->world->systems->process_phases, phase_count);
	ecs->world->systems->process_phases_length = 0;
	
	for (int i = 0; i < phase_count; ++i)
	{
		whisker_ecs_entity_id component_id = whisker_ecs_create_named_entity(ecs->world, phase_names[i]);

		bool exists = false;

		// find existing phase in old list
		for (int pi = 0; pi < process_phases_backup_length; ++pi)
		{
			if (process_phases_backup[pi].id.id == component_id.id)
			{
				// re-register the process phase
				debug_log(DEBUG, ecs:set_process_phase_order, "re-registering phase %s", phase_names[i]);

				size_t idx = ecs->world->systems->process_phases_length++;
				ecs->world->systems->process_phases[idx].id = component_id;
				ecs->world->systems->process_phases[idx].time_step_id = process_phases_backup[pi].time_step_id;

				exists = true;
				break;
			}
		}

		// if it doesn't exist, create it using the defaults
		if (!exists)
		{
			debug_log(DEBUG, ecs:set_process_phase_order, "registering new phase %s", phase_names[i]);
			whisker_ecs_s_register_process_phase(ecs->world->systems, component_id, 0);
		}
	}

	// insert managed process phases
	// note: this assumes that the pre/post phase timestep is the same as the
	// reserved one
	whisker_ecs_register_process_phase(ecs, WHISKER_ECS_PROCESS_PHASE_RESERVED, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);

	whisker_ecs_register_process_phase(ecs, WHISKER_ECS_PROCESS_PHASE_PRE_PHASE_, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);
	ecs->process_phase_pre_idx = ecs->world->systems->process_phases_length - 1;
	ecs->world->systems->process_phases[ecs->process_phase_pre_idx].manual_scheduling = true;
	whisker_ecs_register_process_phase(ecs, WHISKER_ECS_PROCESS_PHASE_POST_PHASE_, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);
	ecs->process_phase_post_idx = ecs->world->systems->process_phases_length - 1;
	ecs->world->systems->process_phases[ecs->process_phase_post_idx].manual_scheduling = true;

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

    for (int i = 0; i < ecs->world->systems->process_phases_length; ++i)
    {
    	if (ecs->world->systems->process_phases[i].manual_scheduling) { continue; }

    	// run pre_phase process phase
        whisker_ecs_s_update_process_phase(ecs->world->systems, ecs->world->entities, &ecs->world->systems->process_phases[ecs->process_phase_pre_idx], update_context);

        whisker_ecs_s_update_process_phase(ecs->world->systems, ecs->world->entities, &ecs->world->systems->process_phases[i], update_context);

    	// run post_phase process phase
        whisker_ecs_s_update_process_phase(ecs->world->systems, ecs->world->entities, &ecs->world->systems->process_phases[ecs->process_phase_post_idx], update_context);

		whisker_ecs_update_process_deferred_actions(ecs);
    }

	// reset process phase time steps to allow next frame to advance
	whisker_ecs_s_reset_process_phase_time_steps(ecs->world->systems);
}

// process any deferred actions queued since the previous update
void whisker_ecs_update_process_deferred_actions(whisker_ecs *ecs)
{
	// pre-process destroyed entities
	whisker_ecs_update_pre_process_destroyed_entities_(ecs);

	// process deferred component actions
	whisker_ecs_update_process_deferred_component_actions_(ecs);

	// process and sort changed components
	whisker_ecs_update_process_changed_components_(ecs);
	
	// process entity actions
	whisker_ecs_update_process_deferred_entity_actions_(ecs);
}

void whisker_ecs_update_pre_process_destroyed_entities_(whisker_ecs *ecs)
{
	// for each deferred deleted entity remove all its components
	for (size_t i = 0; i < ecs->world->entities->deferred_actions_length; ++i)
	{
		whisker_ecs_entity_deferred_action *action = &ecs->world->entities->deferred_actions[i];

		if (action->action == WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY)
		{
			// HACK: get pool managing the entity and create a deferred removal
			// request for any entity not contained within the pool's components
			whisker_ecs_entity *e = whisker_ecs_e(ecs->world->entities, action->id);
			if (e->managed_by != NULL)
			{ 
				whisker_ecs_pool *pool = e->managed_by;

				for (int ci = 0; ci < ecs->world->components->component_ids_length; ++ci)
				{
					whisker_ecs_entity_id component_id = ecs->world->components->component_ids[ci];
					if (!whisker_ecs_c_has_component(ecs->world->components, component_id, action->id)) { continue; }

					// if the pool components set contains this component, skip
					// destroying it
					if (whisker_ss_contains(pool->component_ids_set, component_id.index))
					{
						/* printf("deferred component: skip destroying matching pool component %zu (%s) from entity %zu\n", component_id, whisker_ecs_e(ecs->entities, component_id)->name, action->id); */
						continue;
					}

					/* printf("deferred component: destroying non-matching pool component %zu (%s) from entity %zu\n", component_id, whisker_ecs_e(ecs->entities, component_id)->name, action->id); */

					whisker_ecs_create_deferred_component_action(ecs->world, component_id, 0, action->id, NULL, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE);
				}

				if (ecs->world->entities->entities[action->id.index].destroyed)
				{
					ecs->world->entities->entities[action->id.index].destroyed = false;
					whisker_ecs_p_return_entity(pool, action->id);
				}

				continue;
			}

			/* whisker_ecs_c_remove_all_components(ecs->components, action->id); */
			whisker_ecs_create_deferred_component_action(ecs->world, action->id, 0, action->id, NULL, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE_ALL);

		}
	}
}

void whisker_ecs_update_process_deferred_entity_actions_(whisker_ecs *ecs)
{
	while (ecs->world->entities->deferred_actions_length > 0) 
	{
		whisker_ecs_entity_deferred_action action = ecs->world->entities->deferred_actions[--ecs->world->entities->deferred_actions_length];

		whisker_ecs_entity *e = whisker_ecs_e(ecs->world->entities, action.id);

		switch (action.action) {
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE:
				ecs->world->entities->entities[action.id.index].destroyed = false;		
				break;

			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY:
				// exclude managed entities from being directly destroyed
				if (e->managed_by != NULL)
				{
					continue;
				}
				else
				{
					whisker_ecs_e_destroy(ecs->world->entities, action.id);
				}
		}
	}
}

void whisker_ecs_update_process_deferred_component_actions_(whisker_ecs *ecs)
{
	// process the deferred actions into real component actions
	if (ecs->world->components->deferred_actions_length > 0) 
	{
		for (int i = 0; i < ecs->world->components->deferred_actions_length; ++i)
		{
			struct whisker_ecs_component_deferred_action action = ecs->world->components->deferred_actions[i];

			switch (action.action) {
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET:
					whisker_ecs_c_set_component(ecs->world->components, action.component_id, action.data_size, action.entity_id, ecs->world->components->deferred_actions_data + action.data_offset);
					break;
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE:
					whisker_ecs_c_remove_component(ecs->world->components, action.component_id, action.entity_id);
					break;
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE_ALL:
					whisker_ecs_c_remove_all_components(ecs->world->components, action.entity_id);
					break;
				default:
					break;
			}
		}

		ecs->world->components->deferred_actions_length = 0;
		ecs->world->components->deferred_actions_data_length = 0;
	}
}

void whisker_ecs_update_process_changed_components_(whisker_ecs *ecs)
{
	whisker_arr_ensure_alloc(ecs->component_sort_requests, ecs->world->components->component_ids_length);

	for (int i = 0; i < ecs->world->components->component_ids_length; ++i)
	{
		whisker_ecs_entity_id component_id = ecs->world->components->component_ids[i];
		if (!ecs->world->components->components[component_id.index]->mutations_length)
		{
			continue;
		}

		size_t sort_request_idx = ecs->component_sort_requests_length++;
		ecs->component_sort_requests[sort_request_idx].components = ecs->world->components;
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
whisker_ecs_entity_id whisker_ecs_create_entity(struct whisker_ecs_world *world)
{
	return whisker_ecs_e_create(world->entities);
}

// request an entity ID to be created or recycled, providing a name
// note: names are unique, creating an entity with the same name returns the existing entity if it exists
whisker_ecs_entity_id whisker_ecs_create_named_entity(struct whisker_ecs_world *world, char* name)
{
	return whisker_ecs_e_create_named(world->entities, name);
}

// immediately destroy the given entity ID
void whisker_ecs_destroy_entity(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity *e = whisker_ecs_e(world->entities, entity_id);
	if (e->managed_by != NULL)
	{
		// HACK: for now we assume everything managed is managed by a pool
		whisker_ecs_pool *pool = e->managed_by;
		whisker_ecs_p_return_entity(pool, entity_id);
		return;
	}

	whisker_ecs_e_destroy(world->entities, entity_id);
}

// immediately soft-destroy the given entity ID with an atomic operation
void whisker_ecs_soft_destroy_entity(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_e_make_unmanaged(world->entities, entity_id);
}

// immediately soft-revive the given entity ID with an atomic operation
void whisker_ecs_soft_revive_entity(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_e_make_managed(world->entities, entity_id);
}

// check whether the given entity ID is still alive
// note: this performs a version check using the full entity 64 bit ID
bool whisker_ecs_is_alive(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_e_is_alive(world->entities, entity_id);
}

// request to create an entity, deferring the creation until end of current frame
whisker_ecs_entity_id whisker_ecs_create_entity_deferred(struct whisker_ecs_world *world)
{
	return whisker_ecs_e_create_deferred(world->entities);
}

// request to create an entity with a name, deferring the creation until end of current frame
whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(struct whisker_ecs_world *world, char* name)
{
	return whisker_ecs_e_create_named_deferred(world->entities, name);
}

// request to destroy the provided entity ID at the end of current frame
void whisker_ecs_destroy_entity_deferred(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_e_destroy_deferred(world->entities, entity_id);
	return;
}




/*************************
*  component functions  *
*************************/
// get the component entity ID for the given component name
whisker_ecs_entity_id whisker_ecs_component_id(struct whisker_ecs_world *world, char* component_name)
{
	return whisker_ecs_e_named(world->entities, component_name)->id;
}

// get a named component for an entity
// note: the component has to exist on the entity first
void *whisker_ecs_get_named_component(struct whisker_ecs_world *world, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(world, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}

	return whisker_ecs_get_component(world, component_id, entity_id);
}

// set a named component on an entity
// note: this will handle the creation of the underlying component array
void *whisker_ecs_set_named_component(struct whisker_ecs_world *world, char *component_name, size_t component_size, whisker_ecs_entity_id entity_id, void *value)
{
	whisker_ecs_entity_id component_id = whisker_ecs_e_create_named(world->entities, component_name);;
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}
	return whisker_ecs_set_component(world, component_id, component_size, entity_id, value);
}

// remove a named component from an entity
void whisker_ecs_remove_named_component(struct whisker_ecs_world *world, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(world, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		return;
	}

	whisker_ecs_remove_component(world, component_id, entity_id);
}

// check whether an entity has a named component attached
bool whisker_ecs_has_named_component(struct whisker_ecs_world *world, char *component_name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_id component_id = whisker_ecs_component_id(world, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}
	return whisker_ecs_has_component(world, component_id, entity_id);
}

// get the component by ID for the given entity
// note: the component has to exist on the entity
void *whisker_ecs_get_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_c_get_component(world->components, component_id, entity_id);
}

// set the component by ID on the given entity
// note: this will handle the creation of the underlying component array
void *whisker_ecs_set_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value)
{
	whisker_ecs_create_deferred_component_action(
		world,
		component_id,
		component_size,
		entity_id,
		value,
		WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET
	);

	return value;
}

// remove the component by ID from the given entity
void whisker_ecs_remove_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_create_deferred_component_action(
		world,
		component_id,
		0,
		entity_id,
		NULL,
		WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE
	);
}

// check if an entity has the given component by ID
bool whisker_ecs_has_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	return whisker_ecs_c_has_component(world->components, component_id, entity_id);
}

// create a deferred component action to be processed later
void whisker_ecs_create_deferred_component_action(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value, enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action)
{
	whisker_ecs_c_create_deferred_action(world->components, component_id, entity_id, action, value, component_size, true);
}

/**********************
*  built-in systems  *
**********************/

// this system ensures the process phase WHISKER_ECS_PROCESS_PHASE_ON_STARTUP
// gets disabled after the first frame along with this system
void whisker_ecs_system_deregister_startup_phase(whisker_ecs_system_context *context)
{
	// destroy the process phase entity, and destroy this system's entity
	whisker_ecs_entity *e = whisker_ecs_e_named(context->world->entities, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP);

	if (e)
	{
		debug_log(DEBUG, ecs:system_deregister_startup_phase, "de-registering startup phase entity %d", e->id.index);

		whisker_ecs_soft_destroy_entity(context->world, e->id);

		// destroy system entity to prevent running it again
		whisker_ecs_soft_destroy_entity(context->world, context->system_entity_id);
	}
}


/****************
*  ECS entity  *
****************/

/*************************************
*  entities struct management  *
*************************************/
// create an instance of entities container
whisker_ecs_entities *whisker_ecs_create_and_init_entities_container_()
{
	whisker_ecs_entities *e = whisker_ecs_create_entities_container_();
	whisker_ecs_init_entities_container_(e);

	return e;
}

// allocate instance of entities container
whisker_ecs_entities *whisker_ecs_create_entities_container_()
{
	whisker_ecs_entities *e = whisker_mem_xcalloc_t(1, *e);
	return e;
}

// init entity arrays for an entity container
void whisker_ecs_init_entities_container_(whisker_ecs_entities *entities)
{
	// create and allocate entity arrays
	whisker_arr_init_t(
		entities->entities, 
		WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE
	);
	whisker_arr_init_t(
		entities->destroyed_entities, 
		WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE
	);
	whisker_arr_init_t(
		entities->deferred_actions,
		WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
	);

	// create entity names trie
	entities->entity_names = whisker_mem_xcalloc_t(1, *entities->entity_names);

	// init pthread mutexes
	pthread_mutex_init(&entities->deferred_actions_mutex, NULL);
	pthread_mutex_init(&entities->create_entity_mutex, NULL);
}

// free arrays on instance of entities container
void whisker_ecs_free_entities_container_(whisker_ecs_entities *entities)
{
	// free entity name strings
	for (int i = 0; i < entities->entities_length; ++i)
	{
		if (entities->entities[i].name != NULL)
		{
			free_null(entities->entities[i].name);
		}
	}

	free(entities->entities);
	free(entities->destroyed_entities);
	whisker_trie_free_node_values(entities->entity_names);
	whisker_trie_free_nodes(entities->entity_names);
	free(entities->entity_names);
	free(entities->deferred_actions);
	pthread_mutex_destroy(&entities->deferred_actions_mutex);
	pthread_mutex_destroy(&entities->create_entity_mutex);
}

// free entity container and containing arrays
void whisker_ecs_free_entities_all_(whisker_ecs_entities *entities)
{
	whisker_ecs_free_entities_container_(entities);
	free(entities);
}

/***********************
*  entity management  *
***********************/
// request a new entity
whisker_ecs_entity_id whisker_ecs_e_create(whisker_ecs_entities *entities)
{
	pthread_mutex_lock(&entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_e_create_(entities);

	pthread_mutex_unlock(&entities->create_entity_mutex);

	return e;
}

// create a new entity as a deferred action
whisker_ecs_entity_id whisker_ecs_e_create_deferred(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_id e = whisker_ecs_e_create(entities);
	if (e.id == 0)
	{
		return e;
	}

	// set the entity to dead and add it to the deferred entities
	entities->entities[e.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	return e;
}

// create a new entity as a deferred action without using previously recycled entities
whisker_ecs_entity_id whisker_ecs_e_create_new_deferred(whisker_ecs_entities *entities)
{
	pthread_mutex_lock(&entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_e_create_new_(entities);

	pthread_mutex_unlock(&entities->create_entity_mutex);

	// set the entity to dead and add it to the deferred entities
	entities->entities[e.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	return e;
}

// creates and sets an entity, either new or recycled
whisker_ecs_entity_id whisker_ecs_e_create_(whisker_ecs_entities *entities)
{
	if (entities->destroyed_entities_length)
	{
		return entities->entities[whisker_ecs_e_pop_recycled_(entities)].id;
	}
	return whisker_ecs_e_create_new_(entities);
}

// pop a recycled entity from the destroyed_entities stack
whisker_ecs_entity_index whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities)
{
	if (entities->destroyed_entities_length > 0)
	{
		whisker_ecs_entity_index recycled_index = --entities->destroyed_entities_length;
		entities->entities[recycled_index].destroyed = false;
		return entities->destroyed_entities[recycled_index];
	}

	return 0;
}

// create a new entity and add it to the entities list
whisker_ecs_entity_id whisker_ecs_e_create_new_(whisker_ecs_entities *entities)
{
	const size_t new_idx = entities->entities_length++;

	// reallocate the entity array if required
	whisker_arr_ensure_alloc_block_size(
		entities->entities, 
		(new_idx + 1), 
		WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE
	);

	// init the newly created entity with valid index
	entities->entities[new_idx] = (whisker_ecs_entity) {
		.id.index = new_idx,
	};
	return entities->entities[new_idx].id;
}

// set the name for an entity
void whisker_ecs_e_set_name(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_index *trie_id = whisker_mem_xcalloc_t(1, *trie_id);
	*trie_id = entity_id.index;
	whisker_trie_set_value_str(entities->entity_names, name, trie_id);

	// copy the name into the entities name
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	e->name = whisker_mem_xmalloc(strlen(name) + 1);
	strncpy(e->name, name, strlen(name) + 1);
}

// create entity with a name, or return an existing entity with the same name
whisker_ecs_entity_id whisker_ecs_e_create_named(whisker_ecs_entities *entities, char *name)
{
	pthread_mutex_lock(&entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_e_create_named_(entities, name);

	pthread_mutex_unlock(&entities->create_entity_mutex);

	return e;
}

// create a new named entity as a deferred action
whisker_ecs_entity_id whisker_ecs_e_create_named_deferred(whisker_ecs_entities *entities, char *name)
{
	pthread_mutex_lock(&entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_e_create_named_(entities, name);

	pthread_mutex_unlock(&entities->create_entity_mutex);
	if (e.id == 0)
	{
		return e;
	}

	// set the entity to dead and add it to the deferred entities
	entities->entities[e.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	return e;
}

// create a new entity with the given name
whisker_ecs_entity_id whisker_ecs_e_create_named_(whisker_ecs_entities *entities, char* name)
{
	whisker_ecs_entity *e = whisker_ecs_e_named(entities, name);
	if (e)
	{
		return e->id;
	}

	// create new entity with name
	whisker_ecs_entity_id e_id = whisker_ecs_e_create_(entities);

	// set the name
	whisker_ecs_e_set_name(entities, name, e_id);

	return e_id;
}

// recycle an entity into the destroyed entities stack
void whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	// increment version
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	e->id.version++;

	// clear name pointer
	if (e->name != NULL)
	{
		whisker_trie_remove_value_str(entities->entity_names, e->name);
		free_null(e->name);
		e->name = NULL;
	}

	// push to dead entities stack
	whisker_arr_ensure_alloc_block_size(
		entities->destroyed_entities, 
		(entities->destroyed_entities_length + 1), 
		WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE
	);
	entities->destroyed_entities[entities->destroyed_entities_length++] = e->id.index;
}

// destroy an entity, incrementing it's version and adding it to the destroyed
// entities stack
void whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	// mark entity as destroyed
	_Atomic bool currently_destroyed = atomic_load(&entities->entities[entity_id.index].destroyed);
	if (!currently_destroyed)
	{
		whisker_ecs_e_recycle(entities, entity_id);
    	atomic_store(&entities->entities[entity_id.index].destroyed, true);
	}
	return;
}

// destroy an entity as a deferred action
void whisker_ecs_e_destroy_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	_Atomic bool currently_destroyed = atomic_load(&entities->entities[entity_id.index].destroyed);
	if (!currently_destroyed)
	{
    	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = entity_id, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY});
    	atomic_store(&entities->entities[entity_id.index].destroyed, true);
	}
	return;
}

// set an entity to unmanaged state, and no longer included in iterations or
// recycled
void whisker_ecs_e_make_unmanaged(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
    atomic_store(&entities->entities[entity_id.index].unmanaged, true);
}

// set an entity to managed state
void whisker_ecs_e_make_managed(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
    atomic_store(&entities->entities[entity_id.index].unmanaged, false);
}

// add a deferred entity action to be processed at a later time
// note: typically it would be the end of an update
void whisker_ecs_e_add_deffered_action(whisker_ecs_entities *entities, whisker_ecs_entity_deferred_action action)
{
	size_t deferred_action_idx = atomic_fetch_add(&entities->deferred_actions_length, 1);

	if((deferred_action_idx + 1) * sizeof(*entities->deferred_actions) > entities->deferred_actions_size)
	{
		pthread_mutex_lock(&entities->deferred_actions_mutex);

		whisker_arr_ensure_alloc_block_size(
			entities->deferred_actions, 
			(deferred_action_idx + 1),
			WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
		);

		pthread_mutex_unlock(&entities->deferred_actions_mutex);
	}
	
	entities->deferred_actions[deferred_action_idx] = action;
}

// process the deferred actions stack
void whisker_ecs_e_process_deferred(whisker_ecs_entities *entities)
{
	while (entities->deferred_actions_length > 0) 
	{
		whisker_ecs_entity_deferred_action action = entities->deferred_actions[--entities->deferred_actions_length];

		switch (action.action) {
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE:
				entities->entities[action.id.index].destroyed = false;		
				break;
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY:
				whisker_ecs_e_destroy(entities, action.id);
				break;
		}
	}
}


/***********************
*  utility functions  *
***********************/

// shortcut to get the entity struct from the given ID
whisker_ecs_entity* whisker_ecs_e(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	if (entity_id.index + 1 > whisker_ecs_e_count(entities))
	{
		return NULL;
	}

	return &entities->entities[entity_id.index];
}

// convert a numeric ID to an entity ID
inline whisker_ecs_entity_id whisker_ecs_e_id(whisker_ecs_entity_id_raw id)
{
	return (whisker_ecs_entity_id){.id = id};
}

// shortcut to get the entity with the given name
// note: this will return NULL if no entity exists with this name
whisker_ecs_entity* whisker_ecs_e_named(whisker_ecs_entities *entities, char* entity_name)
{
	// lookup entity by name and return a match, or NULL
	whisker_ecs_entity_index *e_idx = whisker_trie_search_value_str(entities->entity_names, entity_name);
	if (e_idx == NULL)
	{
		return NULL;
	}

	return &entities->entities[*e_idx];
}

// check if an entity is dead by comparing the provided entity version with the
// one in the entities array
bool whisker_ecs_e_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	return (whisker_ecs_e(entities, entity_id)->id.version == entity_id.version);
}

// get the current count of entities in existence including alive and destroyed
size_t whisker_ecs_e_count(whisker_ecs_entities *entities)
{
	return entities->entities_length;
}

// get current count of alive entities
size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities)
{
	return whisker_ecs_e_count(entities) - whisker_ecs_e_destroyed_count(entities);
}

// get current count of destroyed entities
size_t whisker_ecs_e_destroyed_count(whisker_ecs_entities *entities)
{
	return entities->destroyed_entities_length;
}

// convert a string of named entities in the format "name1,name2,name3" to an
// array of entities, creating them if they don't already exist
struct whisker_ecs_entity_id_array* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names_str)
{
	// entity list derived from string entity names
	struct whisker_ecs_entity_id_array *entities_new = whisker_mem_xcalloc_t(1, *entities_new);
	whisker_arr_init_t(entities_new->arr, 1);

	size_t names_length = strlen(entity_names_str);
	char* entity_names = whisker_mem_xmalloc(names_length + 1);
	strncpy(entity_names, entity_names_str, names_length + 1);

	if (names_length == 0)
	{
		free_null(entity_names);
		return entities_new;
	}

	size_t search_index = 0;
	for (size_t i = 0; i < names_length + 1; ++i)
	{
		// if we reached the end of the string, or a separator
		// mutate to null terminator value
		bool mutated = false;
		if (entity_names[i] == ',') {
    		entity_names[i] = '\0';
    		mutated = true;
		}

		if (entity_names[i] == 0x0)
		{
			// create/get entity ID for name
			whisker_ecs_entity_id e = whisker_ecs_e_create_named(entities, entity_names + search_index);

			/* debug_printf("%zu-%zu: %s\n", search_index, i, entity_names + search_index); */

			// add the entity id to the final list, and reset name array
			whisker_arr_ensure_alloc(entities_new->arr, entities_new->arr_length + 1);
			entities_new->arr[entities_new->arr_length++] = e;

			search_index = i + 1;
			if (mutated)
			{
    			entity_names[i] = ',';
			}
			continue;
		}
	}

	free_null(entity_names);

	return entities_new;
}

// compare an entity to another entity
// note: used by the entity sort function
int whisker_ecs_e_compare_entity_ids_(const void *id_a, const void *id_b)
{
	return ((*(whisker_ecs_entity_id*)id_a).id - (*(whisker_ecs_entity_id*)id_b).id);
}

// sort an array of entities in ascending order
void whisker_ecs_e_sort_entity_array(whisker_ecs_entity_id *entities, size_t length)
{
	qsort(entities, length, sizeof(whisker_ecs_entity_id), whisker_ecs_e_compare_entity_ids_);
}


/*******************
*  ECS component  *
*******************/

// create instance of components container
whisker_ecs_components *whisker_ecs_c_create_components()
{
	whisker_ecs_components *c = whisker_mem_xcalloc(1, sizeof(*c));
	return c;
}

// init instance of components container
void whisker_ecs_c_init_components(whisker_ecs_components *components)
{
	// create array
	whisker_arr_init_t(
		components->components, 
		WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE
	);
	whisker_arr_init_t(
		components->component_ids, 
		WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE
	);

	// create deferred actions array
	whisker_arr_init_t(
		components->deferred_actions, 
		WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
	);
	components->deferred_actions_data = whisker_mem_xcalloc(1, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE);
	components->deferred_actions_data_size = WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE;

	// pthread mutexes
	pthread_mutex_init(&components->grow_components_mutex, NULL);
	pthread_mutex_init(&components->deferred_actions_mutex, NULL);
}

// create and init instance of components container
whisker_ecs_components *whisker_ecs_c_create_and_init_components()
{
	whisker_ecs_components *c = whisker_ecs_c_create_components();
	whisker_ecs_c_init_components(c);
	return c;
}

// free instance of components container and all component sets
void whisker_ecs_c_free_components_all(whisker_ecs_components *components)
{
	whisker_ecs_c_free_components(components);
	free(components);
}

// free component data in components container
void whisker_ecs_c_free_components(whisker_ecs_components *components)
{
	for (int i = 0; i < components->components_length; i++) {
		if (components->components[i] != NULL)
		{
			whisker_ss_free_all(components->components[i]);
		}
	}
	free(components->components);
	free(components->component_ids);
	pthread_mutex_destroy(&components->grow_components_mutex);
	pthread_mutex_destroy(&components->deferred_actions_mutex);
	free(components->deferred_actions);
	free(components->deferred_actions_data);
}

/******************************************
*  component array management functions  *
******************************************/
// allocate an empty component array
void whisker_ecs_c_create_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size)
{
	// grow the components array to fit the new component ID
	whisker_ecs_c_grow_components_(components, component_id.index + 1);

	// create array
	whisker_sparse_set *ss;
	debug_log(DEBUG, ecs:create_component_array, "creating component sparse set %zu (%zu total components) size %zu", component_id.id, components->component_ids_length + 1, component_size);
	ss = whisker_ss_create_s(component_size);

	whisker_arr_ensure_alloc_block_size(
		components->component_ids, 
		(components->component_ids_length + 1), 
		(WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE)
	);
	components->component_ids[components->component_ids_length++] = component_id;

	components->components[component_id.index] = ss;
}

// grow components array size if required to the nearest block size
void whisker_ecs_c_grow_components_(whisker_ecs_components *components, size_t capacity)
{
	if (components->components_length < capacity)
	{
		pthread_mutex_lock(&components->grow_components_mutex);

		whisker_arr_ensure_alloc_block_size(
			components->components, 
			(capacity), 
			(WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE)
		);
		if (capacity > components->components_length)
		{
			components->components_length = capacity;
		}

		pthread_mutex_unlock(&components->grow_components_mutex);
	}
}

// get the component array for the provided component ID 
// note: will create if it doesn't exist
whisker_sparse_set *whisker_ecs_c_get_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id)
{
	whisker_ecs_c_grow_components_(components, component_id.index + 1);
	return components->components[component_id.index];
}

// deallocate component array for the provided component ID
void whisker_ecs_c_free_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id)
{
	whisker_sparse_set* component_array = whisker_ecs_c_get_component_array(components, component_id);

	whisker_ss_free_all(component_array);
}

// sort the given component array's sparse set
// note: this is executed after a component has array has been modified
void whisker_ecs_c_sort_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id)
{
	whisker_ss_sort(whisker_ecs_c_get_component_array(components, component_id));
}


/******************************************
*  component deferred actions functions  *
******************************************/
// add a deferred component action to the queue
void whisker_ecs_c_create_deferred_action(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id, enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action, void *data, size_t data_size, bool propagate)
{
	// increment and fetch a stable deferred action index
	size_t deferred_action_idx = atomic_fetch_add(&components->deferred_actions_length, 1);

	if((deferred_action_idx + 1) * sizeof(*components->deferred_actions) > components->deferred_actions_size)
	{
		pthread_mutex_lock(&components->deferred_actions_mutex);

		whisker_arr_ensure_alloc_block_size(
			components->deferred_actions, 
			(deferred_action_idx + 1),
			WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
		);

		pthread_mutex_unlock(&components->deferred_actions_mutex);
	}

	components->deferred_actions[deferred_action_idx].component_id = component_id;
	components->deferred_actions[deferred_action_idx].entity_id = entity_id;
	components->deferred_actions[deferred_action_idx].data_size = data_size;
	components->deferred_actions[deferred_action_idx].data_offset = 0;
	components->deferred_actions[deferred_action_idx].action = action;
	components->deferred_actions[deferred_action_idx].propagate = propagate;

	if (data_size > 0)
	{
		size_t current_size_pos = atomic_fetch_add(&components->deferred_actions_data_length, data_size);

		// reallocate the deferred actions data array if required
		if(current_size_pos + data_size > components->deferred_actions_data_size)
		{
			pthread_mutex_lock(&components->deferred_actions_mutex);

			// double check to protect from stomping
			if(current_size_pos + data_size > components->deferred_actions_data_size)
			{
				components->deferred_actions_data = whisker_mem_xrecalloc(
					components->deferred_actions_data,
					components->deferred_actions_data_size,
					((current_size_pos + data_size) + WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE)
				);
				components->deferred_actions_data_size = (current_size_pos + data_size) + WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE;
			}

			pthread_mutex_unlock(&components->deferred_actions_mutex);
		}

		void *next_data_pointer = (char*)components->deferred_actions_data + current_size_pos;
		memcpy(next_data_pointer, data, data_size);
		components->deferred_actions[deferred_action_idx].data_offset = current_size_pos;
	}
}


/************************************
*  component management functions  *
************************************/
// get a component by ID for the given entity
void* whisker_ecs_c_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	// return component pointer
	return whisker_ss_get(
		whisker_ecs_c_get_component_array(components, component_id),
		entity_id.index
	);
}

// set a component by ID on the given entity
void whisker_ecs_c_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void* component)
{
	// grow array of sparse sets if required
	whisker_ecs_c_grow_components_(components, component_id.index + 1);

	// create a sparse set for the component if its null
	if (components->components_length < component_id.index + 1 || components->components[component_id.index] == NULL)
	{
		whisker_ecs_c_create_component_array(components, component_id, component_size);
	}

	// get the component array
	whisker_sparse_set* component_array = whisker_ecs_c_get_component_array(components, component_id);

	// set the component
	whisker_ss_set(component_array, entity_id.index, component);
}

// check if the provided entity has a component by ID
bool whisker_ecs_c_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	// get component array
	whisker_sparse_set* component_array = whisker_ecs_c_get_component_array(components, component_id);

	// return component pointer
	return component_array != NULL && whisker_ss_contains(component_array, entity_id.index);
}

// remove a component by ID from an entity
void whisker_ecs_c_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	// get component array
	whisker_sparse_set* component_array = whisker_ecs_c_get_component_array(components, component_id);

	// remove component
	whisker_ss_remove(component_array, entity_id.index);
}

// remove all of the components on an entity
void whisker_ecs_c_remove_all_components(whisker_ecs_components *components, whisker_ecs_entity_id entity_id)
{
	for (int ci = 0; ci < components->components_length; ++ci)
	{
		whisker_ecs_entity_id component_id = whisker_ecs_e_id(ci);

		if (components->components[ci] != NULL)
		{
			whisker_ecs_c_remove_component(components, component_id, entity_id);
		}
	}
}


/****************
*  ECS system  *
****************/

// create and init an instance of systems container
whisker_ecs_systems *whisker_ecs_s_create_and_init_systems()
{
	whisker_ecs_systems *s = whisker_mem_xcalloc(1, sizeof(*s));
	whisker_ecs_s_init_systems(s);

	return s;
}

// init an instance of systems container
void whisker_ecs_s_init_systems(whisker_ecs_systems *s)
{
	// create array for systems
	whisker_arr_init_t(s->systems, 1);

	// create array for process phase list
	whisker_arr_init_t(s->process_phases, 1);

	// create array for process phase time steps
	whisker_arr_init_t(s->process_phase_time_steps, 1);
}

// deallocate an instance of systems container and system data
void whisker_ecs_s_free_systems_all(whisker_ecs_systems *systems)
{
	whisker_ecs_s_free_systems(systems);
	free(systems);
}

// deallocate system data in instance of systems container
void whisker_ecs_s_free_systems(whisker_ecs_systems *systems)
{
	for (int i = 0; i < systems->systems_length; ++i)
	{
		whisker_ecs_s_free_system(&systems->systems[i]);
	}

	free(systems->systems);
	free(systems->process_phases);
	free(systems->process_phase_time_steps);
}


/******************************
*  system context functions  *
******************************/
// create and init an instance of a system context
whisker_ecs_system_context *whisker_ecs_s_create_and_init_system_context(whisker_ecs_system *system)
{
	whisker_ecs_system_context *c = whisker_mem_xcalloc(1, sizeof(*c));
	whisker_ecs_s_init_system_context(c, system);

	return c;
}

// init an instance of a system context
void whisker_ecs_s_init_system_context(whisker_ecs_system_context *context, whisker_ecs_system *system)
{
	// create iterators sparse set
	context->iterators = whisker_ss_create_t(whisker_ecs_system_iterator);

	// set system pointers
	context->world = system->world;
	context->process_phase_time_step = system->process_phase_time_step;
	context->system_entity_id = system->entity_id;

	context->delta_time = 0;
	context->thread_id = 0;
	context->thread_max = 0;
}

// deallocate data for system context instance
void whisker_ecs_s_free_system_context(whisker_ecs_system_context *context)
{
	if (context->iterators != NULL)
	{
		for (int i = 0; i < context->iterators->sparse_index_length; ++i)
		{
			whisker_ecs_system_iterator itor = ((whisker_ecs_system_iterator*)context->iterators->dense)[i];

			whisker_ecs_s_free_iterator(&itor);
		}

		whisker_ss_free_all(context->iterators);
	}
}

// deallocate a system context instance
void whisker_ecs_s_free_system_context_all(whisker_ecs_system_context *context)
{
	whisker_ecs_s_free_system_context(context);
	free(context);
}


/********************************
*  system operation functions  *
********************************/
// regiser a system in the systems container
whisker_ecs_system* whisker_ecs_s_register_system(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system system)
{

	// create contexts for the provided thread count
	// set thread count to 1 when it's set to 0
	if (system.thread_count == -1)
	{
		system.thread_count = whisker_tp_system_core_count();
	}
	debug_log(DEBUG, ecs:register_system, "sys e %zu creating %zu thread contexts", system.entity_id, system.thread_count);

	// create system context array
	whisker_arr_init_t(system.thread_contexts, (system.thread_count + 1));

	// create 1 extra context, which acts as the default context or first
	// thread's context
	for (int i = 0; i < system.thread_count + 1; ++i)
	{
		size_t thread_context_idx = system.thread_contexts_length++;
		whisker_ecs_s_init_system_context(&system.thread_contexts[thread_context_idx], &system);
		system.thread_contexts[thread_context_idx].thread_id = i;
		system.thread_contexts[thread_context_idx].thread_max = system.thread_count;
	}

	// create thread pool if we want threads for this system
	if (system.thread_count > 0)
	{
		system.thread_pool = whisker_tp_create_and_init(system.thread_count, system.world->entities->entities[system.entity_id.index].name);
	}
	
	// add system to main systems list
	whisker_arr_ensure_alloc(systems->systems, (systems->systems_length + 1));
	systems->systems[systems->systems_length++] = system;

	return &systems->systems[systems->systems_length - 1];
}

// deallocate a system instance
void whisker_ecs_s_free_system(whisker_ecs_system *system)
{
	for (int i = 0; i < system->thread_contexts_length; ++i)
	{
		whisker_ecs_s_free_system_context(&system->thread_contexts[i]);
	}

	free(system->thread_contexts);
	if (system->thread_pool)
	{
		whisker_tp_wait_work(system->thread_pool);
		whisker_tp_free_all(system->thread_pool);
	}
}



/*****************************
*  system update functions  *
*****************************/
static void system_update_process_phase(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context);
static int system_update_advance_process_phase_time_step(whisker_ecs_system_process_phase_time_step *time_step_container);
static whisker_ecs_system_iterator* system_update_get_system_iterator(whisker_ecs_system_context *default_context, int index, whisker_ecs_entities *entities);
static void system_update_queue_system_work(whisker_ecs_system *system);
static void system_update_execute_system(whisker_ecs_system *system);
static void system_update_set_context_values(whisker_ecs_system_context *context, whisker_ecs_system *system);
static void system_update_execute_system(whisker_ecs_system *system);
static void system_update_process_phase(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context);

// run an update on the registered systems
// note: processes all phases without deferred actions
void whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time)
{
    whisker_ecs_system *default_system = &systems->systems[systems->system_id];
    whisker_ecs_system_context *default_context = &default_system->thread_contexts[0];

    for (int i = 0; i < systems->process_phases_length; ++i)
    {
        whisker_ecs_system_process_phase *process_phase = &systems->process_phases[i];

        system_update_process_phase(systems, entities, process_phase, default_context);
    }
}

static int system_update_advance_process_phase_time_step(whisker_ecs_system_process_phase_time_step *time_step_container)
{
	if (!time_step_container->updated)
	{
		time_step_container->update_count = whisker_time_step_step_get_update_count(&time_step_container->time_step);
		time_step_container->updated = true;
	}

	return time_step_container->update_count;
}

static whisker_ecs_system_iterator* system_update_get_system_iterator(whisker_ecs_system_context *default_context, int index, whisker_ecs_entities *entities)
{
    return whisker_ecs_s_get_iterator(default_context, index, "w_ecs_system_idx", entities->entities[index].name, "");
}

static void system_update_set_context_values(whisker_ecs_system_context *context, whisker_ecs_system *system)
{
    context->process_phase_time_step = system->process_phase_time_step;
    context->delta_time = system->delta_time;
    context->system_ptr = system->system_ptr;
}

// reset updated state on all time steps
// this should be done at the end of the frame to allow triggering an advance
void whisker_ecs_s_reset_process_phase_time_steps(whisker_ecs_systems *systems)
{
	for (int i = 0; i < systems->process_phase_time_steps_length; ++i)
	{
		systems->process_phase_time_steps[i].updated = false;
	}
}

// update all systems in the given process phase
void whisker_ecs_s_update_process_phase(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context)
{
	system_update_process_phase(systems, entities, process_phase, default_context);
}

static void system_update_process_phase(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context)
{
    if (whisker_ecs_e(entities, process_phase->id)->unmanaged)
    {
    	return;
    }

    whisker_ecs_system_process_phase_time_step *time_step_container = &systems->process_phase_time_steps[process_phase->time_step_id];

    int update_count = system_update_advance_process_phase_time_step(time_step_container);

    for (int ui = 0; ui < update_count; ++ui)
    {
        whisker_ecs_system_iterator *system_itor = system_update_get_system_iterator(default_context, process_phase->id.index, entities);

        while (whisker_ecs_s_iterate(system_itor))
        {
            int *system_idx = whisker_ecs_itor_get(system_itor, 0);
            whisker_ecs_system *system = &systems->systems[*system_idx];

            system->delta_time = time_step_container->time_step.delta_time_fixed;
            system->process_phase_time_step = &time_step_container->time_step;

            /* printf("executing system %s frame count %zu\n", whisker_ecs_e(default_context->entities, system->entity_id)->name, system->process_phase_time_step->update_count); */

            if (system->thread_count > 0)
            {
                system_update_queue_system_work(system);
            }
            else
            {
                system_update_execute_system(system);
            }
        }
    }
}

static void system_update_execute_system(whisker_ecs_system *system)
{
    whisker_ecs_system_context *context = &system->thread_contexts[0];
    system_update_set_context_values(context, system);
    context->system_ptr(context);
}


static void system_update_queue_system_work(whisker_ecs_system *system)
{
    for (int ti = 0; ti < system->thread_count; ++ti)
    {
        whisker_ecs_system_context *context = &system->thread_contexts[ti];
        system_update_set_context_values(context, system);
        whisker_tp_queue_work(system->thread_pool, whisker_ecs_s_update_system_thread_, context);
    }
    whisker_tp_wait_work(system->thread_pool);
}


void whisker_ecs_s_update_system_thread_(void *context, whisker_thread_pool_context *t)
{
	whisker_ecs_system_context *system_context = context;
	system_context->system_ptr(system_context);
}

// update the provided system
void whisker_ecs_s_update_system(whisker_ecs_system *system, whisker_ecs_system_context *context)
{
	system->system_ptr(context);
}

// register a process phase time step
size_t whisker_ecs_s_register_process_phase_time_step(whisker_ecs_systems *systems, whisker_time_step time_step)
{
	whisker_ecs_system_process_phase_time_step time_step_container = {
		.time_step = time_step,
		.update_count = 0,
		.updated = false,
	};

	size_t idx = systems->process_phase_time_steps_length++;
	whisker_arr_ensure_alloc(systems->process_phase_time_steps, (idx + 1));

	systems->process_phase_time_steps[idx] = time_step_container;

	return idx;
}

// register a system process phase for the system scheduler
void whisker_ecs_s_register_process_phase(whisker_ecs_systems *systems, whisker_ecs_entity_id component_id, size_t time_step_id)
{
	whisker_ecs_system_process_phase process_phase = {
		.id = component_id,
		.time_step_id = time_step_id,
	};

	whisker_arr_ensure_alloc(systems->process_phases, (systems->process_phases_length + 1));
	systems->process_phases[systems->process_phases_length++] = process_phase;
}

// deregister the provided process phase by ID
// note: this will shift all process phases to fill the gap and maintain order
void whisker_ecs_s_deregister_process_phase(whisker_ecs_systems *systems, whisker_ecs_entity_id component_id)
{
	size_t index = -1;

	for (int i = 0; i < systems->process_phases_length; ++i)
	{
		if (systems->process_phases[i].id.id == component_id.id)
		{
			index = i;
		}
	}
	if (index > -1)
	{
    	for (int i = index; i < systems->process_phases_length - 1; i++)
    	{
        	systems->process_phases[i] = systems->process_phases[i + 1];
    	}

    	systems->process_phases_length--;
	}
}

// clear the current process phase list
// note: this does not remove the entities and components associated with the existing phases
void whisker_ecs_s_reset_process_phases(whisker_ecs_systems *systems)
{
    systems->process_phases_length = 0;
}


/*************************
*  iterator functions   *
*************************/
static bool itor_is_iteration_active(whisker_ecs_system_iterator *itor);
static void itor_increment_cursor(whisker_ecs_system_iterator *itor);
static void itor_set_master_components(whisker_ecs_system_iterator *itor);
static int itor_find_and_set_cursor_components(whisker_ecs_system_iterator *itor);
static void itor_update_master_index(whisker_ecs_system_iterator *itor);

// create and init an iterator instance
whisker_ecs_system_iterator *whisker_ecs_s_create_iterator()
{
	whisker_ecs_system_iterator *itor_new = whisker_mem_xcalloc_t(1, *itor_new);

	// create sparse sets for component pointers
	whisker_arr_init_t(itor_new->component_arrays, 1);
	whisker_arr_init_t(itor_new->component_arrays_cursors, 1);

	return itor_new;
}

// free instance of iterator and all data
void whisker_ecs_s_free_iterator(whisker_ecs_system_iterator *itor)
{
	free_null(itor->component_ids_rw);
	free_null(itor->component_ids_w);
	free_null(itor->component_ids_opt);
	free_null(itor->component_arrays);
	free_null(itor->component_arrays_cursors);
}

// get an iterator instance with the given itor_index
// note: this will init the iterator if one does not exist at the index
whisker_ecs_system_iterator *whisker_ecs_s_get_iterator(whisker_ecs_system_context *context, size_t itor_index, char *read_components, char *write_components, char *optional_components)
{
	whisker_ecs_system_iterator *itor;

	// check if iterator index is set
	if (!whisker_ss_contains(context->iterators, itor_index))
	{
		itor = whisker_ecs_s_create_iterator();
		whisker_ss_set(context->iterators, itor_index, itor);

		free(itor);
		itor = whisker_ss_get(context->iterators, itor_index);
		whisker_ecs_s_init_iterator(context, itor, read_components, write_components, optional_components);
	}

	itor = whisker_ss_get(context->iterators, itor_index);
	if (itor == NULL)
	{
		// TODO: panic here
		return NULL;
	}

	// reset iterator state
	itor->master_index = UINT64_MAX;
	itor->cursor = UINT64_MAX;
	itor->cursor_max = 0;
	itor->count = 0;
	itor->entity_id = whisker_ecs_e_id(UINT64_MAX);

	// find the master iterator by finding the smallest set
	for (int i = 0; i < itor->component_ids_rw_length; ++i)
	{
		whisker_sparse_set *component_array;
		if (itor->component_arrays[i] == NULL)
		{
			component_array = whisker_ecs_c_get_component_array(context->world->components, itor->component_ids_rw[i]);
			if (component_array == NULL)
			{
				itor->master_index = UINT64_MAX;
				break;
			}

			itor->component_arrays[i] = component_array;
		}
		else
		{
			component_array = itor->component_arrays[i];
		}

		if (component_array->sparse_index_length < itor->count || itor->count == 0)
		{
			itor->count = component_array->sparse_index_length;
			itor->cursor_max = itor->count;
			itor->master_index = i;
		}
	}

	/* debug_printf("ecs:sys:itor master selected: %zu count %zu components %zu\n", itor->master_index, itor->count, itor->component_ids->length); */

	// try to cache optional components when they are NULL
	for (int i = 0; i < itor->component_ids_opt_length; ++i)
	{
		whisker_sparse_set *component_array;
		size_t array_offset = itor->component_ids_rw_length + i;
		if (itor->component_arrays[array_offset] == NULL)
		{
			component_array = whisker_ecs_c_get_component_array(context->world->components, itor->component_ids_opt[i]);
			if (component_array == NULL)
			{
				continue;
			}

			itor->component_arrays[array_offset] = component_array;
		}
		itor->component_arrays_cursors[i] = 0;
	}

	// calculate cursor start and end point from thread context
	if (context->thread_max > 1)
	{
    	size_t context_thread_max = context->thread_max;
    	size_t thread_context_chunk_size = itor->count / context_thread_max;
    	itor->cursor = (context->thread_id * thread_context_chunk_size) - 1;
    	itor->cursor_max = (context->thread_id == context_thread_max - 1) ? itor->count : (context->thread_id * thread_context_chunk_size) + thread_context_chunk_size;

    	if (itor->cursor_max == itor->cursor + 1)
    	{
    		itor->count = 0;
    		itor->master_index = UINT64_MAX;
    		return itor;
    	}

    	/* if (context->thread_max > 1) { */
        /* 	debug_printf("itor: thread stats system %zu [t:%zu of %zu][cs:%zu m:%zu][c:%zu-%zu]\n", context->system_entity_id.id, context->thread_id + 1, context_thread_max, thread_context_chunk_size, itor->count, itor->cursor + 1, itor->cursor_max); */
    	/* } */
	}

	// if thread_max is uint64_max, assume we don't want to process anything
	// note: this is a hack added to allow an iterator to complete an
	// initialisation but ignore any matched entities
	else if (context->thread_max == UINT64_MAX)
	{
		itor->count = 0;
	}

	return itor;
}

// init the provided iterator and cache the given components
void whisker_ecs_s_init_iterator(whisker_ecs_system_context *context, whisker_ecs_system_iterator *itor, char *read_components, char *write_components, char *optional_components)
{
	itor->world = context->world;

	// convert read and write component names to component sparse sets
	char *combined_components;
	combined_components = whisker_mem_xmalloc(strlen(read_components) + strlen(write_components) + 2);
	if (combined_components == NULL)
	{
		return;
	}

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
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_e_from_named_entities(context->world->entities, combined_components);
		itor->component_ids_rw = e_arr->arr;
		itor->component_ids_rw_length = e_arr->arr_length;
		itor->component_ids_rw_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_rw == NULL)
		{
			// TODO: panic here
			free(combined_components);
			return;
		}

		whisker_arr_ensure_alloc(itor->component_arrays, itor->component_ids_rw_length);
		whisker_arr_ensure_alloc(itor->component_arrays_cursors, itor->component_ids_rw_length);
	}
	free(combined_components);

	// w components only includes write component IDs
	// note: these do not resize the main component arrays
	if (itor->component_ids_w == NULL)
	{
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_e_from_named_entities(context->world->entities, write_components);
		itor->component_ids_w = e_arr->arr;
		itor->component_ids_w_length = e_arr->arr_length;
		itor->component_ids_w_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_w == NULL)
		{
			// TODO: panic here
			return;
		}
	}

	// opt components only include optional component IDs
	// note: these belong at the end of the component arrays
	if (itor->component_ids_opt == NULL)
	{
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_e_from_named_entities(context->world->entities, optional_components);
		itor->component_ids_opt = e_arr->arr;
		itor->component_ids_opt_length = e_arr->arr_length;
		itor->component_ids_opt_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_opt == NULL)
		{
			// TODO: panic here
			return;
		}
		whisker_arr_ensure_alloc(itor->component_arrays, (itor->component_ids_rw_length + itor->component_ids_opt_length));
		whisker_arr_ensure_alloc(itor->component_arrays_cursors, (itor->component_ids_rw_length + itor->component_ids_opt_length));
	}
}


// step through an iterator incrementing the cursor
// note: returns false to indicate the iteration has reached the end
bool whisker_ecs_s_iterate(whisker_ecs_system_iterator *itor) 
{
    if (!itor_is_iteration_active(itor)) { return false; }

    itor_increment_cursor(itor);
    itor_set_master_components(itor);

    int cursor_state = itor_find_and_set_cursor_components(itor);
    itor_update_master_index(itor);

    return cursor_state == 0;
}

static bool itor_is_iteration_active(whisker_ecs_system_iterator *itor) {
    return (itor->master_index != UINT64_MAX && itor->count > 0);
}

static void itor_increment_cursor(whisker_ecs_system_iterator *itor) {
    itor->cursor++;
}

static void itor_set_master_components(whisker_ecs_system_iterator *itor) {
	while (itor->cursor < itor->cursor_max) 
	{
    	whisker_sparse_set *master_set = itor->component_arrays[itor->master_index];
    	itor->entity_id.id = master_set->sparse_index[itor->cursor];

		// skip entities marked as destroyed
		// note: if an entity is marked destroyed but still has components, then it's
		// an entity with destroyed state managed externally
        if (whisker_ecs_e(itor->world->entities, itor->entity_id)->unmanaged)
        {
        	itor_increment_cursor(itor);
        	continue;
        }

        break;
	}

}

static int itor_find_and_set_cursor_components(whisker_ecs_system_iterator *itor) {
    int cursor_state = 0;
    size_t rw_length = itor->component_ids_rw_length;
    for (int ci = 0; ci < rw_length; ++ci) {
        whisker_sparse_set *set = itor->component_arrays[ci];
        whisker_ecs_entity_id_raw cursor_entity = set->sparse_index[itor->cursor];


        if (cursor_entity == itor->entity_id.id) {
            itor->component_arrays_cursors[ci] = itor->cursor;
            cursor_state = 0;
            continue;
        }

        cursor_state = -1;
        size_t set_length = set->sparse_index_length;
        for (int i = itor->cursor; i < set_length; ++i) {
            cursor_entity = set->sparse_index[i];

            if (cursor_entity == itor->entity_id.id) {
            	itor->component_arrays_cursors[ci] = i;
                cursor_state = 0;
                break;
            }

            if (cursor_entity > itor->entity_id.id) {
                cursor_state = 1;
                break;
            }
        }

        if (cursor_state != 0) {
            break;
        }
    }
    return cursor_state;
}

static void itor_update_master_index(whisker_ecs_system_iterator *itor) {
    if (itor->cursor + 1 >= itor->cursor_max) {
        itor->master_index = UINT64_MAX;
    }
}


/**************
*  ECS pool  *
**************/

// create an instance of an entity pool
whisker_ecs_pool *whisker_ecs_p_create()
{
	return whisker_mem_xcalloc_t(1, whisker_ecs_pool);
}

// create and init an instance of an entity pool
whisker_ecs_pool *whisker_ecs_p_create_and_init(struct whisker_ecs_world *world, size_t count, size_t realloc_count)
{
	whisker_ecs_pool *pool = whisker_ecs_p_create();
	whisker_ecs_p_init(pool, world, count, realloc_count);

	return pool;
}

// init an instance of an entity pool
void whisker_ecs_p_init(whisker_ecs_pool *pool, struct whisker_ecs_world *world, size_t count, size_t realloc_count)
{
	whisker_arr_init_t(pool->component_ids, 1);
	whisker_arr_init_t(pool->entity_pool, count);
	pool->component_ids_set = whisker_ss_create_t(whisker_ecs_entity_id);

	pool->realloc_block_size = realloc_count;
	pool->inital_size = count;
	pool->stat_cache_misses = 0;

	// create prototype entity
	pool->prototype_entity_id = whisker_ecs_e_create(world->entities);
	whisker_ecs_e_make_unmanaged(world->entities, pool->prototype_entity_id);

	pthread_mutex_init(&pool->entity_pool_mutex, NULL);

	// force enable propagate changes
	pool->propagate_component_changes = true;

	pool->world = world;
}

// deallocate an entity pool's allocations
void whisker_ecs_p_free(whisker_ecs_pool *pool)
{
	free(pool->component_ids);
	free(pool->entity_pool);
	whisker_ss_free_all(pool->component_ids_set);
	pthread_mutex_destroy(&pool->entity_pool_mutex);
}

// deallocate and free an entity pool
void whisker_ecs_p_free_all(whisker_ecs_pool *pool)
{
	whisker_ecs_p_free(pool);
	free(pool);
}

// set a component on the prototype entity for a pool
void whisker_ecs_p_set_prototype_component_f(whisker_ecs_pool *pool, whisker_ecs_entity_id component_id, size_t component_size, void *prototype_value)
{
	if (!whisker_ss_contains(pool->component_ids_set, component_id.index))
	{
		// ensure space for component IDs
		size_t component_idx = atomic_fetch_add(&pool->component_ids_length, 1);
		
		if((component_idx + 1) * sizeof(*pool->component_ids) > pool->component_ids_size)
		{
			pthread_mutex_lock(&pool->entity_pool_mutex);

			whisker_arr_ensure_alloc_block_size(
				pool->component_ids, 
				(component_idx + 1),
				WHISKER_ECS_POOL_REALLOC_BLOCK_SIZE
			);

			pthread_mutex_unlock(&pool->entity_pool_mutex);
		}

		pool->component_ids[component_idx] = component_id;
		whisker_ss_set(pool->component_ids_set, component_id.index, &component_id);
	}

	// set component data
	whisker_ecs_c_set_component(pool->world->components, component_id, component_size, pool->prototype_entity_id, prototype_value);
}

// set a named component on the prototype entity for a pool
void whisker_ecs_p_set_prototype_named_component_f(whisker_ecs_pool *pool, char* component_name, size_t component_size, void *prototype_value)
{
	whisker_ecs_p_set_prototype_component_f(pool, whisker_ecs_e_create_named(pool->world->entities, component_name), component_size, prototype_value);
}

// set the prototype entity for this pool, clearing previously set component IDs
void whisker_ecs_p_set_prototype_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id prototype_entity_id)
{
	pool->component_ids_length = 0;
	pool->prototype_entity_id = prototype_entity_id;

	// TODO: build this into sparse set to allow easy reset
	whisker_ss_free_all(pool->component_ids_set);
	pool->component_ids_set = whisker_ss_create_t(whisker_ecs_entity_id);
}

// request an entity from the pool
whisker_ecs_entity_id whisker_ecs_p_request_entity(whisker_ecs_pool *pool)
{
	/* printf("pool %p entities in pool: ", pool); */
	/* for (int i = 0; i < pool->entity_pool_length; ++i) */
	/* { */
	/* 	printf("%zu ", pool->entity_pool[i]); */
	/* } */
	/* printf("\n"); */

	whisker_ecs_entity_id e;

	if (pool->entity_pool_length > 0)
	{
		size_t entity_idx = atomic_fetch_sub(&pool->entity_pool_length, 1) - 1;

		e = pool->entity_pool[entity_idx];
		/* debug_log(DEBUG, ecs:pool_request, "pool %p get from pool index %zu entity %zu\n", pool, entity_idx, e.id); */
	}	
	else
	{
		// create and init new entity
		e = whisker_ecs_p_create_entity_deferred(pool);

		atomic_fetch_add_explicit(&(pool->stat_cache_misses), 1, memory_order_relaxed);

		debug_log(DEBUG, ecs:pool_request, "pool %p create new entity %zu (requests %zu returns %zu misses %zu)", pool, e.id, pool->stat_total_requests, pool->stat_total_returns, pool->stat_cache_misses);

		// refill the pool
		whisker_ecs_p_realloc_entities(pool);
	}

	whisker_ecs_p_init_entity(pool, e, pool->propagate_component_changes);

	// increase stats
	atomic_fetch_add_explicit(&pool->stat_total_requests, 1, memory_order_relaxed);

	return e;
}

// issue a real create request for an entity this pool can use
whisker_ecs_entity_id whisker_ecs_p_create_entity_deferred(whisker_ecs_pool *pool)
{
	whisker_ecs_entity_id e = whisker_ecs_e_create_new_deferred(pool->world->entities);

	// make sure to perform the soft-recycle actions on the entity
	pool->world->entities->entities[e.index].unmanaged = true;
	pool->world->entities->entities[e.index].managed_by = pool;

	return e;
}

// initialise the entities components using the pool's prototype entity
void whisker_ecs_p_init_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id, bool propagate_component_changes)
{
	// copy component data from prototype entity
	for (size_t i = 0; i < pool->component_ids_length; ++i)
	{
		whisker_sparse_set *component_array = whisker_ecs_c_get_component_array(pool->world->components, pool->component_ids[i]);
		/* whisker_ss_set(component_array, entity_id.index, whisker_ss_get(component_array, pool->prototype_entity_id.index)); */
		whisker_ecs_c_create_deferred_action(pool->world->components, pool->component_ids[i], entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET, whisker_ss_get(component_array, pool->prototype_entity_id.index), component_array->element_size, false);

		// trigger a DUMMY_ADD on this entity
		whisker_ecs_c_create_deferred_action(pool->world->components, pool->component_ids[i], entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DUMMY_ADD, NULL, 0, propagate_component_changes);
	}

	// turn into managed entity
	whisker_ecs_e_make_managed(pool->world->entities, entity_id);
}

// de-initialise the entity and handle component actions
void whisker_ecs_p_deinit_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id, bool propagate_component_changes)
{
	// trigger DUMMY_REMOVE actions for each component
	for (size_t i = 0; i < pool->component_ids_length; ++i)
	{
		// trigger a DUMMY_REMOVE on this entity
		whisker_ecs_c_create_deferred_action(pool->world->components, pool->component_ids[i], entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DUMMY_REMOVE, NULL, 0, propagate_component_changes);
	}

	// turn into unmanaged entity
	whisker_ecs_e_make_unmanaged(pool->world->entities, entity_id);
}

// return an entity to the pool
void whisker_ecs_p_return_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id)
{
	// add the entity to the pool
	whisker_ecs_p_add_entity(pool, entity_id);

	/* debug_log(DEBUG, ecs:pool_return, "pool %p return entity %zu (requests %zu returns %zu misses %zu)", pool, entity_id, pool->stat_total_requests, pool->stat_total_returns, pool->stat_cache_misses); */

	whisker_ecs_p_deinit_entity(pool, entity_id, pool->propagate_component_changes);
	
	// increase stats
	atomic_fetch_add_explicit(&pool->stat_total_returns, 1, memory_order_relaxed);
}

// create new entities topping up the pool
void whisker_ecs_p_realloc_entities(whisker_ecs_pool *pool)
{
	/* debug_log(DEBUG, ecs:pool_realloc, "pool %p realloc entities block size %zu cache misses %zu\n", pool, pool->realloc_block_size, pool->cache_misses); */
	whisker_ecs_p_create_and_return(pool, (pool->stat_cache_misses <= 1) ? pool->inital_size : pool->realloc_block_size * pool->stat_cache_misses);
}

// create and add entity to the pool
void whisker_ecs_p_create_and_return(whisker_ecs_pool *pool, size_t count)
{
	for (int i = 0; i < count; ++i)
	{
    	whisker_ecs_entity_id e = whisker_ecs_p_create_entity_deferred(pool);
    	whisker_ecs_p_init_entity(pool, e, false);
    	whisker_ecs_p_deinit_entity(pool, e, false);
		whisker_ecs_p_add_entity(pool, e);
	}
}

// add an entity to the pool (this is not the same as returning an entity)
void whisker_ecs_p_add_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id)
{
	// grow array if required using lock
	size_t entity_idx = atomic_fetch_add(&pool->entity_pool_length, 1);

	if((entity_idx + 1) * sizeof(*pool->entity_pool) > pool->entity_pool_size)
	{
		pthread_mutex_lock(&pool->entity_pool_mutex);

		whisker_arr_ensure_alloc_block_size(
			pool->entity_pool, 
			(entity_idx + 1),
			WHISKER_ECS_POOL_REALLOC_BLOCK_SIZE
		);

		pthread_mutex_unlock(&pool->entity_pool_mutex);
	}

	pool->entity_pool[entity_idx] = entity_id;
	pool->world->entities->entities[entity_id.index].id.version++;
}


