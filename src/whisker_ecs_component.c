/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_component
 * @created     : Thursday Feb 13, 2025 17:59:26 CST
 */

#include "whisker_std.h"
#include "whisker_dict.h"
#include "whisker_debug.h"
#include "whisker_block_array.h"
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

	// create sparse set for changed components
	wss_create_t(&components->changed_components, whisker_ecs_entity_id);
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
			wss_free(components->components[i]);
		}
	}
	free(components->components);
	wss_free(components->changed_components);
}

/******************************************
*  component array management functions  *
******************************************/

// allocate an empty component array
E_WHISKER_ECS_COMP whisker_ecs_c_create_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size)
{
	// grow the components array to fit the new component ID
	whisker_ecs_c_grow_components_(components, component_id.index + 1);

	// create array
	whisker_sparse_set *ss;
	debug_printf("creating component sparse set %zu size %zu\n", component_id.id, component_size);
	if (wss_create_s(&ss, component_size) != E_WHISKER_SS_OK)
	{
		return E_WHISKER_ECS_COMP_ARR;
	}

	components->components[component_id.index] = ss;

	return E_WHISKER_ECS_COMP_OK;
}

// grow components array size if required to the nearest block size
void whisker_ecs_c_grow_components_(whisker_ecs_components *components, size_t capacity)
{
	whisker_arr_ensure_alloc_block_size(
		components->components, 
		(capacity), 
		(WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE)
	);
	if (capacity > components->components_length)
	{
		components->components_length = capacity;
	}
}

// get the component array for the provided component ID 
// note: will create if it doesn't exist
E_WHISKER_ECS_COMP whisker_ecs_c_get_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_sparse_set **component_array)
{
	whisker_ecs_c_grow_components_(components, component_id.index + 1);
	*component_array = components->components[component_id.index];

	return E_WHISKER_ECS_COMP_OK;
}

// deallocate component array for the provided component ID
E_WHISKER_ECS_COMP whisker_ecs_c_free_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id)
{
	whisker_sparse_set* component_array;
	E_WHISKER_ECS_COMP err = whisker_ecs_c_get_component_array(components, component_id, &component_array);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return err;
	}

	whisker_arr_free(component_array);

	return E_WHISKER_ECS_COMP_OK;
}

// sort the given component array's sparse set
// note: this is executed after a component has array has been modified
E_WHISKER_ECS_COMP whisker_ecs_c_sort_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id)
{
	whisker_sparse_set* component_array;
	E_WHISKER_ECS_COMP err = whisker_ecs_c_get_component_array(components, component_id, &component_array);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return err;
	}

	whisker_ss_sort(component_array);

	return E_WHISKER_ECS_COMP_OK;
}

/************************************
*  component management functions  *
************************************/
// get a component by ID for the given entity
void* whisker_ecs_c_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	// get component array
	whisker_sparse_set* component_array;
	E_WHISKER_ECS_COMP err = whisker_ecs_c_get_component_array(components, component_id, &component_array);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return NULL;
	}

	// return component pointer
	return wss_get(component_array, entity_id.index);
}

// set a component by ID on the given entity
E_WHISKER_ECS_COMP whisker_ecs_c_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void* component, bool sort)
{
	// grow array of sparse sets if required
	whisker_ecs_c_grow_components_(components, component_id.index + 1);

	// create a sparse set for the component if its null
	if (components->components_length < component_id.index + 1 || components->components[component_id.index] == NULL)
	{
		E_WHISKER_ECS_COMP create_err = whisker_ecs_c_create_component_array(components, component_id, component_size);
		if (create_err != E_WHISKER_ECS_COMP_OK)
		{
			return create_err;
		}
	}

	// get the component array
	whisker_sparse_set* component_array;
	E_WHISKER_ECS_COMP err = whisker_ecs_c_get_component_array(components, component_id, &component_array);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return err;
	}
	bool sort_required = (sort && !wss_contains(component_array, entity_id.index));

	// set the component
	E_WHISKER_SS set_err = wss_set(component_array, entity_id.index, component);
	if (set_err != E_WHISKER_SS_OK)
	{
		return E_WHISKER_ECS_COMP_ARR;
	}

	if (sort_required)
	{
		E_WHISKER_ECS_COMP sort_err = whisker_ecs_c_sort_component_array(components, component_id);
		if (sort_err != E_WHISKER_ECS_COMP_OK)
		{
			return sort_err;
		}
	}

	return E_WHISKER_ECS_COMP_OK;
}

// check if the provided entity has a component by ID
bool whisker_ecs_c_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id)
{
	// get component array
	whisker_sparse_set* component_array;
	E_WHISKER_ECS_COMP err = whisker_ecs_c_get_component_array(components, component_id, &component_array);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return false;
	}

	// return component pointer
	return component_array != NULL && wss_contains(component_array, entity_id.index);
}

// remove a component by ID from an entity
E_WHISKER_ECS_COMP whisker_ecs_c_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id, bool sort)
{
	// get component array
	whisker_sparse_set* component_array;
	E_WHISKER_ECS_COMP err = whisker_ecs_c_get_component_array(components, component_id, &component_array);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return err;
	}

	// return component pointer
	E_WHISKER_SS remove_err = wss_remove(component_array, entity_id.index);
	if (remove_err != E_WHISKER_SS_OK)
	{
		return E_WHISKER_ECS_COMP_ARR;
	}

	if (sort)
	{
		E_WHISKER_ECS_COMP sort_err = whisker_ecs_c_sort_component_array(components, component_id);
		if (sort_err != E_WHISKER_ECS_COMP_OK)
		{
			return sort_err;
		}
	}

	return E_WHISKER_ECS_COMP_OK;
}
