/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_v1
 * @created     : Tuesday Feb 11, 2025 13:34:00 CST
 */

#include "whisker_array.h"
#include "whisker_dict.h"
#include "whisker_debug.h"
#include "whisker_ecs_v1.h"

size_t entity[ENTITY_MAX] = {};
size_t entity_recycled[ENTITY_MAX] = {};
size_t component_entity[COMPONENT_MAX][ENTITY_MAX] = {0};
/* void* component_array[COMPONENT_MAX] = {}; */
void* components;
void (*system_array[SYSTEM_MAX])(float) = {};

typedef struct components
{
	void* components;
} components_array;

void init_ecs()
{
	debug_printf("ecs:init\n");

	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		entity[i] = -1;
		entity_recycled[i] = -1;
	}

	for (size_t i = 0; i < COMPONENT_MAX; ++i)
	{
		for (size_t ii = 0; ii < ENTITY_MAX; ++ii)
		{
    		component_entity[i][ii] = 0;
		}
	}

	whisker_dict_create(&components, components_array, 0);
}

void deinit_ecs()
{
	debug_printf("ecs:deinit\n");

	for (size_t i = 0; i < COMPONENT_MAX; ++i)
	{
		for (size_t ii = 0; ii < ENTITY_MAX; ++ii)
		{
    		/* free(component_entity[i]); */
		}

		if (whisker_dict_contains_key(components, (char*) &i))
		{
			components_array* component_array = whisker_dict_get(components, (char*) &i);
			whisker_arr_free(component_array->components);
		}
	}

	whisker_dict_free(components);
}

/**********************
*  entity functions  *
**********************/
void add_entity(size_t e)
{
	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		if (entity[i] == -1)
		{
			/* debug_printf("ecs:add_entity:e = %zu @ %zu\n", e, i); */

			entity[i] = e;
			break;
		}
	}
}

void recycle_entity(size_t e)
{
	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		if (entity_recycled[i] == -1)
		{
			/* debug_printf("ecs:recycle_entity:e = %zu @ %zu\n", e, i); */

			entity_recycled[i] = e;
			break;
		}
	}
}

void remove_entity(size_t e)
{
	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		if (entity[i] == e)
		{
			/* debug_printf("ecs:remove_entity:e = %d @ %d\n", e, i); */

			for (size_t i = 0; i < COMPONENT_MAX; ++i)
			{
    			component_entity[i][e] = 0;
			}

			entity[i] = -1;
			recycle_entity(e);
			break;
		}
	}
}

size_t get_recycled_entity()
{
	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		if (entity_recycled[i] != -1) {
			size_t recycled = entity_recycled[i];
			entity_recycled[i] = -1;

			return recycled;
		}
	}

	return -1;
}

size_t create_entity()
{
	// first try to get a recycled entity
	size_t recycled_entity = get_recycled_entity();

	if (recycled_entity == -1) {
		// use the entity current
		size_t largest_entity = 0;
		for (size_t i = 0; i < ENTITY_MAX; ++i)
		{
			/* debug_printf("%d %d %d\n", i, entity[i], largest_entity); */
			if (entity[i] != -1 && entity[i] > largest_entity) {
				largest_entity = entity[i];
			}
		}
		/* debug_printf("ecs:create_entity:largest_entity+1 = %d\n", largest_entity + 1); */
		add_entity(largest_entity + 1);
		return largest_entity + 1;
	}
	else
	{
		/* debug_printf("ecs:create_entity:recycled_entity = %d\n", recycled_entity); */
		add_entity(recycled_entity);
	}

	return recycled_entity;
}



/*************************
*  component functions  *
*************************/
bool add_component_entity(size_t component_id, size_t entity_id)
{
    if (component_id >= COMPONENT_MAX || entity_id >= ENTITY_MAX) {
        return false;
    }

    /* debug_printf("ecs:add_component_entity:component_id = %zu\n", component_id); */
    /* debug_printf("ecs:add_component_entity:entity_id = %zu\n", entity_id); */

    component_entity[component_id][entity_id] = 1;
    return true;
}

bool remove_component_entity(size_t component_id, size_t entity_id)
{
    if (component_id >= COMPONENT_MAX || entity_id >= ENTITY_MAX) {
        return false;
    }

    /* debug_printf("ecs:remove_component_entity:component_id = %zu\n", component_id); */
    /* debug_printf("ecs:remove_component_entity:entity_id = %zu\n", entity_id); */

    component_entity[component_id][entity_id] = 0;
    return true;
}

bool has_component_entity(size_t component_id, size_t entity_id)
{
    if (component_id >= COMPONENT_MAX || entity_id >= ENTITY_MAX) {
        return false;
    }

    return (component_entity[component_id][entity_id] == 1);
}


/*******************************
*  component array functions  *
*******************************/
void init_component_array(size_t component_id, size_t component_size)
{
	if (!whisker_dict_contains_key(components, (char*) &component_id))
	{
		components_array component_array = {};
		whisker_arr_create_f(component_size, ENTITY_MAX, &component_array.components);
		whisker_dict_set(&components, (char*) &component_id, &component_array);
	}
}

void set_component(size_t component_id, size_t component_size, size_t entity, void* component_value)
{
	init_component_array(component_id, component_size);

	components_array* component_array = whisker_dict_get(components, (char*) &component_id);

    memcpy((char*)component_array->components + entity * component_size, component_value, component_size);
	add_component_entity(component_id, entity);    
}

void remove_component(size_t component_id, size_t component_size, size_t entity)
{
	init_component_array(component_id, component_size);

	components_array* component_array = whisker_dict_get(components, (char*) &component_id);

    memset((char*)component_array->components + entity * component_size, 0, component_size);
	remove_component_entity(component_id, entity);    
}

void* get_component(size_t component_id, size_t component_size, size_t entity)
{
	init_component_array(component_id, component_size);

	components_array* component_array = whisker_dict_get(components, (char*) &component_id);

    return (char*)component_array->components + entity * component_size;
}

size_t set_component_entities(size_t component_id, size_t entity_list[])
{
	size_t entity_l[ENTITY_MAX];
	size_t entity_i = 0;
	for (size_t e = 0; e < ENTITY_MAX; ++e)
	{
		size_t ce = component_entity[component_id][e];
		if (ce == 0) {
			continue;
		}

		entity_l[entity_i] = e;
		entity_i++;
	}

	memcpy(entity_list, entity_l, entity_i * sizeof(size_t));

	return entity_i;
}


/**********************
*  system functions  *
**********************/
void update_systems(float delta_time)
{
	for (size_t i = 0; i < SYSTEM_MAX; ++i)
	{
		if (system_array[i] != NULL) {
			system_array[i](delta_time);
		}
	}
}

void register_system(void (*system_ptr)(float))
{
	deregister_system(system_ptr);

	for (size_t i = 0; i < SYSTEM_MAX; ++i)
	{
		if (system_array[i] == NULL)
		{
			system_array[i] = system_ptr;
			break;
		}
	}	
}
void deregister_system(void (*system_ptr)(float))
{
	for (size_t i = 0; i < SYSTEM_MAX; ++i)
	{
		if (system_array[i] == system_ptr)
		{
			system_array[i] = NULL;
		}
	}	
}
