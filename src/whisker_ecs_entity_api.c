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
w_entity_id w_entity_api_create_(struct w_world *world)
{
	pthread_mutex_lock(&world->entities->create_entity_mutex);

	w_entity_id e = w_entity_api_create_unsafe_(world);

	pthread_mutex_unlock(&world->entities->create_entity_mutex);

	return e;
}

// create a new entity as a deferred action
w_entity_id w_entity_api_create_deferred_(struct w_world *world)
{
	w_entity_id e = w_entity_api_create_(world);
	if (e.id == 0)
	{
		return e;
	}

	// set the entity to dead and add it to the deferred entities
	world->entities->entities[e.index].destroyed = true;
	w_create_deferred_entity_action(world, e, W_ENTITY_DEFERRED_ACTION_CREATE);

	return e;
}

// create a new entity as a deferred action without using previously recycled entities
w_entity_id w_entity_api_create_new_deferred_(struct w_world *world)
{
	pthread_mutex_lock(&world->entities->create_entity_mutex);

	w_entity_id e = w_entity_api_create_new_(world);

	pthread_mutex_unlock(&world->entities->create_entity_mutex);

	// set the entity to dead and add it to the deferred entities
	world->entities->entities[e.index].destroyed = true;
	w_create_deferred_entity_action(world, e, W_ENTITY_DEFERRED_ACTION_CREATE);

	return e;
}

// creates and sets an entity, either new or recycled
w_entity_id w_entity_api_create_unsafe_(struct w_world *world)
{
	if (world->entities->destroyed_entities_length)
	{
		return world->entities->entities[w_entity_api_pop_recycled_(world)].id;
	}
	return w_entity_api_create_new_(world);
}

// pop a recycled entity from the destroyed_entities stack
w_entity_idx w_entity_api_pop_recycled_(struct w_world *world)
{
	if (world->entities->destroyed_entities_length > 0)
	{
		w_entity_idx recycled_index = --world->entities->destroyed_entities_length;
		world->entities->entities[recycled_index].destroyed = false;
		return world->entities->destroyed_entities[recycled_index];
	}

	return 0;
}

// create a new entity and add it to the entities list
w_entity_id w_entity_api_create_new_(struct w_world *world)
{
	const size_t new_idx = world->entities->entities_length++;

	// reallocate the entity array if required
	w_array_ensure_alloc_block_size(
		world->entities->entities, 
		(new_idx + 1), 
		W_ENTITY_REALLOC_BLOCK_SIZE
	);

	// init the newly created entity with valid index
	world->entities->entities[new_idx] = (struct w_entity) {
		.id.index = new_idx,
	};
	return world->entities->entities[new_idx].id;
}

// create entity with a name, or return an existing entity with the same name
w_entity_id w_entity_api_create_named_(struct w_world *world, char *name)
{
	pthread_mutex_lock(&world->entities->create_entity_mutex);

	w_entity_id e = w_entity_api_create_named_unsafe_(world, name);

	pthread_mutex_unlock(&world->entities->create_entity_mutex);

	return e;
}

// create a new named entity as a deferred action
w_entity_id w_entity_api_create_named_deferred_(struct w_world *world, char *name)
{
	pthread_mutex_lock(&world->entities->create_entity_mutex);

	w_entity_id e = w_entity_api_create_named_unsafe_(world, name);

	pthread_mutex_unlock(&world->entities->create_entity_mutex);
	if (e.id == 0)
	{
		return e;
	}

	// set the entity to dead and add it to the deferred entities
	world->entities->entities[e.index].destroyed = true;
	w_create_deferred_entity_action(world, e, W_ENTITY_DEFERRED_ACTION_CREATE);

	return e;
}

// create a new entity with the given name
w_entity_id w_entity_api_create_named_unsafe_(struct w_world *world, char* name)
{
	struct w_entity *e = w_get_named_entity(world, name);
	if (e)
	{
		return e->id;
	}

	// create new entity with name
	w_entity_id e_id = w_entity_api_create_unsafe_(world);

	// set the name
	w_set_entity_name(world, e_id, name);

	return e_id;
}

// recycle an entity into the destroyed entities stack
void w_entity_api_recycle_(struct w_world *world, w_entity_id entity_id)
{
	// increment version
	struct w_entity *e = w_get_entity(world, entity_id);
	e->id.version++;

	// clear name pointer
	if (e->name != NULL)
	{
		w_trie_remove_value_str(world->entities->entity_names, e->name);
		free_null(e->name);
		e->name = NULL;
	}

	// push to dead entities stack
	w_array_ensure_alloc_block_size(
		world->entities->destroyed_entities, 
		(world->entities->destroyed_entities_length + 1), 
		W_ENTITY_DESTROYED_REALLOC_BLOCK_SIZE
	);
	world->entities->destroyed_entities[world->entities->destroyed_entities_length++] = e->id.index;
}

// destroy an entity, incrementing it's version and adding it to the destroyed
// entities stack
void w_entity_api_destroy_(struct w_world *world, w_entity_id entity_id)
{
	// mark entity as destroyed
	_Atomic bool currently_destroyed = atomic_load(&world->entities->entities[entity_id.index].destroyed);
	if (!currently_destroyed)
	{
		w_entity_api_recycle_(world, entity_id);
    	atomic_store(&world->entities->entities[entity_id.index].destroyed, true);
	}
	return;
}

// destroy an entity as a deferred action
void w_entity_api_destroy_deferred_(struct w_world *world, w_entity_id entity_id)
{
	_Atomic bool currently_destroyed = atomic_load(&world->entities->entities[entity_id.index].destroyed);
	if (!currently_destroyed)
	{
    	w_create_deferred_entity_action(world, entity_id, W_ENTITY_DEFERRED_ACTION_DESTROY);
    	atomic_store(&world->entities->entities[entity_id.index].destroyed, true);
	}
	return;
}
