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

// create instance of components array holder
E_WHISKER_ECS_COMP whisker_ecs_c_create_components(whisker_ecs_components **components)
{
	whisker_ecs_components *c = whisker_mem_xcalloc(1, sizeof(*c));
	if (c == NULL)
	{
		return E_WHISKER_ECS_COMP_MEM;
	}

	// create array
	if (warr_create(void*, 0, &c->components) != E_WHISKER_ARR_OK)
	{
		free(c);
		return E_WHISKER_ECS_COMP_ARR;
	}
	c->components_length = 0;

	if (wss_create_t(&c->changed_components, whisker_ecs_entity_id) != E_WHISKER_SS_OK)
	{
		warr_free(c->components);
		free(c);
		return E_WHISKER_ECS_COMP_ARR;
	}

	*components = c;

	return E_WHISKER_ECS_COMP_OK;
}

// free instance of components array holder
void whisker_ecs_c_free_components(whisker_ecs_components *components)
{
	for (int i = 0; i < components->components_length; i++) {
		if (components->components[i] != NULL)
		{
			wss_free(components->components[i]);
		}
	}
	warr_free(components->components);
	wss_free(components->changed_components);

	free(components);
}

/******************************************
*  component array management functions  *
******************************************/

// allocate an empty component array
E_WHISKER_ECS_COMP whisker_ecs_c_create_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size)
{
	// grow the components array to fit the new component ID if required
	whisker_ecs_c_grow_components_(components, component_id.index + 1);

	// create array
	whisker_sparse_set *ss;
	debug_printf("creating component block array %zu size %zu\n", component_id.id, component_size);
	if (wss_create_s(&ss, component_size) != E_WHISKER_SS_OK)
	{
		return E_WHISKER_ECS_COMP_ARR;
	}

	components->components[component_id.index] = ss;

	return E_WHISKER_ECS_COMP_OK;
}

// resize the components array to the specified size
E_WHISKER_ECS_COMP whisker_ecs_c_grow_components_(whisker_ecs_components *components, size_t capacity)
{
	if (components->components_length >= capacity)
	{
		return E_WHISKER_ECS_COMP_OK;
	}

	if (warr_resize(&components->components, capacity) != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ECS_COMP_ARR;
	}
	components->components_length = capacity;

	return E_WHISKER_ECS_COMP_OK;
}

// get the component array for the provided component ID 
// note: will create if it doesn't exist
E_WHISKER_ECS_COMP whisker_ecs_c_get_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_sparse_set **component_array)
{
	// grow components if needed
	E_WHISKER_ECS_COMP err = whisker_ecs_c_grow_components_(components, component_id.index + 1);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return err;
	}

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

	warr_free(component_array);

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
	E_WHISKER_ECS_COMP grow_err = whisker_ecs_c_grow_components_(components, component_id.index + 1);
	if (grow_err != E_WHISKER_ECS_COMP_OK)
	{
		return grow_err;
	}

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

	bool sort_required = (!wss_contains(component_array, entity_id.index) && sort);

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
