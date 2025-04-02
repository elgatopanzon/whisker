/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_entity_pool
 * @created     : Tuesday Apr 01, 2025 19:49:47 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"

// create an instance of an entity pool
whisker_ecs_pool *whisker_ecs_create_entity_pool()
{
	return whisker_mem_xcalloc_t(1, whisker_ecs_pool);
}

// create and init an instance of an entity pool
whisker_ecs_pool *whisker_ecs_create_and_init_entity_pool(struct whisker_ecs_world *world, size_t count, size_t realloc_count)
{
	whisker_ecs_pool *pool = whisker_ecs_create_entity_pool();
	whisker_ecs_init_entity_pool(pool, world, count, realloc_count);

	return pool;
}

// init an instance of an entity pool
void whisker_ecs_init_entity_pool(whisker_ecs_pool *pool, struct whisker_ecs_world *world, size_t count, size_t realloc_count)
{
	whisker_arr_init_t(pool->component_ids, 1);
	whisker_arr_init_t(pool->entity_pool, count);
	pool->component_ids_set = whisker_ss_create_t(whisker_ecs_entity_id);

	pool->realloc_block_size = realloc_count;
	pool->inital_size = count;
	pool->stat_cache_misses = 0;

	// create prototype entity
	pool->prototype_entity_id = whisker_ecs_create_entity(world);
	whisker_ecs_set_entity_unmanaged(world, pool->prototype_entity_id);

	pthread_mutex_init(&pool->entity_pool_mutex, NULL);

	// force enable propagate changes
	pool->propagate_component_changes = true;

	pool->world = world;
}

// deallocate an entity pool's allocations
void whisker_ecs_free_entity_pool(whisker_ecs_pool *pool)
{
	free(pool->component_ids);
	free(pool->entity_pool);
	whisker_ss_free_all(pool->component_ids_set);
	pthread_mutex_destroy(&pool->entity_pool_mutex);
}

// deallocate and free an entity pool
void whisker_ecs_free_entity_pool_all(whisker_ecs_pool *pool)
{
	whisker_ecs_free_entity_pool(pool);
	free(pool);
}

// set a component on the prototype entity for a pool
void whisker_ecs_set_entity_pool_component_f(whisker_ecs_pool *pool, whisker_ecs_entity_id component_id, size_t component_size, void *prototype_value)
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
	whisker_ecs_set_component_(pool->world, component_id, component_size, pool->prototype_entity_id, prototype_value);
}

// set a named component on the prototype entity for a pool
void whisker_ecs_set_entity_pool_named_component_f(whisker_ecs_pool *pool, char* component_name, size_t component_size, void *prototype_value)
{
	whisker_ecs_set_entity_pool_component_f(pool, whisker_ecs_entity_api_create_named_(pool->world, component_name), component_size, prototype_value);
}

// set the prototype entity for this pool, clearing previously set component IDs
void whisker_ecs_set_entity_pool_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id prototype_entity_id)
{
	pool->component_ids_length = 0;
	pool->prototype_entity_id = prototype_entity_id;

	// TODO: build this into sparse set to allow easy reset
	whisker_ss_free_all(pool->component_ids_set);
	pool->component_ids_set = whisker_ss_create_t(whisker_ecs_entity_id);
}

// request an entity from the pool
whisker_ecs_entity_id whisker_ecs_request_pool_entity(whisker_ecs_pool *pool)
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
		/* debug_log(DEBUG, ecs:pool_request, "pool %p get from pool index %zu entity %zu\n", pool, entity_idx, e.id); */
	}	
	else
	{
		// create and init new entity
		e = whisker_ecs_create_pool_entity_deferred_(pool);

		atomic_fetch_add_explicit(&(pool->stat_cache_misses), 1, memory_order_relaxed);

		debug_log(DEBUG, ecs:pool_request, "pool %p create new entity %zu (requests %zu returns %zu misses %zu)", pool, e.id, pool->stat_total_requests, pool->stat_total_returns, pool->stat_cache_misses);

		// refill the pool
		whisker_ecs_realloc_pool_entities_(pool);
	}

	whisker_ecs_init_pool_entity_(pool, e, pool->propagate_component_changes);

	// increase stats
	atomic_fetch_add_explicit(&pool->stat_total_requests, 1, memory_order_relaxed);

	return e;
}

// issue a real create request for an entity this pool can use
whisker_ecs_entity_id whisker_ecs_create_pool_entity_deferred_(whisker_ecs_pool *pool)
{
	whisker_ecs_entity_id e = whisker_ecs_entity_api_create_new_deferred_(pool->world);

	// make sure to perform the soft-recycle actions on the entity
	pool->world->entities->entities[e.index].unmanaged = true;
	pool->world->entities->entities[e.index].managed_by = pool;

	return e;
}

// initialise the entities components using the pool's prototype entity
void whisker_ecs_init_pool_entity_(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id, bool propagate_component_changes)
{
	// copy component data from prototype entity
	for (size_t i = 0; i < pool->component_ids_length; ++i)
	{
		whisker_sparse_set *component_array = whisker_ecs_get_component_array(pool->world, pool->component_ids[i]);
		/* whisker_ss_set(component_array, entity_id.index, whisker_ss_get(component_array, pool->prototype_entity_id.index)); */
		whisker_ecs_create_deferred_component_action_(
			pool->world,
			pool->component_ids[i],
			component_array->element_size,
			entity_id,
			whisker_ss_get(component_array, pool->prototype_entity_id.index),
			WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET,
			false
		);

		// trigger a DUMMY_ADD on this entity
		whisker_ecs_create_deferred_component_action_(
			pool->world,
			pool->component_ids[i],
			0,
			entity_id,
			NULL,
			WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DUMMY_ADD,
			propagate_component_changes
		);
	}

	// turn into managed entity
	whisker_ecs_set_entity_managed(pool->world, entity_id);
}

// de-initialise the entity and handle component actions
void whisker_ecs_deinit_pool_entity_(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id, bool propagate_component_changes)
{
	// trigger DUMMY_REMOVE actions for each component
	for (size_t i = 0; i < pool->component_ids_length; ++i)
	{
		// trigger a DUMMY_REMOVE on this entity
		whisker_ecs_create_deferred_component_action_(
			pool->world,
			pool->component_ids[i],
			0,
			entity_id,
			NULL,
			WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DUMMY_REMOVE,
			propagate_component_changes
		);
	}

	// turn into unmanaged entity
	whisker_ecs_set_entity_unmanaged(pool->world, entity_id);
}

// return an entity to the pool
void whisker_ecs_return_pool_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id)
{
	// add the entity to the pool
	whisker_ecs_add_pool_entity_(pool, entity_id);

	/* debug_log(DEBUG, ecs:pool_return, "pool %p return entity %zu (requests %zu returns %zu misses %zu)", pool, entity_id, pool->stat_total_requests, pool->stat_total_returns, pool->stat_cache_misses); */

	whisker_ecs_deinit_pool_entity_(pool, entity_id, pool->propagate_component_changes);
	
	// increase stats
	atomic_fetch_add_explicit(&pool->stat_total_returns, 1, memory_order_relaxed);
}

// create new entities topping up the pool
void whisker_ecs_realloc_pool_entities_(whisker_ecs_pool *pool)
{
	/* debug_log(DEBUG, ecs:pool_realloc, "pool %p realloc entities block size %zu cache misses %zu\n", pool, pool->realloc_block_size, pool->cache_misses); */
	whisker_ecs_create_and_return_pool_entity_(pool, (pool->stat_cache_misses <= 1) ? pool->inital_size : pool->realloc_block_size * pool->stat_cache_misses);
}

// create and add entity to the pool
void whisker_ecs_create_and_return_pool_entity_(whisker_ecs_pool *pool, size_t count)
{
	for (int i = 0; i < count; ++i)
	{
    	whisker_ecs_entity_id e = whisker_ecs_create_pool_entity_deferred_(pool);
    	whisker_ecs_init_pool_entity_(pool, e, false);
    	whisker_ecs_deinit_pool_entity_(pool, e, false);
		whisker_ecs_add_pool_entity_(pool, e);
	}
}

// add an entity to the pool (this is not the same as returning an entity)
void whisker_ecs_add_pool_entity_(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id)
{
	// grow array if required using lock
	size_t entity_idx = atomic_fetch_add(&pool->entity_pool_length, 1);

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
	pool->world->entities->entities[entity_id.index].id.version++;
}
