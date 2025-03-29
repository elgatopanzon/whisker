/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_component
 * @created     : Thursday Feb 13, 2025 17:59:26 CST
 */

#include "whisker_std.h"
#include "whisker_debug.h"
#include "whisker_sparse_set.h"

#include "whisker_ecs.h"

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
	debug_log(DEBUG, ecs:create_component_array, "creating component sparse set %zu size %zu", component_id.id, component_size);
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
