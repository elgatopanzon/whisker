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
whisker_ecs_entities *whisker_ecs_create_and_init_entities_container_()
{
	whisker_ecs_entities *e = whisker_ecs_create_entities_container_();
	whisker_ecs_init_entities_container_(e);

	return e;
}

// allocate instance of entities container
whisker_ecs_entities *whisker_ecs_create_entities_container_()
{
	whisker_ecs_entities *e = whisker_mem_xcalloc_t(1, *e);
	return e;
}

// init entity arrays for an entity container
void whisker_ecs_init_entities_container_(whisker_ecs_entities *entities)
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
void whisker_ecs_free_entities_container_(whisker_ecs_entities *entities)
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
void whisker_ecs_free_entities_all_(whisker_ecs_entities *entities)
{
	whisker_ecs_free_entities_container_(entities);
	free(entities);
}


/***********************
*  Entity Management  *
***********************/

// request an entity ID to be created or recycled
whisker_ecs_entity_id whisker_ecs_create_entity(struct whisker_ecs_world *world)
{
	return whisker_ecs_entity_api_create_(world);
}

// request an entity ID to be created or recycled, providing a name
// note: names are unique, creating an entity with the same name returns the existing entity if it exists
whisker_ecs_entity_id whisker_ecs_create_named_entity(struct whisker_ecs_world *world, char* name)
{
	return whisker_ecs_entity_api_create_named_(world, name);
}

// immediately destroy the given entity ID
void whisker_ecs_destroy_entity(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity *e = whisker_ecs_get_entity(world, entity_id);
	if (e->managed_by != NULL)
	{
		// HACK: for now we assume everything managed is managed by a pool
		whisker_ecs_pool *pool = e->managed_by;
		whisker_ecs_return_pool_entity(pool, entity_id);
		return;
	}

	whisker_ecs_entity_api_destroy_(world, entity_id);
}

// immediately soft-destroy the given entity ID with an atomic operation
void whisker_ecs_set_entity_unmanaged(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
    atomic_store(&world->entities->entities[entity_id.index].unmanaged, true);
}

// immediately soft-revive the given entity ID with an atomic operation
void whisker_ecs_set_entity_managed(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
    atomic_store(&world->entities->entities[entity_id.index].unmanaged, false);
}

// request to create an entity, deferring the creation until end of current frame
whisker_ecs_entity_id whisker_ecs_create_entity_deferred(struct whisker_ecs_world *world)
{
	return whisker_ecs_entity_api_create_deferred_(world);
}

// request to create an entity with a name, deferring the creation until end of current frame
whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(struct whisker_ecs_world *world, char* name)
{
	return whisker_ecs_entity_api_create_named_deferred_(world, name);
}

// request to destroy the provided entity ID at the end of current frame
void whisker_ecs_destroy_entity_deferred(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_api_destroy_deferred_(world, entity_id);
	return;
}

// create a deferred entity action
void whisker_ecs_create_deferred_entity_action(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id, WHISKER_ECS_ENTITY_DEFERRED_ACTION action)
{
	whisker_ecs_entity_deferred_action deferred_action = {.id = entity_id, .action = action};

	size_t deferred_action_idx = atomic_fetch_add(&world->entities->deferred_actions_length, 1);

	if((deferred_action_idx + 1) * sizeof(*world->entities->deferred_actions) > world->entities->deferred_actions_size)
	{
		pthread_mutex_lock(&world->entities->deferred_actions_mutex);

		whisker_arr_ensure_alloc_block_size(
			world->entities->deferred_actions, 
			(deferred_action_idx + 1),
			WHISKER_ECS_ENTITY_DEFERRED_ACTION_REALLOC_BLOCK_SIZE
		);

		pthread_mutex_unlock(&world->entities->deferred_actions_mutex);
	}
	
	world->entities->deferred_actions[deferred_action_idx] = deferred_action;
}


/***********************
*  utility functions  *
***********************/

// shortcut to get the entity struct from the given ID
whisker_ecs_entity* whisker_ecs_get_entity(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	if (entity_id.index + 1 > world->entities->entities_length)
	{
		return NULL;
	}

	return &world->entities->entities[entity_id.index];
}

// convert a numeric ID to an entity ID
inline whisker_ecs_entity_id whisker_ecs_entity_id_from_raw(whisker_ecs_entity_id_raw id)
{
	return (whisker_ecs_entity_id){.id = id};
}

// shortcut to get the entity with the given name
// note: this will return NULL if no entity exists with this name
whisker_ecs_entity* whisker_ecs_get_named_entity(struct whisker_ecs_world *world, char* entity_name)
{
	// lookup entity by name and return a match, or NULL
	whisker_ecs_entity_index *e_idx = whisker_trie_search_value_str(world->entities->entity_names, entity_name);
	if (e_idx == NULL)
	{
		return NULL;
	}

	return &world->entities->entities[*e_idx];
}

// check if an entity is dead by comparing the provided entity version with the
// one in the entities array
bool whisker_ecs_is_entity_alive(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	return (whisker_ecs_get_entity(world, entity_id)->id.version == entity_id.version);
}

// get the current count of entities in existence including alive and destroyed
size_t whisker_ecs_entity_count(struct whisker_ecs_world *world)
{
	return world->entities->entities_length;
}

// get current count of alive entities
size_t whisker_ecs_alive_entity_count(struct whisker_ecs_world *world)
{
	return whisker_ecs_entity_count(world) - whisker_ecs_destroyed_entity_count(world);
}

// get current count of destroyed entities
size_t whisker_ecs_destroyed_entity_count(struct whisker_ecs_world *world)
{
	return world->entities->destroyed_entities_length;
}

// convert a string of named entities in the format "name1,name2,name3" to an
// array of entities, creating them if they don't already exist
struct whisker_ecs_entity_id_array* whisker_ecs_batch_create_named_entities(struct whisker_ecs_world *world, char* entity_names_str)
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
			whisker_ecs_entity_id e = whisker_ecs_entity_api_create_named_(world, entity_names + search_index);

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
int whisker_ecs_compared_entity_ids_(const void *id_a, const void *id_b)
{
	return ((*(whisker_ecs_entity_id*)id_a).id - (*(whisker_ecs_entity_id*)id_b).id);
}

// sort an array of entities in ascending order
void whisker_ecs_sort_entity_array_(whisker_ecs_entity_id *entities, size_t length)
{
	qsort(entities, length, sizeof(whisker_ecs_entity_id), whisker_ecs_compared_entity_ids_);
}
