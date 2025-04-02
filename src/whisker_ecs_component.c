/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_component
 * @created     : Tuesday Apr 01, 2025 19:52:31 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"


/*************************
*  component functions  *
*************************/
// get the component entity ID for the given component name
whisker_ecs_entity_id whisker_ecs_component_id(struct whisker_ecs_world *world, char* component_name)
{
	return whisker_ecs_create_named_entity(world, component_name);
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
	whisker_ecs_entity_id component_id = whisker_ecs_entity_api_create_named_(world, component_name);;
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
	// return component pointer
	return whisker_ss_get(
		whisker_ecs_get_component_array(world, component_id),
		entity_id.index
	);
}

// set the component by ID on the given entity
// note: this will handle the creation of the underlying component array
void *whisker_ecs_set_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value)
{
	whisker_ecs_create_deferred_component_action_(
		world,
		component_id,
		component_size,
		entity_id,
		value,
		WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET,
		true
	);

	return value;
}

// remove the component by ID from the given entity
void whisker_ecs_remove_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_create_deferred_component_action_(
		world,
		component_id,
		0,
		entity_id,
		NULL,
		WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE,
		true
	);
}

// check if an entity has the given component by ID
bool whisker_ecs_has_component(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	// get component array
	whisker_sparse_set* component_array = whisker_ecs_get_component_array(world, component_id);

	// return component pointer
	return component_array != NULL && whisker_ss_contains(component_array, entity_id.index);
}


/*************************************
*  components container management  *
*************************************/

// create instance of components container
whisker_ecs_components *whisker_ecs_create_components_container()
{
	whisker_ecs_components *c = whisker_mem_xcalloc(1, sizeof(*c));
	return c;
}

// init instance of components container
void whisker_ecs_init_components_container(whisker_ecs_components *components)
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
whisker_ecs_components *whisker_ecs_create_and_init_components_container()
{
	whisker_ecs_components *c = whisker_ecs_create_components_container();
	whisker_ecs_init_components_container(c);
	return c;
}

// free instance of components container and all component sets
void whisker_ecs_free_components_container_all(whisker_ecs_components *components)
{
	whisker_ecs_free_components_container(components);
	free(components);
}

// free component data in components container
void whisker_ecs_free_components_container(whisker_ecs_components *components)
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
void whisker_ecs_create_component_array(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, size_t component_size)
{
	// grow the components array to fit the new component ID
	whisker_ecs_grow_components_container_(world, component_id.index + 1);

	// create array
	whisker_sparse_set *ss;
	debug_log(DEBUG, ecs:create_component_array, "creating component sparse set %zu (%zu total components) size %zu", component_id.id, world->components->component_ids_length + 1, component_size);
	ss = whisker_ss_create_s(component_size);

	whisker_arr_ensure_alloc_block_size(
		world->components->component_ids, 
		(world->components->component_ids_length + 1), 
		(WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE)
	);
	world->components->component_ids[world->components->component_ids_length++] = component_id;

	world->components->components[component_id.index] = ss;
}

// grow components array size if required to the nearest block size
void whisker_ecs_grow_components_container_(struct whisker_ecs_world *world, size_t capacity)
{
	if (world->components->components_length < capacity)
	{
		pthread_mutex_lock(&world->components->grow_components_mutex);

		whisker_arr_ensure_alloc_block_size(
			world->components->components, 
			(capacity), 
			(WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE)
		);
		if (capacity > world->components->components_length)
		{
			world->components->components_length = capacity;
		}

		pthread_mutex_unlock(&world->components->grow_components_mutex);
	}
}

// get the component array for the provided component ID 
// note: will create if it doesn't exist
whisker_sparse_set *whisker_ecs_get_component_array(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id)
{
	whisker_ecs_grow_components_container_(world, component_id.index + 1);
	return world->components->components[component_id.index];
}

// sort the given component array's sparse set
// note: this is executed after a component has array has been modified
void whisker_ecs_sort_component_array(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id)
{
	whisker_ss_sort(whisker_ecs_get_component_array(world, component_id));
}


/******************************
*  component management API  *
******************************/

// set a component by ID on the given entity
void whisker_ecs_set_component_(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void* component)
{
	// grow array of sparse sets if required
	whisker_ecs_grow_components_container_(world, component_id.index + 1);

	// create a sparse set for the component if its null
	if (world->components->components_length < component_id.index + 1 || world->components->components[component_id.index] == NULL)
	{
		whisker_ecs_create_component_array(world, component_id, component_size);
	}

	// get the component array
	whisker_sparse_set* component_array = whisker_ecs_get_component_array(world, component_id);

	// set the component
	whisker_ss_set(component_array, entity_id.index, component);
}

// remove a component by ID from an entity
void whisker_ecs_remove_component_(struct whisker_ecs_world *world, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	// get component array
	whisker_sparse_set* component_array = whisker_ecs_get_component_array(world, component_id);

	// remove component
	whisker_ss_remove(component_array, entity_id.index);
}

// remove all of the components on an entity
void whisker_ecs_remove_all_components_(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	for (int ci = 0; ci < world->components->components_length; ++ci)
	{
		whisker_ecs_entity_id component_id = whisker_ecs_entity_id_from_raw(ci);

		if (world->components->components[ci] != NULL)
		{
			whisker_ecs_remove_component_(world, component_id, entity_id);
		}
	}
}

