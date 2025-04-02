/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity_api
 * @created     : Tuesday Apr 01, 2025 19:49:12 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"

/*************************
*  Internal Entity API  *
*************************/

// request a new entity
whisker_ecs_entity_id whisker_ecs_entity_api_create_(struct whisker_ecs_world *world)
{
	pthread_mutex_lock(&world->entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_entity_api_create_unsafe_(world);

	pthread_mutex_unlock(&world->entities->create_entity_mutex);

	return e;
}

// create a new entity as a deferred action
whisker_ecs_entity_id whisker_ecs_entity_api_create_deferred_(struct whisker_ecs_world *world)
{
	whisker_ecs_entity_id e = whisker_ecs_entity_api_create_(world);
	if (e.id == 0)
	{
		return e;
	}

	// set the entity to dead and add it to the deferred entities
	world->entities->entities[e.index].destroyed = true;
	whisker_ecs_create_deferred_entity_action(world, e, WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE);

	return e;
}

// create a new entity as a deferred action without using previously recycled entities
whisker_ecs_entity_id whisker_ecs_entity_api_create_new_deferred_(struct whisker_ecs_world *world)
{
	pthread_mutex_lock(&world->entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_entity_api_create_new_(world);

	pthread_mutex_unlock(&world->entities->create_entity_mutex);

	// set the entity to dead and add it to the deferred entities
	world->entities->entities[e.index].destroyed = true;
	whisker_ecs_create_deferred_entity_action(world, e, WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE);

	return e;
}

// creates and sets an entity, either new or recycled
whisker_ecs_entity_id whisker_ecs_entity_api_create_unsafe_(struct whisker_ecs_world *world)
{
	if (world->entities->destroyed_entities_length)
	{
		return world->entities->entities[whisker_ecs_entity_api_pop_recycled_(world)].id;
	}
	return whisker_ecs_entity_api_create_new_(world);
}

// pop a recycled entity from the destroyed_entities stack
whisker_ecs_entity_index whisker_ecs_entity_api_pop_recycled_(struct whisker_ecs_world *world)
{
	if (world->entities->destroyed_entities_length > 0)
	{
		whisker_ecs_entity_index recycled_index = --world->entities->destroyed_entities_length;
		world->entities->entities[recycled_index].destroyed = false;
		return world->entities->destroyed_entities[recycled_index];
	}

	return 0;
}

// create a new entity and add it to the entities list
whisker_ecs_entity_id whisker_ecs_entity_api_create_new_(struct whisker_ecs_world *world)
{
	const size_t new_idx = world->entities->entities_length++;

	// reallocate the entity array if required
	whisker_arr_ensure_alloc_block_size(
		world->entities->entities, 
		(new_idx + 1), 
		WHISKER_ECS_ENTITY_REALLOC_BLOCK_SIZE
	);

	// init the newly created entity with valid index
	world->entities->entities[new_idx] = (whisker_ecs_entity) {
		.id.index = new_idx,
	};
	return world->entities->entities[new_idx].id;
}

// set the name for an entity
void whisker_ecs_e_set_name(struct whisker_ecs_world *world, char* name, whisker_ecs_entity_id entity_id)
{
	whisker_ecs_entity_index *trie_id = whisker_mem_xcalloc_t(1, *trie_id);
	*trie_id = entity_id.index;
	whisker_trie_set_value_str(world->entities->entity_names, name, trie_id);

	// copy the name into the entities name
	whisker_ecs_entity *e = whisker_ecs_get_entity(world, entity_id);
	e->name = whisker_mem_xmalloc(strlen(name) + 1);
	strncpy(e->name, name, strlen(name) + 1);
}

// create entity with a name, or return an existing entity with the same name
whisker_ecs_entity_id whisker_ecs_entity_api_create_named_(struct whisker_ecs_world *world, char *name)
{
	pthread_mutex_lock(&world->entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_entity_api_create_named_unsafe_(world, name);

	pthread_mutex_unlock(&world->entities->create_entity_mutex);

	return e;
}

// create a new named entity as a deferred action
whisker_ecs_entity_id whisker_ecs_entity_api_create_named_deferred_(struct whisker_ecs_world *world, char *name)
{
	pthread_mutex_lock(&world->entities->create_entity_mutex);

	whisker_ecs_entity_id e = whisker_ecs_entity_api_create_named_unsafe_(world, name);

	pthread_mutex_unlock(&world->entities->create_entity_mutex);
	if (e.id == 0)
	{
		return e;
	}

	// set the entity to dead and add it to the deferred entities
	world->entities->entities[e.index].destroyed = true;
	whisker_ecs_create_deferred_entity_action(world, e, WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE);

	return e;
}

// create a new entity with the given name
whisker_ecs_entity_id whisker_ecs_entity_api_create_named_unsafe_(struct whisker_ecs_world *world, char* name)
{
	whisker_ecs_entity *e = whisker_ecs_get_named_entity(world, name);
	if (e)
	{
		return e->id;
	}

	// create new entity with name
	whisker_ecs_entity_id e_id = whisker_ecs_entity_api_create_unsafe_(world);

	// set the name
	whisker_ecs_e_set_name(world, name, e_id);

	return e_id;
}

// recycle an entity into the destroyed entities stack
void whisker_ecs_entity_api_recycle_(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	// increment version
	whisker_ecs_entity *e = whisker_ecs_get_entity(world, entity_id);
	e->id.version++;

	// clear name pointer
	if (e->name != NULL)
	{
		whisker_trie_remove_value_str(world->entities->entity_names, e->name);
		free_null(e->name);
		e->name = NULL;
	}

	// push to dead entities stack
	whisker_arr_ensure_alloc_block_size(
		world->entities->destroyed_entities, 
		(world->entities->destroyed_entities_length + 1), 
		WHISKER_ECS_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE
	);
	world->entities->destroyed_entities[world->entities->destroyed_entities_length++] = e->id.index;
}

// destroy an entity, incrementing it's version and adding it to the destroyed
// entities stack
void whisker_ecs_entity_api_destroy_(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	// mark entity as destroyed
	_Atomic bool currently_destroyed = atomic_load(&world->entities->entities[entity_id.index].destroyed);
	if (!currently_destroyed)
	{
		whisker_ecs_entity_api_recycle_(world, entity_id);
    	atomic_store(&world->entities->entities[entity_id.index].destroyed, true);
	}
	return;
}

// destroy an entity as a deferred action
void whisker_ecs_entity_api_destroy_deferred_(struct whisker_ecs_world *world, whisker_ecs_entity_id entity_id)
{
	_Atomic bool currently_destroyed = atomic_load(&world->entities->entities[entity_id.index].destroyed);
	if (!currently_destroyed)
	{
    	whisker_ecs_create_deferred_entity_action(world, entity_id, WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY);
    	atomic_store(&world->entities->entities[entity_id.index].destroyed, true);
	}
	return;
}
