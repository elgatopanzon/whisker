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
	whisker_ecs_entities *e = whisker_mem_xcalloc_t(1, *e);

	// create arrays
	whisker_arr_init_t(e->entities, WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE);
	whisker_arr_init_t(e->destroyed_entities, WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE);
	whisker_arr_init_t(e->deferred_actions, WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE);

	// create entity names trie
	e->entity_names = whisker_mem_xcalloc_t(1, *e->entity_names);

	// create empty entities up to the MIN
	for (int i = 0; i < WHISKER_ECS_ENTITY_MIN; ++i)
	{
		whisker_ecs_entity_id create_err = whisker_ecs_e_create(e);
		if (create_err.id == 0)
		{
			// note: the ONLY occasion where entity ID 0 is a possible expected
			// value
			// TODO: handle this with whisker_error.h by checking for an error
		}
	}

	*entities = e;

	return E_WHISKER_ECS_ENTITY_OK;
}

// free an instance of entities state
void whisker_ecs_e_free_entities(whisker_ecs_entities *entities)
{
	// free entity name strings
	for (int i = 0; i < entities->entities_length; ++i)
	{
		if (entities->entities[i].name != NULL)
		{
			free_null(entities->entities[i].name);
		}
	}

	free(entities->entities);
	free(entities->destroyed_entities);
	whisker_trie_free_node_values(entities->entity_names);
	whisker_trie_free_nodes(entities->entity_names);
	free(entities->entity_names);
	free(entities->deferred_actions);

	free(entities);
}

/***********************
*  entity management  *
***********************/
// request a new entity
whisker_ecs_entity_id whisker_ecs_e_create(whisker_ecs_entities *entities)
{
	whisker_ecs_entity_id e;
	E_WHISKER_ECS_ENTITY err = whisker_ecs_e_create_(entities, &e);
	if (err != E_WHISKER_ECS_ENTITY_OK)
	{
		// TODO: some kind of panic
		return (whisker_ecs_entity_id) { .id = 0 };
	}

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

		new_id = entities->entities[recycled_index].id;
		entities->entities[recycled_index].destroyed = false;
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
	if (entities->destroyed_entities_length > 0)
	{
		*entity_index = entities->destroyed_entities[--entities->destroyed_entities_length];
		return E_WHISKER_ECS_ENTITY_OK;
	}

	return E_WHISKER_ECS_ENTITY_ARR;
}

// create a new entity and add it to the entities list
E_WHISKER_ECS_ENTITY whisker_ecs_e_create_new_(whisker_ecs_entities *entities, whisker_ecs_entity_id *entity_id)
{
	size_t entity_count = whisker_ecs_e_count(entities);

	// TODO: check for max entities reached if there's a limit set later
	
	whisker_arr_ensure_alloc_block_size(entities->entities, (entities->entities_length + 1), WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE);

	entities->entities[entity_count].id.index = entity_count;
	entities->entities[entity_count].id.version = 0;
	entities->entities[entity_count].destroyed = false;
	entities->entities[entity_count].name = NULL;

	*entity_id = entities->entities[entity_count].id;
	entities->entities_length++;

	return E_WHISKER_ECS_ENTITY_OK;
}

// set the name for an entity
E_WHISKER_ECS_ENTITY whisker_ecs_e_set_name(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_index *trie_id = whisker_mem_xcalloc_t(1, *trie_id);
	*trie_id = entity_id.index;
	whisker_trie_set_value_str(entities->entity_names, name, trie_id);

	// copy the name into the entities name
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	e->name = whisker_mem_xmalloc(strlen(name) + 1);
	strncpy(e->name, name, strlen(name) + 1);

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
		whisker_trie_remove_value_str(entities->entity_names, e->name);
		whisker_trie_set_value_str(entities->entity_names, e->name, NULL);
		free_null(e->name);
		e->name = NULL;
	}

	// push to dead entities stack
	whisker_arr_ensure_alloc_block_size(entities->destroyed_entities, (entities->destroyed_entities_length + 1), WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE);
	entities->destroyed_entities[entities->destroyed_entities_length++] = e->id.index;

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

// add a deferred entity action to be processed at a later time
// note: typically it would be the end of an update
E_WHISKER_ECS_ENTITY whisker_ecs_e_add_deffered_action(whisker_ecs_entities *entities, whisker_ecs_entity_deferred_action action)
{
	whisker_arr_ensure_alloc_block_size(entities->deferred_actions, (entities->deferred_actions_length + 1), WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE);
	entities->deferred_actions[entities->deferred_actions_length++] = action;

	return E_WHISKER_ECS_ENTITY_OK;
}

// process the deferred actions stack
E_WHISKER_ECS_ENTITY whisker_ecs_e_process_deferred(whisker_ecs_entities *entities)
{
	while (entities->deferred_actions_length > 0) 
	{
		whisker_ecs_entity_deferred_action action = entities->deferred_actions[--entities->deferred_actions_length];

		switch (action.action) {
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE:
				entities->entities[action.id.index].destroyed = false;		
				break;
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY:
				if (whisker_ecs_e_destroy(entities, action.id) != E_WHISKER_ECS_ENTITY_OK)
				{
					// TODO: panic here
					break;
				}
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

	return &entities->entities[entity_id.index];
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
	whisker_ecs_entity_index *e_idx = whisker_trie_search_value_str(entities->entity_names, entity_name);
	if (e_idx == NULL)
	{
		return NULL;
	}

	return &entities->entities[*e_idx];
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
	return entities->entities_length;
}

// get current count of alive entities
size_t whisker_ecs_e_alive_count(whisker_ecs_entities *entities)
{
	return whisker_ecs_e_count(entities) - whisker_ecs_e_destroyed_count(entities);
}

// get current count of destroyed entities
size_t whisker_ecs_e_destroyed_count(whisker_ecs_entities *entities)
{
	return entities->destroyed_entities_length;
}

// convert a string of named entities in the format "name1,name2,name3" to an
// array of entities, creating them if they don't already exist
whisker_arr_whisker_ecs_entity_id* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names_str)
{
	// entity list derived from string entity names
	whisker_arr_whisker_ecs_entity_id *entities_new;
	E_WHISKER_ARR arr_err = whisker_arr_create_whisker_ecs_entity_id(&entities_new, 0);
	if (arr_err != E_WHISKER_ARR_OK)
	{
		// TODO: panic here
		return NULL;
	}

	size_t names_length = strlen(entity_names_str);
	char* entity_names = whisker_mem_xmalloc(names_length + 1);
	strncpy(entity_names, entity_names_str, names_length + 1);

	if (names_length == 0)
	{
		free_null(entity_names);
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
			E_WHISKER_ECS_ENTITY entity_err = whisker_ecs_e_create_named_(entities, entity_names + search_index, &e);
			if (entity_err != E_WHISKER_ECS_ENTITY_OK)
			{
				// TODO: panic here
				break;
			}

			debug_printf("%zu-%zu: %s\n", search_index, i, entity_names + search_index);

			// add the entity id to the final list, and reset name array
			arr_err = whisker_arr_push_whisker_ecs_entity_id(entities_new, e);
			if (arr_err != E_WHISKER_ARR_OK)
			{
				// TODO: panic here
				break;
			}

			search_index = i + 1;
			if (mutated)
			{
    			entity_names[i] = ',';
			}
			continue;
		}
	}

	free_null(entity_names);

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
