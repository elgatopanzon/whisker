/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity
 * @created     : Tuesday Apr 01, 2025 19:46:28 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"


/*************************************
*  entities struct management  *
*************************************/
// create an instance of entities container
struct w_entities *w_create_and_init_entities_container_()
{
	struct w_entities *e = w_create_entities_container_();
	w_init_entities_container_(e);

	return e;
}

// allocate instance of entities container
struct w_entities *w_create_entities_container_()
{
	struct w_entities *e = w_mem_xcalloc_t(1, *e);
	return e;
}

// init entity arrays for an entity container
void w_init_entities_container_(struct w_entities *entities)
{
	// create and allocate entity arrays
	w_array_init_t(
		entities->entities, 
		W_ENTITY_REALLOC_BLOCK_SIZE
	);
	w_array_init_t(
		entities->destroyed_entities, 
		W_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE
	);
	w_array_init_t(
		entities->deferred_actions,
		W_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
	);

	// create entity names trie
	entities->entity_names = w_mem_xcalloc_t(1, *entities->entity_names);

	// init pthread mutexes
	pthread_mutex_init(&entities->deferred_actions_mutex, NULL);
	pthread_mutex_init(&entities->create_entity_mutex, NULL);
}

// free arrays on instance of entities container
void w_free_entities_container_(struct w_entities *entities)
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
	w_trie_free_node_values(entities->entity_names);
	w_trie_free_nodes(entities->entity_names);
	free(entities->entity_names);
	free(entities->deferred_actions);
	pthread_mutex_destroy(&entities->deferred_actions_mutex);
	pthread_mutex_destroy(&entities->create_entity_mutex);
}

// free entity container and containing arrays
void w_free_entities_all_(struct w_entities *entities)
{
	w_free_entities_container_(entities);
	free(entities);
}


/***********************
*  Entity Management  *
***********************/

// request an entity ID to be created or recycled
w_entity_id w_create_entity_non_deferred(struct w_world *world)
{
	return w_entity_api_create_(world);
}

// request an entity ID to be created or recycled, providing a name
// note: names are unique, creating an entity with the same name returns the existing entity if it exists
w_entity_id w_create_named_entity_non_deferred(struct w_world *world, char* name)
{
	return w_entity_api_create_named_(world, name);
}

// immediately destroy the given entity ID
void w_destroy_entity_non_deferred(struct w_world *world, w_entity_id entity_id)
{
	struct w_entity *e = w_get_entity(world, entity_id);
	if (e->managed_by != NULL)
	{
		// HACK: for now we assume everything managed is managed by a pool
		struct w_pool *pool = e->managed_by;
		w_return_pool_entity(pool, entity_id);
		return;
	}

	w_entity_api_destroy_(world, entity_id);
}

// immediately soft-destroy the given entity ID with an atomic operation
void w_set_entity_unmanaged(struct w_world *world, w_entity_id entity_id)
{
    atomic_store(&world->entities->entities[entity_id.index].unmanaged, true);
}

// immediately soft-revive the given entity ID with an atomic operation
void w_set_entity_managed(struct w_world *world, w_entity_id entity_id)
{
    atomic_store(&world->entities->entities[entity_id.index].unmanaged, false);
}

// request to create an entity, deferring the creation until end of current frame
w_entity_id w_create_entity(struct w_world *world)
{
	return w_entity_api_create_deferred_(world);
}

// request to create an entity with a name, deferring the creation until end of current frame
w_entity_id w_create_named_entity(struct w_world *world, char* name)
{
	return w_entity_api_create_named_deferred_(world, name);
}

// request to destroy the provided entity ID at the end of current frame
void w_destroy_entity(struct w_world *world, w_entity_id entity_id)
{
	w_entity_api_destroy_deferred_(world, entity_id);
	return;
}

// create a deferred entity action
void w_create_deferred_entity_action(struct w_world *world, w_entity_id entity_id, enum W_ENTITY_DEFERRED_ACTION action)
{
	struct w_entity_deferred_action deferred_action = {.id = entity_id, .action = action};

	size_t deferred_action_idx = atomic_fetch_add(&world->entities->deferred_actions_length, 1);

	if((deferred_action_idx + 1) * sizeof(*world->entities->deferred_actions) > world->entities->deferred_actions_size)
	{
		pthread_mutex_lock(&world->entities->deferred_actions_mutex);

		w_array_ensure_alloc_block_size(
			world->entities->deferred_actions, 
			(deferred_action_idx + 1),
			W_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
		);

		pthread_mutex_unlock(&world->entities->deferred_actions_mutex);
	}
	
	world->entities->deferred_actions[deferred_action_idx] = deferred_action;
}


/***********************
*  utility functions  *
***********************/

// shortcut to get the entity struct from the given ID
struct w_entity* w_get_entity(struct w_world *world, w_entity_id entity_id)
{
	if (entity_id.index + 1 > world->entities->entities_length)
	{
		return NULL;
	}

	return &world->entities->entities[entity_id.index];
}

// convert a numeric ID to an entity ID
inline w_entity_id w_entity_id_from_raw(w_entity_id_raw id)
{
	return (w_entity_id){.id = id};
}

// shortcut to get the entity with the given name
// note: this will return NULL if no entity exists with this name
struct w_entity* w_get_named_entity(struct w_world *world, char* entity_name)
{
	// lookup entity by name and return a match, or NULL
	w_entity_idx *e_idx = w_trie_search_value_str(world->entities->entity_names, entity_name);
	if (e_idx == NULL)
	{
		return NULL;
	}

	return &world->entities->entities[*e_idx];
}

// set the name for an entity
void w_set_entity_name(struct w_world *world, w_entity_id entity_id, char* name)
{
	w_entity_idx *trie_id = w_mem_xcalloc_t(1, *trie_id);
	*trie_id = entity_id.index;
	w_trie_set_value_str(world->entities->entity_names, name, trie_id);

	// copy the name into the entities name
	struct w_entity *e = w_get_entity(world, entity_id);
	e->name = w_mem_xmalloc(strlen(name) + 1);
	strncpy(e->name, name, strlen(name) + 1);
}


// check if an entity is dead by comparing the provided entity version with the
// one in the entities array
bool w_is_entity_alive(struct w_world *world, w_entity_id entity_id)
{
	return (w_get_entity(world, entity_id)->id.version == entity_id.version);
}

// get the current count of entities in existence including alive and destroyed
size_t w_entity_count(struct w_world *world)
{
	return world->entities->entities_length;
}

// get current count of alive entities
size_t w_alive_entity_count(struct w_world *world)
{
	return w_entity_count(world) - w_destroyed_entity_count(world);
}

// get current count of destroyed entities
size_t w_destroyed_entity_count(struct w_world *world)
{
	return world->entities->destroyed_entities_length;
}

// convert a string of named entities in the format "name1,name2,name3" to an
// array of entities, creating them if they don't already exist
struct w_entity_id_arr* w_batch_create_named_entities(struct w_world *world, char* entity_names_str)
{
	// entity list derived from string entity names
	struct w_entity_id_arr *entities_new = w_mem_xcalloc_t(1, *entities_new);
	w_array_init_t(entities_new->arr, 1);

	size_t names_length = strlen(entity_names_str);
	char* entity_names = w_mem_xmalloc(names_length + 1);
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
			w_entity_id e = w_entity_api_create_named_(world, entity_names + search_index);

			/* debug_printf("%zu-%zu: %s\n", search_index, i, entity_names + search_index); */

			// add the entity id to the final list, and reset name array
			w_array_ensure_alloc(entities_new->arr, entities_new->arr_length + 1);
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
int w_compared_entity_ids_(const void *id_a, const void *id_b)
{
	return ((*(w_entity_id*)id_a).id - (*(w_entity_id*)id_b).id);
}

// sort an array of entities in ascending order
void w_sort_entity_array_(w_entity_id *entities, size_t length)
{
	qsort(entities, length, sizeof(w_entity_id), w_compared_entity_ids_);
}
