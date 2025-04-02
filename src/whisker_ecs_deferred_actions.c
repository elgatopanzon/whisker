/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_deferred_actions
 * @created     : Tuesday Apr 01, 2025 19:54:00 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"

// process any deferred actions queued since the previous update
void whisker_ecs_update_process_deferred_actions_(whisker_ecs *ecs)
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
			whisker_ecs_entity *e = whisker_ecs_get_entity(ecs->world, action->id);
			if (e->managed_by != NULL)
			{ 
				whisker_ecs_pool *pool = e->managed_by;

				for (int ci = 0; ci < ecs->world->components->component_ids_length; ++ci)
				{
					whisker_ecs_entity_id component_id = ecs->world->components->component_ids[ci];
					if (!whisker_ecs_has_component(ecs->world, component_id, action->id)) { continue; }

					// if the pool components set contains this component, skip
					// destroying it
					if (whisker_ss_contains(pool->component_ids_set, component_id.index))
					{
						/* printf("deferred component: skip destroying matching pool component %zu (%s) from entity %zu\n", component_id, whisker_ecs_e(ecs->entities, component_id)->name, action->id); */
						continue;
					}

					/* printf("deferred component: destroying non-matching pool component %zu (%s) from entity %zu\n", component_id, whisker_ecs_e(ecs->entities, component_id)->name, action->id); */

					whisker_ecs_create_deferred_component_action_(ecs->world, component_id, 0, action->id, NULL, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE, true);
				}

				if (ecs->world->entities->entities[action->id.index].destroyed)
				{
					ecs->world->entities->entities[action->id.index].destroyed = false;
					whisker_ecs_return_pool_entity(pool, action->id);
				}

				continue;
			}

			whisker_ecs_create_deferred_component_action_(ecs->world, action->id, 0, action->id, NULL, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE_ALL, true);

		}
	}
}

void whisker_ecs_update_process_deferred_entity_actions_(whisker_ecs *ecs)
{
	while (ecs->world->entities->deferred_actions_length > 0) 
	{
		whisker_ecs_entity_deferred_action action = ecs->world->entities->deferred_actions[--ecs->world->entities->deferred_actions_length];

		whisker_ecs_entity *e = whisker_ecs_get_entity(ecs->world, action.id);

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
					whisker_ecs_entity_api_destroy_(ecs->world, action.id);
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
					whisker_ecs_set_component_(ecs->world, action.component_id, action.data_size, action.entity_id, ecs->world->components->deferred_actions_data + action.data_offset);
					break;
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE:
					whisker_ecs_remove_component_(ecs->world, action.component_id, action.entity_id);
					break;
				case WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE_ALL:
					whisker_ecs_remove_all_components_(ecs->world, action.entity_id);
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
		ecs->component_sort_requests[sort_request_idx].world = ecs->world;
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
	whisker_ecs_sort_component_array(sort_request->world, sort_request->component_id);

	sort_request->world->components->components[sort_request->component_id.index]->mutations_length = 0;
}

// create a deferred component action to be processed later
void whisker_ecs_create_deferred_component_action_(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value, enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action, bool propagate)
{
	// increment and fetch a stable deferred action index
	size_t deferred_action_idx = atomic_fetch_add(&world->components->deferred_actions_length, 1);

	if((deferred_action_idx + 1) * sizeof(*world->components->deferred_actions) > world->components->deferred_actions_size)
	{
		pthread_mutex_lock(&world->components->deferred_actions_mutex);

		whisker_arr_ensure_alloc_block_size(
			world->components->deferred_actions, 
			(deferred_action_idx + 1),
			WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
		);

		pthread_mutex_unlock(&world->components->deferred_actions_mutex);
	}

	world->components->deferred_actions[deferred_action_idx].component_id = component_id;
	world->components->deferred_actions[deferred_action_idx].entity_id = entity_id;
	world->components->deferred_actions[deferred_action_idx].data_size = component_size;
	world->components->deferred_actions[deferred_action_idx].data_offset = 0;
	world->components->deferred_actions[deferred_action_idx].action = action;
	world->components->deferred_actions[deferred_action_idx].propagate = propagate;

	if (component_size > 0)
	{
		size_t current_size_pos = atomic_fetch_add(&world->components->deferred_actions_data_length, component_size);

		// reallocate the deferred actions data array if required
		if(current_size_pos + component_size > world->components->deferred_actions_data_size)
		{
			pthread_mutex_lock(&world->components->deferred_actions_mutex);

			// double check to protect from stomping
			if(current_size_pos + component_size > world->components->deferred_actions_data_size)
			{
				world->components->deferred_actions_data = whisker_mem_xrecalloc(
					world->components->deferred_actions_data,
					world->components->deferred_actions_data_size,
					((current_size_pos + component_size) + WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE)
				);
				world->components->deferred_actions_data_size = (current_size_pos + component_size) + WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE;
			}

			pthread_mutex_unlock(&world->components->deferred_actions_mutex);
		}

		void *next_data_pointer = (char*)world->components->deferred_actions_data + current_size_pos;
		memcpy(next_data_pointer, value, component_size);
		world->components->deferred_actions[deferred_action_idx].data_offset = current_size_pos;
	}
}
