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
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_LOAD, WHISKER_ECS_PROCESS_PHASE_PRE_LOAD_RATE);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE, WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE, WHISKER_ECS_PROCESS_PHASE_ON_UPDATE_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, WHISKER_ECS_PROCESS_PHASE_ON_UPDATE_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_UPDATE, WHISKER_ECS_PROCESS_PHASE_POST_UPDATE_RATE);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_PHASE_FINAL_RATE);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_RENDER, WHISKER_ECS_PROCESS_PHASE_PRE_RENDER_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_RENDER, WHISKER_ECS_PROCESS_PHASE_ON_RENDER_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_RENDER, WHISKER_ECS_PROCESS_PHASE_POST_RENDER_RATE);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER, WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER_RATE);

	// register built-in systems
	whisker_ecs_register_system(new, whisker_ecs_system_deregister_startup_phase, "wecs_system_deregister_startup_phase", WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD);

	// create thread pool for general work tasks
	new->general_thread_pool = whisker_tp_create_and_init(0, "ecs_general_tasks");
	whisker_arr_init_t(new->component_sort_requests, 32);

	// register the event system's systems
	whisker_ecs_register_system(new, whisker_ecs_ev_system_cull_event_components, "wecs_system_cull_event_components", WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_THREADED_AUTO);
	whisker_ecs_register_system(new, whisker_ecs_ev_system_cull_events, "wecs_system_cull_events", WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_THREADED_AUTO);

	// create event entity pool
	new->events_entity_pool = whisker_ecs_p_create_and_init(new->components, new->entities, 128, 64);

	// disable propagation of changes to ensure that events don't trigger new
	// events
	new->events_entity_pool->propagate_component_changes = false;

	// set the event components for the prototype entity
	whisker_ecs_p_set_prototype_named_tag(new->events_entity_pool, t_event);
	struct whisker_ecs_event_cull_event_component cull = {};
	whisker_ecs_p_set_prototype_named_component(new->events_entity_pool, t_event_cull_data, cull, &cull);

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

	// free entity pool
	whisker_ecs_p_free_all(ecs->events_entity_pool);

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

// register a process phase for use by the system scheduler
// note: update_rate_sec set to 0 = uncapped processing with variable delta time
whisker_ecs_entity_id whisker_ecs_register_process_phase(whisker_ecs *ecs, char *phase_name, double update_rate_sec)
{
	whisker_ecs_entity_id component_id = whisker_ecs_create_named_entity(ecs->entities, phase_name);

	// add component ID to system's process phase list
	whisker_ecs_s_register_process_phase(ecs->systems, component_id, update_rate_sec);

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
				ecs->systems->process_phases[idx].time_step = process_phases_backup[pi].time_step;

				exists = true;
				break;
			}
		}

		// if it doesn't exist, create it using the defaults
		if (!exists)
		{
			debug_log(DEBUG, ecs:set_process_phase_order, "registering new phase %s", phase_names[i]);
			whisker_ecs_s_register_process_phase(ecs->systems, component_id, WHISKER_ECS_PROCESS_PHASE_DEFAULT_RATE);
		}
	}

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
        whisker_ecs_s_update_process_phase(ecs->systems, ecs->entities, &ecs->systems->process_phases[i], update_context);

		whisker_ecs_update_process_deferred_actions(ecs);
    }

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

	// generate events from deferred component actions
	whisker_ecs_update_generate_component_events_(ecs);

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

void whisker_ecs_update_generate_component_events_(whisker_ecs *ecs)
{
	// use deferred actions to create component events
	if (ecs->components->deferred_actions_length > 0) 
	{
		for (int i = 0; i < ecs->components->deferred_actions_length; ++i)
		{
			struct whisker_ecs_component_deferred_action *action = &ecs->components->deferred_actions[i];

			// holds the name of the event to create
			char* event_name; 
			char* event_name_target; 
			char* component_name = whisker_ecs_e(ecs->entities, action->component_id)->name;

			// skip actions with propagate disabled or any component containing
			if (!action->propagate || (component_name != NULL && strstr(component_name, "ev_") != NULL))
			{
				continue;
			}

			// the string "ev_" which is assumed to be an event
			// HACK: not sure how else to do this
			switch (action->action) {
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET:
					// determine if this is an add event or a changed event
					if (!whisker_ecs_has_component(ecs->components, action->component_id, action->entity_id))
					{
						asprintf(&event_name, "%s_ev_added", component_name);
						asprintf(&event_name_target, "%s_ev_added_to", component_name);
					}
					else
					{
						// compare the existing value to the new value, overwise
						// skip this action
						if (memcmp(ecs->components->deferred_actions_data + action->data_offset, whisker_ss_get(ecs->components->components[action->component_id.index], action->entity_id.index), action->data_size) == 0)
						{
							continue;
						}

						// if the memory is different consider it changed
						asprintf(&event_name, "%s_ev_changed", component_name);
						asprintf(&event_name_target, "%s_ev_changed_on", component_name);
					}
					break;

				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE:
					asprintf(&event_name, "%s_ev_removed", component_name);
					asprintf(&event_name_target, "%s_ev_removed_from", component_name);
					break;

				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DUMMY_ADD:
					asprintf(&event_name, "%s_ev_added", component_name);
					asprintf(&event_name_target, "%s_ev_added_to", component_name);
					break;
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DUMMY_REMOVE:
					asprintf(&event_name, "%s_ev_removed", component_name);
					asprintf(&event_name_target, "%s_ev_removed_from", component_name);
					break;
				default:
					continue;
					break;
			}

			// create and fire event on the entity
			whisker_ecs_entity_id event_entity = whisker_ecs_e_create_named(ecs->entities, event_name);
			whisker_ecs_entity_id event_entity_target = whisker_ecs_e_create_named(ecs->entities, event_name_target);


			// first fire the targetted event as a new event
			whisker_ecs_entity_id event_target_entity = action->entity_id;
			whisker_ecs_entity_id ev = whisker_ecs_ev_create_with_data(ecs->events_entity_pool, event_entity_target, whisker_ecs_entity_id, &event_target_entity);
			whisker_ecs_ev_fire(ecs->events_entity_pool, ev);

			/* printf("deferred component events: creating %s (%zu as %zu) event on entity %zu\n", event_name, event_entity, ev, action->entity_id); */

			// fire the normal event directly on the entity itself
			// note: events fired on the entity don't survive the entities
			// destruction
			whisker_ecs_ev_fire_on_f(ecs->events_entity_pool, event_entity, event_target_entity);

			free(event_name);
			free(event_name_target);
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
