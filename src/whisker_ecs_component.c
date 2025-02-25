/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_component
 * @created     : Thursday Feb 13, 2025 17:59:26 CST
 */

#include "whisker_std.h"
#include "whisker_dict.h"
#include "whisker_debug.h"
#include "whisker_block_array.h"

#include "whisker_ecs.h"

E_WHISKER_ECS_COMP whisker_ecs_c_create_components(whisker_ecs_components **components)
{
	whisker_ecs_components *c = calloc(1, sizeof(*c));
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

	*components = c;

	return E_WHISKER_ECS_COMP_OK;
}

void whisker_ecs_c_free_components(whisker_ecs_components *components)
{
	for (int i = 0; i < warr_length(components->components); i++) {
		if (components->components[i] != NULL)
		{
			whisker_block_arr_free(components->components[i]);
		}
	}
	warr_free(components->components);

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
	whisker_block_array *barr;
	debug_printf("creating component block array %zu size %zu\n", component_id.id, component_size);
	if (whisker_block_arr_create_f(component_size, 256, &barr) != E_WHISKER_BLOCK_ARR_OK)
	{
		return E_WHISKER_ECS_COMP_ARR;
	}

	components->components[component_id.index] = barr;

	return E_WHISKER_ECS_COMP_OK;
}

// resize the components array to the specified size
E_WHISKER_ECS_COMP whisker_ecs_c_grow_components_(whisker_ecs_components *components, size_t capacity)
{
	if (warr_length(components->components) >= capacity)
	{
		return E_WHISKER_ECS_COMP_OK;
	}

	if (warr_resize(&components->components, capacity) != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ECS_COMP_ARR;
	}

	return E_WHISKER_ECS_COMP_OK;
}

// get the component array for the provided component ID (create if doesn't
// exist)
E_WHISKER_ECS_COMP whisker_ecs_c_get_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, void **component_array)
{
	if (warr_length(components->components) < component_id.index + 1 || components->components[component_id.index] == NULL)
	{
		E_WHISKER_ECS_COMP create_err = whisker_ecs_c_create_component_array(components, component_id, component_size);
		if (create_err != E_WHISKER_ECS_COMP_OK)
		{
			return create_err;
		}
	}

	*component_array = components->components[component_id.index];

	return E_WHISKER_ECS_COMP_OK;
}

// deallocate component array for the provided component ID
E_WHISKER_ECS_COMP whisker_ecs_c_free_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id)
{
	void* component_array;
	E_WHISKER_ECS_COMP err = whisker_ecs_c_get_component_array(components, component_id, 0, &component_array);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return err;
	}

	warr_free(component_array);

	return E_WHISKER_ECS_COMP_OK;
}

/************************************
*  component management functions  *
************************************/
void* whisker_ecs_c_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id)
{
	// get component array
	void* component_array;
	E_WHISKER_ECS_COMP err = whisker_ecs_c_get_component_array(components, component_id, component_size, (void**)&component_array);
	if (err != E_WHISKER_ECS_COMP_OK)
	{
		return NULL;
	}

	// grow component array if needed
	/* whisker_ecs_c_grow_component_array_(&component_array, entity_id.index + 1); */
	/* components->components[component_id.index] = component_array; */

	// return component pointer
	return whisker_block_arr_get(component_array, entity_id.index);
	/* return component_array + (entity_id.index * component_size); */
}

E_WHISKER_ECS_COMP whisker_ecs_c_grow_component_array_(void **component_array, size_t capacity)
{
	/* if (warr_length(*component_array) >= capacity) */
	/* { */
	/* 	return E_WHISKER_ECS_COMP_OK; */
	/* } */
    /*  */
	/* if (warr_resize(component_array, capacity) != E_WHISKER_ARR_OK) */
	/* { */
	/* 	return E_WHISKER_ECS_COMP_ARR; */
	/* } */
    /*  */
	return E_WHISKER_ECS_COMP_OK;
}

E_WHISKER_ECS_COMP whisker_ecs_c_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void* component)
{
	void* component_ptr = whisker_ecs_c_get_component(components, component_id, component_size, entity_id);
	if (component_ptr == NULL)
	{
		return E_WHISKER_ECS_COMP_UNKNOWN;
	}

	memcpy(component_ptr, component, component_size);

	return E_WHISKER_ECS_COMP_OK;
}

/***********************
*  utility functions  *
***********************/
