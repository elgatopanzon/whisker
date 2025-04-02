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
w_entity_id w_component_id(struct w_world *world, char* component_name)
{
	return w_create_named_entity(world, component_name);
}

// get a named component for an entity
// note: the component has to exist on the entity first
void *w_get_named_component(struct w_world *world, char *component_name, w_entity_id entity_id)
{
	w_entity_id component_id = w_component_id(world, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}

	return w_get_component(world, component_id, entity_id);
}

// set a named component on an entity
// note: this will handle the creation of the underlying component array
void *w_set_named_component(struct w_world *world, char *component_name, size_t component_size, w_entity_id entity_id, void *value)
{
	w_entity_id component_id = w_entity_api_create_named_(world, component_name);;
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}
	return w_set_component(world, component_id, component_size, entity_id, value);
}

// remove a named component from an entity
void w_remove_named_component(struct w_world *world, char *component_name, w_entity_id entity_id)
{
	w_entity_id component_id = w_component_id(world, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		return;
	}

	w_remove_component(world, component_id, entity_id);
}

// check whether an entity has a named component attached
bool w_has_named_component(struct w_world *world, char *component_name, w_entity_id entity_id)
{
	w_entity_id component_id = w_component_id(world, component_name);
	if (component_id.id == 0)
	{
		// TODO: panic here
		// for now just return a NULL
		return NULL;
	}
	return w_has_component(world, component_id, entity_id);
}

// get the component by ID for the given entity
// note: the component has to exist on the entity
void *w_get_component(struct w_world *world, w_entity_id component_id, w_entity_id entity_id)
{
	// return component pointer
	return w_sparse_set_get(
		w_get_component_array(world, component_id),
		entity_id.index
	);
}

// set the component by ID on the given entity
// note: this will handle the creation of the underlying component array
void *w_set_component(struct w_world *world, w_entity_id component_id, size_t component_size, w_entity_id entity_id, void *value)
{
	w_create_deferred_component_action_(
		world,
		component_id,
		component_size,
		entity_id,
		value,
		W_COMPONENT_DEFERRED_ACTION_SET,
		true
	);

	return value;
}

// remove the component by ID from the given entity
void w_remove_component(struct w_world *world, w_entity_id component_id, w_entity_id entity_id)
{
	w_create_deferred_component_action_(
		world,
		component_id,
		0,
		entity_id,
		NULL,
		W_COMPONENT_DEFERRED_ACTION_REMOVE,
		true
	);
}

// check if an entity has the given component by ID
bool w_has_component(struct w_world *world, w_entity_id component_id, w_entity_id entity_id)
{
	// get component array
	w_sparse_set* component_array = w_get_component_array(world, component_id);

	// return component pointer
	return component_array != NULL && w_sparse_set_contains(component_array, entity_id.index);
}


/*************************************
*  components container management  *
*************************************/

// create instance of components container
struct w_components *w_create_components_container()
{
	struct w_components *c = w_mem_xcalloc(1, sizeof(*c));
	return c;
}

// init instance of components container
void w_init_components_container(struct w_components *components)
{
	// create array
	w_array_init_t(
		components->components, 
		W_COMPONENT_SET_REALLOC_BLOCK_SIZE
	);
	w_array_init_t(
		components->component_ids, 
		W_COMPONENT_SET_REALLOC_BLOCK_SIZE
	);

	// create deferred actions array
	w_array_init_t(
		components->deferred_actions, 
		W_COMPONENT_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
	);
	components->deferred_actions_data = w_mem_xcalloc(1, W_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE);
	components->deferred_actions_data_size = W_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE;

	// pthread mutexes
	pthread_mutex_init(&components->grow_components_mutex, NULL);
	pthread_mutex_init(&components->deferred_actions_mutex, NULL);
}

// create and init instance of components container
struct w_components *w_create_and_init_components_container()
{
	struct w_components *c = w_create_components_container();
	w_init_components_container(c);
	return c;
}

// free instance of components container and all component sets
void w_free_components_container_all(struct w_components *components)
{
	w_free_components_container(components);
	free(components);
}

// free component data in components container
void w_free_components_container(struct w_components *components)
{
	for (int i = 0; i < components->components_length; i++) {
		if (components->components[i] != NULL)
		{
			w_sparse_set_free_all(components->components[i]);
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
void w_create_component_array(struct w_world *world, w_entity_id component_id, size_t component_size)
{
	// grow the components array to fit the new component ID
	w_grow_components_container_(world, component_id.index + 1);

	// create array
	w_sparse_set *ss;
	debug_log(DEBUG, ecs:create_component_array, "creating component sparse set %zu (%zu total components) size %zu", component_id.id, world->components->component_ids_length + 1, component_size);
	ss = w_sparse_set_create_s(component_size);

	w_array_ensure_alloc_block_size(
		world->components->component_ids, 
		(world->components->component_ids_length + 1), 
		(W_COMPONENT_SET_REALLOC_BLOCK_SIZE)
	);
	world->components->component_ids[world->components->component_ids_length++] = component_id;

	world->components->components[component_id.index] = ss;
}

// grow components array size if required to the nearest block size
void w_grow_components_container_(struct w_world *world, size_t capacity)
{
	if (world->components->components_length < capacity)
	{
		pthread_mutex_lock(&world->components->grow_components_mutex);

		w_array_ensure_alloc_block_size(
			world->components->components, 
			(capacity), 
			(W_COMPONENT_SET_REALLOC_BLOCK_SIZE)
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
w_sparse_set *w_get_component_array(struct w_world *world, w_entity_id component_id)
{
	w_grow_components_container_(world, component_id.index + 1);
	return world->components->components[component_id.index];
}

// sort the given component array's sparse set
// note: this is executed after a component has array has been modified
void w_sort_component_array(struct w_world *world, w_entity_id component_id)
{
	w_sparse_set_sort(w_get_component_array(world, component_id));
}


/******************************
*  component management API  *
******************************/

// set a component by ID on the given entity
void w_set_component_(struct w_world *world, w_entity_id component_id, size_t component_size, w_entity_id entity_id, void* component)
{
	// grow array of sparse sets if required
	w_grow_components_container_(world, component_id.index + 1);

	// create a sparse set for the component if its null
	if (world->components->components_length < component_id.index + 1 || world->components->components[component_id.index] == NULL)
	{
		w_create_component_array(world, component_id, component_size);
	}

	// get the component array
	w_sparse_set* component_array = w_get_component_array(world, component_id);

	// set the component
	w_sparse_set_set(component_array, entity_id.index, component);
}

// remove a component by ID from an entity
void w_remove_component_(struct w_world *world, w_entity_id component_id, w_entity_id entity_id)
{
	// get component array
	w_sparse_set* component_array = w_get_component_array(world, component_id);

	// remove component
	w_sparse_set_remove(component_array, entity_id.index);
}

// remove all of the components on an entity
void w_remove_all_components_(struct w_world *world, w_entity_id entity_id)
{
	for (int ci = 0; ci < world->components->components_length; ++ci)
	{
		w_entity_id component_id = w_entity_id_from_raw(ci);

		if (world->components->components[ci] != NULL)
		{
			w_remove_component_(world, component_id, entity_id);
		}
	}
}

