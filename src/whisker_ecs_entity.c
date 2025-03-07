/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 17:53:56 CST
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_dict.h"
#include "whisker_debug.h"
#include "whisker_string.h"

#include "whisker_ecs.h"

/************
*  arrays  *
************/

/*************************************
*  entities struct management  *
*************************************/
// create an instance of entities state
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_entities(whisker_ecs_entities **entities)
{
	whisker_ecs_entities *e;
	if (wmem_try_calloc_t(1, *e, &e) != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_ECS_ENTITY_MEM;
	}

	// create arrays and dict
	if (whisker_arr_create_whisker_ecs_entity(&e->entities, 0) != E_WHISKER_ARR_OK)
	{
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	if (whisker_arr_create_whisker_ecs_entity_index(&e->destroyed_entities, 0) != E_WHISKER_ARR_OK)
	{
		whisker_arr_free_whisker_ecs_entity(e->entities);
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	if (whisker_arr_create_whisker_ecs_entity_deferred_action(&e->deferred_actions, 0) != E_WHISKER_ARR_OK)
	{
		whisker_arr_free_whisker_ecs_entity(e->entities);
		whisker_arr_free_whisker_ecs_entity_index(e->destroyed_entities);
		free(e);
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	if (wdict_create(&e->entity_names, char*, 0) != E_WHISKER_DICT_OK)
	{
		whisker_arr_free_whisker_ecs_entity(e->entities);
		whisker_arr_free_whisker_ecs_entity_index(e->destroyed_entities);
		free(e);
		return E_WHISKER_ECS_ENTITY_DICT;
	}

	// create empty entities up to the MIN
	for (int i = 0; i < WHISKER_ECS_ENTITY_MIN; ++i)
	{
		whisker_ecs_e_create(e);
	}

	*entities = e;

	return E_WHISKER_ECS_ENTITY_OK;
}

// free an instance of entities state
void whisker_ecs_e_free_entities(whisker_ecs_entities *entities)
{
	// free entity name wstrs
	for (int i = 0; i < entities->entities->length; ++i)
	{
		if (entities->entities->arr[i].name != NULL)
		{
			wstr_free(entities->entities->arr[i].name);
		}
	}

	whisker_arr_free_whisker_ecs_entity(entities->entities);
	whisker_arr_free_whisker_ecs_entity_index(entities->destroyed_entities);
	wdict_free(entities->entity_names);
	whisker_arr_free_whisker_ecs_entity_deferred_action(entities->deferred_actions);

	free(entities);
}

/***********************
*  entity management  *
***********************/
// request a new entity
whisker_ecs_entity_id whisker_ecs_e_create(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_(entities, &e);

	return e;
}

// creates and sets an entity, either new or recycled
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id)
{
	whisker_ecs_entity_id new_id;

	E_WHISKER_ECS_ENTITY err_create;
	if (whisker_ecs_e_destroyed_count(entities))
	{
		whisker_ecs_entity_index recycled_index;
		err_create = whisker_ecs_e_pop_recycled_(entities, &recycled_index);

		new_id = entities->entities->arr[recycled_index].id;
		entities->entities->arr[recycled_index].destroyed = false;
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

// pop a recycled entity from the destroyed_entities stack
E_WHISKER_ECS_ENTITY whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities, whisker_ecs_entity_index *entity_index)
{
	if (whisker_arr_pop_whisker_ecs_entity_index(entities->destroyed_entities, entity_index) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_ENTITY_ARR;

	return E_WHISKER_ECS_ENTITY_OK;
}

// create a new entity and add it to the entities list
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_new_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id)
{
	size_t entity_count = whisker_ecs_e_count(entities);

	// TODO: check for max entities reached if there's a limit set later
	
	if (whisker_arr_increment_size_whisker_ecs_entity(entities->entities) != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	entities->entities->arr[entity_count].id.index = entity_count;
	entities->entities->arr[entity_count].id.version = 0;
	entities->entities->arr[entity_count].destroyed = false;
	entities->entities->arr[entity_count].name = NULL;

	*entity_id = entities->entities->arr[entity_count].id;

	return E_WHISKER_ECS_ENTITY_OK;
}

// set the name for an entity
E_WHISKER_ECS_ENTITY whisker_ecs_e_set_name(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id entity_id)
{
	E_WHISKER_DICT err = whisker_dict_set_strk(&entities->entity_names, name, &entity_id);
	if (err != E_WHISKER_DICT_OK)
	{
		return E_WHISKER_ECS_ENTITY_DICT;
	}

	// copy the name into the entities name
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	if (wstr(name, &e->name) != E_WHISKER_STR_OK)
	{
		return E_WHISKER_ECS_ENTITY_MEM;
	}

	return E_WHISKER_ECS_ENTITY_OK;
}

// create entity with a name, or return an existing entity with the same name
whisker_ecs_entity_id whisker_ecs_e_create_named(whisker_ecs_entities *entities, char *name)
{
	whisker_ecs_entity_id e;
	whisker_ecs_e_create_named_(entities, name, &e);

	return e;
}

// create a new entity with the given name
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_named_(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id *entity_id)
{
	whisker_ecs_entity *e = whisker_ecs_e_named(entities, name);
	if (e != NULL)
	{
		*entity_id = e->id;
		return E_WHISKER_ECS_ENTITY_OK;
	}

	// create new entity with name
	whisker_ecs_entity_id e_id;
	E_WHISKER_ECS_ENTITY create_err = whisker_ecs_e_create_(entities, &e_id);
	if (create_err != E_WHISKER_ECS_ENTITY_OK)
	{
		return create_err;
	}

	// set the name
	create_err = whisker_ecs_e_set_name(entities, name, e_id);
	if (create_err != E_WHISKER_ECS_ENTITY_OK)
	{
		return create_err;
	}

	*entity_id = e_id;

	return E_WHISKER_ECS_ENTITY_OK;
}

// recycle an entity into the destroyed entities stack
E_WHISKER_ECS_ENTITY whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	// increment version
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	e->id.version++;

	// clear name pointer
	if (e->name != NULL)
	{
		whisker_dict_remove_strk(&entities->entity_names, e->name);
		wstr_free(e->name);
		e->name = NULL;
	}

	// push to dead entities stack
	if (whisker_arr_push_whisker_ecs_entity_index(entities->destroyed_entities, e->id.index) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_ENTITY_ARR;

	return E_WHISKER_ECS_ENTITY_OK;
}

// destroy an entity, incrementing it's version and adding it to the destroyed
// entities stack
E_WHISKER_ECS_ENTITY whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	// mark entity as destroyed
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	if (!e->destroyed)
	{
		e->destroyed = true;

		if (whisker_ecs_e_recycle(entities, entity_id) != E_WHISKER_ECS_ENTITY_OK)
			return E_WHISKER_ECS_ENTITY_ARR;
	}
	else
	{
		// TODO: better error than this
		return E_WHISKER_ECS_ENTITY_UNKNOWN;
	}

	return E_WHISKER_ECS_ENTITY_OK;
}

E_WHISKER_ECS_ENTITY whisker_ecs_e_add_deffered_action(whisker_ecs_entities *entities, whisker_ecs_entity_deferred_action action)
{
	if (whisker_arr_push_whisker_ecs_entity_deferred_action(entities->deferred_actions, action) == E_WHISKER_ARR_OK) 
	{
		return E_WHISKER_ECS_ENTITY_ARR;
	}

	return E_WHISKER_ECS_ENTITY_OK;
}

// process the deferred actions stack
E_WHISKER_ECS_ENTITY whisker_ecs_e_process_deferred(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_deferred_action action;
	while (whisker_arr_pop_whisker_ecs_entity_deferred_action(entities->deferred_actions, &action) == E_WHISKER_ARR_OK) 
	{
		switch (action.action) {
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE:
				entities->entities->arr[action.id.index].destroyed = false;		
				break;
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY:
				whisker_ecs_e_destroy(entities, action.id);
				break;
		}
	}

	return E_WHISKER_ECS_ENTITY_OK;
}


/***********************
*  utility functions  *
***********************/

// shortcut to get the entity struct from the given ID
whisker_ecs_entity* whisker_ecs_e(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	if (entity_id.index + 1 > whisker_ecs_e_count(entities))
	{
		return NULL;
	}

	return &entities->entities->arr[entity_id.index];
}

// convert a numeric ID to an entity ID
inline whisker_ecs_entity_id whisker_ecs_e_id(whisker_ecs_entity_id_raw id)
{
	return (whisker_ecs_entity_id){.id = id};
}

// shortcut to get the entity with the given name
// note: this will return NULL if no entity exists with this name
whisker_ecs_entity* whisker_ecs_e_named(whisker_ecs_entities *entities, char* entity_name)
{
	// lookup entity by name and return a match, or NULL
	whisker_ecs_entity_id *entity_id = whisker_dict_get_strk(entities->entity_names, entity_name);
	if (entity_id == NULL)
	{
		return NULL;
	}

	return whisker_ecs_e(entities, *entity_id);
}

// check if an entity is dead by comparing the provided entity version with the
// one in the entities array
bool whisker_ecs_e_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	return (whisker_ecs_e(entities, entity_id)->id.version == entity_id.version);
}

// get the current count of entities in existence including alive and destroyed
size_t whisker_ecs_e_count(whisker_ecs_entities *entities)
{
	return entities->entities->length;
}

// get current count of alive entities
size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities)
{
	return whisker_ecs_e_count(entities) - whisker_ecs_e_destroyed_count(entities);
}

// get current count of destroyed entities
size_t whisker_ecs_e_destroyed_count(whisker_ecs_entities *entities)
{
	return entities->destroyed_entities->length;
}

// convert a string of named entities in the format "name1,name2,name3" to an
// array of entities, creating them if they don't already exist
whisker_arr_whisker_ecs_entity_id* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names_str)
{
	// entity list derived from string entity names
	whisker_arr_whisker_ecs_entity_id *entities_new;
	whisker_arr_create_whisker_ecs_entity_id(&entities_new, 0);

	char* entity_names; wstr(entity_names_str, &entity_names);
	size_t names_length = wstr_length(entity_names);

	if (names_length == 0)
	{
		wstr_free(entity_names);
		return entities_new;
	}

	size_t search_index = 0;
	for (size_t i = 0; i < names_length + 1; ++i)
	{
		// if we reached the end of the string, or a separator
		// mutate to null terminator value
		bool mutated = false;
		if (entity_names[i] == ',') {
    		entity_names[i] = '\0';
    		mutated = true;
		}

		if (entity_names[i] == 0x0)
		{
			// create/get entity ID for name
			whisker_ecs_entity_id e;
			whisker_ecs_e_create_named_(entities, entity_names + search_index, &e);

			debug_printf("%zu-%zu: %s\n", search_index, i, entity_names + search_index);

			// add the entity id to the final list, and reset name array
			whisker_arr_push_whisker_ecs_entity_id(entities_new, e);

			search_index = i + 1;
			if (mutated)
			{
    			entity_names[i] = ',';
			}
			continue;
		}
	}

	wstr_free(entity_names);

	return entities_new;
}

// compare an entity to another entity
// note: used by the entity sort function
int whisker_ecs_e_compare_entity_ids_(const void *id_a, const void *id_b)
{
	return ((*(whisker_ecs_entity_id*)id_a).id - (*(whisker_ecs_entity_id*)id_b).id);
}

// sort an array of entities in ascending order
void whisker_ecs_e_sort_entity_array(whisker_arr_whisker_ecs_entity_id *entities)
{
	qsort(entities->arr, entities->length, sizeof(whisker_ecs_entity_id), whisker_ecs_e_compare_entity_ids_);
}
