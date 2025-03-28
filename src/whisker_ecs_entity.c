/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 17:53:56 CST
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_debug.h"

#include "whisker_ecs.h"

/************
*  arrays  *
************/

/*************************************
*  entities struct management  *
*************************************/
// create an instance of entities container
whisker_ecs_entities *whisker_ecs_e_create_and_init_entities()
{
	whisker_ecs_entities *e = whisker_ecs_e_create_entities();
	whisker_ecs_e_init_entities(e);

	return e;
}

// allocate instance of entities container
whisker_ecs_entities *whisker_ecs_e_create_entities()
{
	whisker_ecs_entities *e = whisker_mem_xcalloc_t(1, *e);
	return e;
}

// init entity arrays for an entity container
void whisker_ecs_e_init_entities(whisker_ecs_entities *entities)
{
	// create and allocate entity arrays
	whisker_arr_init_t(
		entities->entities, 
		WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE
	);
	whisker_arr_init_t(
		entities->destroyed_entities, 
		WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE
	);
	whisker_arr_init_t(
		entities->deferred_actions,
		WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
	);

	// create entity names trie
	entities->entity_names = whisker_mem_xcalloc_t(1, *entities->entity_names);

	// init pthread mutexes
	pthread_mutex_init(&entities->deferred_actions_mutex, NULL);
	pthread_mutex_init(&entities->create_entity_mutex, NULL);
}

// free arrays on instance of entities container
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
	pthread_mutex_destroy(&entities->deferred_actions_mutex);
	pthread_mutex_destroy(&entities->create_entity_mutex);
}

// free entity container and containing arrays
void whisker_ecs_e_free_entities_all(whisker_ecs_entities *entities)
{
	whisker_ecs_e_free_entities(entities);
	free(entities);
}

/***********************
*  entity management  *
***********************/
// request a new entity
whisker_ecs_entity_id whisker_ecs_e_create(whisker_ecs_entities *entities)
{
	pthread_mutex_lock(&entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_e_create_(entities);

	pthread_mutex_unlock(&entities->create_entity_mutex);

	return e;
}

// creates and sets an entity, either new or recycled
whisker_ecs_entity_id whisker_ecs_e_create_(whisker_ecs_entities *entities)
{
	if (entities->destroyed_entities_length)
	{
		return entities->entities[whisker_ecs_e_pop_recycled_(entities)].id;
	}
	return whisker_ecs_e_create_new_(entities);
}

// pop a recycled entity from the destroyed_entities stack
whisker_ecs_entity_index whisker_ecs_e_pop_recycled_(whisker_ecs_entities *entities)
{
	if (entities->destroyed_entities_length > 0)
	{
		whisker_ecs_entity_index recycled_index = --entities->destroyed_entities_length;
		entities->entities[recycled_index].destroyed = false;
		return entities->destroyed_entities[recycled_index];
	}

	return 0;
}

// create a new entity and add it to the entities list
whisker_ecs_entity_id whisker_ecs_e_create_new_(whisker_ecs_entities *entities)
{
	const size_t new_idx = entities->entities_length++;

	// reallocate the entity array if required
	whisker_arr_ensure_alloc_block_size(
		entities->entities, 
		(new_idx + 1), 
		WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE
	);

	// init the newly created entity with valid index
	entities->entities[new_idx] = (whisker_ecs_entity) {
		.id.index = new_idx,
	};
	return entities->entities[new_idx].id;
}

// set the name for an entity
void whisker_ecs_e_set_name(whisker_ecs_entities *entities, char* name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_index *trie_id = whisker_mem_xcalloc_t(1, *trie_id);
	*trie_id = entity_id.index;
	whisker_trie_set_value_str(entities->entity_names, name, trie_id);

	// copy the name into the entities name
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	e->name = whisker_mem_xmalloc(strlen(name) + 1);
	strncpy(e->name, name, strlen(name) + 1);
}

// create entity with a name, or return an existing entity with the same name
whisker_ecs_entity_id whisker_ecs_e_create_named(whisker_ecs_entities *entities, char *name)
{
	pthread_mutex_lock(&entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_e_create_named_(entities, name);

	pthread_mutex_unlock(&entities->create_entity_mutex);

	return e;
}

// create a new entity with the given name
whisker_ecs_entity_id whisker_ecs_e_create_named_(whisker_ecs_entities *entities, char* name)
{
	whisker_ecs_entity *e = whisker_ecs_e_named(entities, name);
	if (e)
	{
		return e->id;
	}

	// create new entity with name
	whisker_ecs_entity_id e_id = whisker_ecs_e_create_(entities);

	// set the name
	whisker_ecs_e_set_name(entities, name, e_id);

	return e_id;
}

// recycle an entity into the destroyed entities stack
void whisker_ecs_e_recycle(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	// increment version
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	e->id.version++;

	// clear name pointer
	if (e->name != NULL)
	{
		whisker_trie_remove_value_str(entities->entity_names, e->name);
		free_null(e->name);
		e->name = NULL;
	}

	// push to dead entities stack
	whisker_arr_ensure_alloc_block_size(
		entities->destroyed_entities, 
		(entities->destroyed_entities_length + 1), 
		WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE
	);
	entities->destroyed_entities[entities->destroyed_entities_length++] = e->id.index;
}

// destroy an entity, incrementing it's version and adding it to the destroyed
// entities stack
void whisker_ecs_e_destroy(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
	// mark entity as destroyed
	whisker_ecs_entity *e = whisker_ecs_e(entities, entity_id);
	if (!e->destroyed)
	{
		e->destroyed = true;
		whisker_ecs_e_recycle(entities, entity_id);
	}
}

// set an entity to unmanaged state, and no longer included in iterations or
// recycled
void whisker_ecs_e_make_unmanaged(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
    atomic_store(&entities->entities[entity_id.index].unmanaged, true);
}

// set an entity to managed state
void whisker_ecs_e_make_managed(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id)
{
    atomic_store(&entities->entities[entity_id.index].unmanaged, false);
}

// add a deferred entity action to be processed at a later time
// note: typically it would be the end of an update
void whisker_ecs_e_add_deffered_action(whisker_ecs_entities *entities, whisker_ecs_entity_deferred_action action)
{
	size_t deferred_action_idx = atomic_fetch_add(&entities->deferred_actions_length, 1);

	if((deferred_action_idx + 1) * sizeof(*entities->deferred_actions) > entities->deferred_actions_size)
	{
		pthread_mutex_lock(&entities->deferred_actions_mutex);

		whisker_arr_ensure_alloc_block_size(
			entities->deferred_actions, 
			(deferred_action_idx + 1),
			WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
		);

		pthread_mutex_unlock(&entities->deferred_actions_mutex);
	}
	
	entities->deferred_actions[deferred_action_idx] = action;
}

// process the deferred actions stack
void whisker_ecs_e_process_deferred(whisker_ecs_entities *entities)
{
	while (entities->deferred_actions_length > 0) 
	{
		whisker_ecs_entity_deferred_action action = entities->deferred_actions[--entities->deferred_actions_length];

		switch (action.action) {
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE:
				entities->entities[action.id.index].destroyed = false;		
				break;
			case WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY:
				whisker_ecs_e_destroy(entities, action.id);
				break;
		}
	}
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
struct whisker_ecs_entity_id_array* whisker_ecs_e_from_named_entities(whisker_ecs_entities *entities, char* entity_names_str)
{
	// entity list derived from string entity names
	struct whisker_ecs_entity_id_array *entities_new = whisker_mem_xcalloc_t(1, *entities_new);
	whisker_arr_init_t(entities_new->arr, 1);

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
			whisker_ecs_entity_id e = whisker_ecs_e_create_named(entities, entity_names + search_index);

			/* debug_printf("%zu-%zu: %s\n", search_index, i, entity_names + search_index); */

			// add the entity id to the final list, and reset name array
			whisker_arr_ensure_alloc(entities_new->arr, entities_new->arr_length + 1);
			entities_new->arr[entities_new->arr_length++] = e;

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
void whisker_ecs_e_sort_entity_array(whisker_ecs_entity_id *entities, size_t length)
{
	qsort(entities, length, sizeof(whisker_ecs_entity_id), whisker_ecs_e_compare_entity_ids_);
}
