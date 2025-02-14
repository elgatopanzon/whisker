/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 17:53:56 CST
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_dict.h"

#include "whisker_ecs.h"

/*************************************
*  entities struct management  *
*************************************/
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_entities(whisker_ecs_entities **entities)
{
	whisker_ecs_entities *e;
	if (wmem_try_calloc_t(1, *e, &e) != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_ECS_ENTITY_MEM;
	}

	// create arrays and dict
	if (warr_create(whisker_ecs_entity, 0, &e->entities) != E_WHISKER_ARR_OK)
	{
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	if (warr_create(whisker_ecs_entity_index, 0, &e->dead_entities) != E_WHISKER_ARR_OK)
	{
		warr_free(e->entities);
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	if (wdict_create(&e->entity_keys, char*, 0) != E_WHISKER_DICT_OK)
	{
		warr_free(e->entities);
		warr_free(e->dead_entities);
		free(e);
		return E_WHISKER_ECS_ENTITY_DICT;
	}

	*entities = e;

	return E_WHISKER_ECS_ENTITY_OK;
}

void whisker_ecs_e_free_entities(whisker_ecs_entities *entities)
{
	warr_free(entities->entities);
	warr_free(entities->dead_entities);
	wdict_free(entities->entity_keys);

	free(entities);
}

/***********************
*  entity management  *
***********************/

E_WHISKER_ECS_ENTITY whisker_ecs_e_create(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id)
{
	whisker_ecs_entity_id new_id;

	E_WHISKER_ECS_ENTITY err_create;
	if (whisker_ecs_e_dead_count(entities))
	{
		whisker_ecs_entity_index recycled_index;
		err_create = whisker_ecs_e_pop_recycled_(entities, &recycled_index);

		new_id = entities->entities[recycled_index].id;
	}
	else
	{
		err_create = whisker_ecs_e_create_new_(entities, &new_id);
	}

	if (err_create != E_WHISKER_ECS_ENTITY_OK)
	{
		return err_create;
	}

	*entity_id = new_id;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities, whisker_ecs_entity_index *entity_index)
{
	if (warr_pop_front(&entities->dead_entities, entity_index) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_ENTITY_ARR;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_create_new_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id)
{
	size_t entity_count = whisker_ecs_e_count(entities);

	// TODO: pop a dead entity if one exists
	
	// TODO: check for max entities reached if there's a limit set later
	
	if (warr_increment_size(&entities->entities) != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	entities->entities[entity_count].id.index = entity_count;

	// TODO: init archetypes array

	*entity_id = entities->entities[entity_count].id;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_set_name(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id entity_id)
{

}

E_WHISKER_ECS_ENTITY whisker_ecs_e_create_named(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id entity_id)
{

}

E_WHISKER_ECS_ENTITY whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	// increment version
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	e->id.version++;

	// push to dead entities stack
	if (warr_push(&entities->dead_entities, &e->id.index) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_ENTITY_ARR;

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);

	// TODO: reset archetype array length to 0

	if (whisker_ecs_e_recycle(entities, entity_id) != E_WHISKER_ECS_ENTITY_OK)
		return E_WHISKER_ECS_ENTITY_ARR;

	return E_WHISKER_ECS_ENTITY_OK;
}


/***********************
*  utility functions  *
***********************/

whisker_ecs_entity* whisker_ecs_e(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	if (entity_id.index + 1 > whisker_ecs_e_count(entities))
	{
		return NULL;
	}

	return &entities->entities[entity_id.index];
}

bool whisker_ecs_e_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	return (whisker_ecs_e(entities, entity_id)->id.version == entity_id.version);
}

size_t whisker_ecs_e_count(whisker_ecs_entities *entities)
{
	return warr_length(entities->entities);
}

size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities)
{
	return whisker_ecs_e_count(entities) - whisker_ecs_e_dead_count(entities);
}

size_t whisker_ecs_e_dead_count(whisker_ecs_entities *entities)
{
	return warr_length(entities->dead_entities);
}
