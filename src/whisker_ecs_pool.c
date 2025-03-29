/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_pool
 * @created     : Thursday Mar 27, 2025 17:30:13 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_pool.h"

// create an instance of an entity pool
whisker_ecs_pool *whisker_ecs_p_create()
{
	return whisker_mem_xcalloc_t(1, whisker_ecs_pool);
}

// create and init an instance of an entity pool
whisker_ecs_pool *whisker_ecs_p_create_and_init(whisker_ecs_components *components, whisker_ecs_entities *entities, size_t count, size_t realloc_count)
{
	whisker_ecs_pool *pool = whisker_ecs_p_create();
	whisker_ecs_p_init(pool, components, entities, count, realloc_count);

	return pool;
}

// init an instance of an entity pool
void whisker_ecs_p_init(whisker_ecs_pool *pool, whisker_ecs_components *components, whisker_ecs_entities *entities, size_t count, size_t realloc_count)
{
	whisker_arr_init_t(pool->component_ids, 1);
	whisker_arr_init_t(pool->entity_pool, count);
	pool->component_ids_set = whisker_ss_create_t(whisker_ecs_entity_id);

	pool->realloc_block_size = realloc_count;
	pool->inital_size = count;
	pool->cache_misses = 0;

	// create prototype entity
	pool->prototype_entity_id = whisker_ecs_e_create(entities);
	whisker_ecs_e_make_unmanaged(entities, pool->prototype_entity_id);

	pthread_mutex_init(&pool->entity_pool_mutex, NULL);

	pool->components = components;
	pool->entities = entities;
}

// deallocate an entity pool's allocations
void whisker_ecs_p_free(whisker_ecs_pool *pool)
{
	free(pool->component_ids);
	free(pool->entity_pool);
	whisker_ss_free_all(pool->component_ids_set);
	pthread_mutex_destroy(&pool->entity_pool_mutex);
}

// deallocate and free an entity pool
void whisker_ecs_p_free_all(whisker_ecs_pool *pool)
{
	whisker_ecs_p_free(pool);
	free(pool);
}

// set a component on the prototype entity for a pool
void whisker_ecs_p_set_prototype_component_f(whisker_ecs_pool *pool, whisker_ecs_entity_id component_id, size_t component_size, void *prototype_value)
{
	if (!whisker_ss_contains(pool->component_ids_set, component_id.index))
	{
		// ensure space for component IDs
		size_t component_idx = atomic_fetch_add(&pool->component_ids_length, 1);
		
		if((component_idx + 1) * sizeof(*pool->component_ids) > pool->component_ids_size)
		{
			pthread_mutex_lock(&pool->entity_pool_mutex);

			whisker_arr_ensure_alloc_block_size(
				pool->component_ids, 
				(component_idx + 1),
				WHISKER_ECS_POOL_REALLOC_BLOCK_SIZE
			);

			pthread_mutex_unlock(&pool->entity_pool_mutex);
		}

		pool->component_ids[component_idx] = component_id;
		whisker_ss_set(pool->component_ids_set, component_id.index, &component_id);
	}

	// set component data
	whisker_ecs_c_set_component(pool->components, component_id, component_size, pool->prototype_entity_id, prototype_value);
}

// set a named component on the prototype entity for a pool
void whisker_ecs_p_set_prototype_named_component_f(whisker_ecs_pool *pool, char* component_name, size_t component_size, void *prototype_value)
{
	whisker_ecs_p_set_prototype_component_f(pool, whisker_ecs_e_create_named(pool->entities, component_name), component_size, prototype_value);
}

// set the prototype entity for this pool, clearing previously set component IDs
void whisker_ecs_p_set_prototype_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id prototype_entity_id)
{
	pool->component_ids_length = 0;
	pool->prototype_entity_id = prototype_entity_id;

	// TODO: build this into sparse set to allow easy reset
	whisker_ss_free_all(pool->component_ids_set);
	pool->component_ids_set = whisker_ss_create_t(whisker_ecs_entity_id);
}

// request an entity from the pool
whisker_ecs_entity_id whisker_ecs_p_request_entity(whisker_ecs_pool *pool)
{
	/* printf("pool %p entities in pool: ", pool); */
	/* for (int i = 0; i < pool->entity_pool_length; ++i) */
	/* { */
	/* 	printf("%zu ", pool->entity_pool[i]); */
	/* } */
	/* printf("\n"); */

	whisker_ecs_entity_id e;

	if (pool->entity_pool_length > 0)
	{
		size_t entity_idx = atomic_fetch_sub(&pool->entity_pool_length, 1) - 1;

		e = pool->entity_pool[entity_idx];
		/* printf("pool %p get from pool index %zu entity %zu\n", pool, entity_idx, e.id); */
	}	
	else
	{
		// create and init new entity
		e = whisker_ecs_p_create_entity_deferred(pool);
		/* printf("pool %p create new entity %zu\n", pool, e.id); */

		atomic_fetch_add_explicit(&(pool->cache_misses), 1, memory_order_relaxed);

		// refill the pool
		whisker_ecs_p_realloc_entities(pool);
	}

	whisker_ecs_p_init_entity(pool, e);

	return e;
}

// issue a real create request for an entity this pool can use
whisker_ecs_entity_id whisker_ecs_p_create_entity_deferred(whisker_ecs_pool *pool)
{
	whisker_ecs_entity_id e = whisker_ecs_e_create(pool->entities);

	// make sure to perform the soft-recycle actions on the entity
	pool->entities->entities[e.index].destroyed = true;
	pool->entities->entities[e.index].unmanaged = true;
	pool->entities->entities[e.index].managed_by = pool;
	whisker_ecs_e_add_deffered_action(pool->entities, (whisker_ecs_entity_deferred_action){.id = e, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	return e;
}

// initialise the entities components using the pool's prototype entity
void whisker_ecs_p_init_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id)
{
	// copy component data from prototype entity
	for (size_t i = 0; i < pool->component_ids_length; ++i)
	{
		whisker_sparse_set *component_array = whisker_ecs_c_get_component_array(pool->components, pool->component_ids[i]);
		/* whisker_ss_set(component_array, entity_id.index, whisker_ss_get(component_array, pool->prototype_entity_id.index)); */
		whisker_ecs_c_create_deferred_action(pool->components, pool->component_ids[i], entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET, whisker_ss_get(component_array, pool->prototype_entity_id.index), component_array->element_size, false);
	}

	// turn into managed entity
	whisker_ecs_e_make_managed(pool->entities, entity_id);
}

// return an entity to the pool
void whisker_ecs_p_return_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id)
{
	// increment the pool length
	size_t entity_idx = atomic_fetch_add(&pool->entity_pool_length, 1);

	/* printf("pool %p return entity %zu to index %zu\n", pool, entity_id, entity_idx); */

	// grow array if required using lock
	if((entity_idx + 1) * sizeof(*pool->entity_pool) > pool->entity_pool_size)
	{
		pthread_mutex_lock(&pool->entity_pool_mutex);

		whisker_arr_ensure_alloc_block_size(
			pool->entity_pool, 
			(entity_idx + 1),
			WHISKER_ECS_POOL_REALLOC_BLOCK_SIZE
		);

		pthread_mutex_unlock(&pool->entity_pool_mutex);
	}

	pool->entity_pool[entity_idx] = entity_id;
	pool->entities->entities[entity_id.index].id.version++;

	// turn into unmanaged entity
	whisker_ecs_e_make_unmanaged(pool->entities, entity_id);
}

// create new entities topping up the pool
void whisker_ecs_p_realloc_entities(whisker_ecs_pool *pool)
{
	/* printf("pool %p realloc entities block size %zu cache misses %zu\n", pool, pool->realloc_block_size, pool->cache_misses); */
	whisker_ecs_p_create_and_return(pool, (pool->cache_misses <= 1) ? pool->inital_size : pool->realloc_block_size * pool->cache_misses);
}

// create and add entity to the pool
void whisker_ecs_p_create_and_return(whisker_ecs_pool *pool, size_t count)
{
	/* printf("pool %p creating and adding %zu entities\n", pool, count); */
	for (int i = 0; i < count; ++i)
	{
		whisker_ecs_entity_id e = whisker_ecs_p_create_entity_deferred(pool);
		whisker_ecs_p_init_entity(pool, e);
		whisker_ecs_p_return_entity(pool, e);
	}
}
