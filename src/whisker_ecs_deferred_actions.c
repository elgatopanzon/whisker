/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_deferred_actions
 * @created     : Tuesday Apr 01, 2025 19:54:00 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"

// process any deferred actions queued since the previous update
void w_process_deferred_actions_(struct w_ecs *ecs)
{
	// pre-process destroyed entities
	w_pre_process_destroyed_entities_(ecs);

	// process deferred component actions
	w_process_deferred_component_actions_(ecs);

	// process and sort changed components
	w_process_changed_components_(ecs);
	
	// process entity actions
	w_process_deferred_entity_actions_(ecs);
}

void w_pre_process_destroyed_entities_(struct w_ecs *ecs)
{
	// for each deferred deleted entity remove all its components
	for (size_t i = 0; i < ecs->world->entities->deferred_actions_length; ++i)
	{
		struct w_entity_deferred_action *action = &ecs->world->entities->deferred_actions[i];

		if (action->action == W_ENTITY_DEFERRED_ACTION_DESTROY)
		{
			// HACK: get pool managing the entity and create a deferred removal
			// request for any entity not contained within the pool's components
			struct w_entity *e = w_get_entity(ecs->world, action->id);
			if (e->managed_by != NULL)
			{ 
				struct w_pool *pool = e->managed_by;

				for (int ci = 0; ci < ecs->world->components->component_ids_length; ++ci)
				{
					w_entity_id component_id = ecs->world->components->component_ids[ci];
					if (!w_has_component(ecs->world, component_id, action->id)) { continue; }

					// if the pool components set contains this component, skip
					// destroying it
					if (w_sparse_set_contains(pool->component_ids_set, component_id.index))
					{
						/* printf("deferred component: skip destroying matching pool component %zu (%s) from entity %zu\n", component_id, w_get_entity(ecs->entities, component_id)->name, action->id); */
						continue;
					}

					/* printf("deferred component: destroying non-matching pool component %zu (%s) from entity %zu\n", component_id, w_get_entity(ecs->entities, component_id)->name, action->id); */

					w_create_deferred_component_action_(ecs->world, component_id, 0, action->id, NULL, W_COMPONENT_DEFERRED_ACTION_REMOVE, true);
				}

				if (ecs->world->entities->entities[action->id.index].destroyed)
				{
					ecs->world->entities->entities[action->id.index].destroyed = false;
					w_return_pool_entity(pool, action->id);
				}

				continue;
			}

			w_create_deferred_component_action_(ecs->world, action->id, 0, action->id, NULL, W_COMPONENT_DEFERRED_ACTION_REMOVE_ALL, true);

		}
	}
}

void w_process_deferred_entity_actions_(struct w_ecs *ecs)
{
	while (ecs->world->entities->deferred_actions_length > 0) 
	{
		struct w_entity_deferred_action action = ecs->world->entities->deferred_actions[--ecs->world->entities->deferred_actions_length];

		struct w_entity *e = w_get_entity(ecs->world, action.id);

		switch (action.action) {
			case W_ENTITY_DEFERRED_ACTION_CREATE:
				ecs->world->entities->entities[action.id.index].destroyed = false;		
				break;

			case W_ENTITY_DEFERRED_ACTION_DESTROY:
				// exclude managed entities from being directly destroyed
				if (e->managed_by != NULL)
				{
					continue;
				}
				else
				{
					w_entity_api_destroy_(ecs->world, action.id);
				}
		}
	}
}

void w_process_deferred_component_actions_(struct w_ecs *ecs)
{
	// process the deferred actions into real component actions
	if (ecs->world->components->deferred_actions_length > 0) 
	{
		for (int i = 0; i < ecs->world->components->deferred_actions_length; ++i)
		{
			struct w_component_deferred_action action = ecs->world->components->deferred_actions[i];

			switch (action.action) {
				case W_COMPONENT_DEFERRED_ACTION_SET:
					w_set_component_(ecs->world, action.component_id, action.data_size, action.entity_id, ecs->world->components->deferred_actions_data + action.data_offset);
					break;
				case W_COMPONENT_DEFERRED_ACTION_REMOVE:
					w_remove_component_(ecs->world, action.component_id, action.entity_id);
					break;
				case W_COMPONENT_DEFERRED_ACTION_REMOVE_ALL:
					w_remove_all_components_(ecs->world, action.entity_id);
					break;
				default:
					break;
			}
		}

		ecs->world->components->deferred_actions_length = 0;
		ecs->world->components->deferred_actions_data_length = 0;
	}
}

void w_process_changed_components_(struct w_ecs *ecs)
{
	w_array_ensure_alloc(ecs->component_sort_requests, ecs->world->components->component_ids_length);

	for (int i = 0; i < ecs->world->components->component_ids_length; ++i)
	{
		w_entity_id component_id = ecs->world->components->component_ids[i];
		if (!ecs->world->components->components[component_id.index]->mutations_length)
		{
			continue;
		}

		size_t sort_request_idx = ecs->component_sort_requests_length++;
		ecs->component_sort_requests[sort_request_idx].world = ecs->world;
		ecs->component_sort_requests[sort_request_idx].component_id = component_id;

		w_thread_pool_queue_work(ecs->general_thread_pool, w_sort_component_thread_func_, &ecs->component_sort_requests[sort_request_idx]);
	}

	if (ecs->component_sort_requests_length)
	{
		w_thread_pool_wait_work(ecs->general_thread_pool);
    	ecs->component_sort_requests_length = 0;
	}
}

void w_sort_component_thread_func_(void *component_sort_request, w_thread_pool_context *t)
{
	struct w_component_sort_request *sort_request = component_sort_request;
	w_sort_component_array(sort_request->world, sort_request->component_id);

	sort_request->world->components->components[sort_request->component_id.index]->mutations_length = 0;
}

// create a deferred component action to be processed later
void w_create_deferred_component_action_(struct w_world *world, w_entity_id component_id, size_t component_size, w_entity_id entity_id, void *value, enum W_COMPONENT_DEFERRED_ACTION action, bool propagate)
{
	// increment and fetch a stable deferred action index
	size_t deferred_action_idx = atomic_fetch_add(&world->components->deferred_actions_length, 1);

	if((deferred_action_idx + 1) * sizeof(*world->components->deferred_actions) > world->components->deferred_actions_size)
	{
		pthread_mutex_lock(&world->components->deferred_actions_mutex);

		w_array_ensure_alloc_block_size(
			world->components->deferred_actions, 
			(deferred_action_idx + 1),
			W_COMPONENT_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
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
				world->components->deferred_actions_data = w_mem_xrecalloc(
					world->components->deferred_actions_data,
					world->components->deferred_actions_data_size,
					((current_size_pos + component_size) + W_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE)
				);
				world->components->deferred_actions_data_size = (current_size_pos + component_size) + W_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE;
			}

			pthread_mutex_unlock(&world->components->deferred_actions_mutex);
		}

		void *next_data_pointer = (char*)world->components->deferred_actions_data + current_size_pos;
		memcpy(next_data_pointer, value, component_size);
		world->components->deferred_actions[deferred_action_idx].data_offset = current_size_pos;
	}
}
