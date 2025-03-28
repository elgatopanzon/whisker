/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_pool
 * @created     : Thursday Mar 27, 2025 17:23:20 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_sparse_set.h"
#include "whisker_memory.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_component.h"

#ifndef WHISKER_ECS_POOL_H
#define WHISKER_ECS_POOL_H

#define WHISKER_ECS_POOL_REALLOC_BLOCK_SIZE (16384 / sizeof(whisker_ecs_entity_id))

typedef struct whisker_ecs_pool
{
	whisker_ecs_entity_id prototype_entity_id;
	whisker_arr_declare(whisker_ecs_entity_id, component_ids);
	whisker_sparse_set *component_ids_set;
	whisker_arr_declare(whisker_ecs_entity_id, entity_pool);
	pthread_mutex_t entity_pool_mutex;
	size_t inital_size;
	size_t realloc_block_size;
	_Atomic size_t cache_misses;

	whisker_ecs_components *components;
	whisker_ecs_entities *entities;
} whisker_ecs_pool;

#define whisker_ecs_p_set_prototype_component(p, c, s, v) \
	whisker_ecs_p_set_prototype_component_f(p, c, s, v)
#define whisker_ecs_p_set_prototype_named_component(p, c, s, v) \
	whisker_ecs_p_set_prototype_named_component_f(p, #c, sizeof(s), v)

#define whisker_ecs_p_set_prototype_tag(p, c) \
	whisker_ecs_p_set_prototype_component_f(p, c, sizeof(bool), &(bool){0})
#define whisker_ecs_p_set_prototype_named_tag(p, c) \
	whisker_ecs_p_set_prototype_named_component_f(p, #c, sizeof(bool), &(bool){0})

whisker_ecs_pool *whisker_ecs_p_create();
whisker_ecs_pool *whisker_ecs_p_create_and_init(whisker_ecs_components *components, whisker_ecs_entities *entities, size_t count, size_t realloc_count);
void whisker_ecs_p_init(whisker_ecs_pool *pool, whisker_ecs_components *components, whisker_ecs_entities *entities, size_t count, size_t realloc_count);
void whisker_ecs_p_free(whisker_ecs_pool *pool);
void whisker_ecs_p_free_all(whisker_ecs_pool *pool);
void whisker_ecs_p_set_prototype_component_f(whisker_ecs_pool *pool, whisker_ecs_entity_id component_id, size_t component_size, void *prototype_value);
void whisker_ecs_p_set_prototype_named_component_f(whisker_ecs_pool *pool, char* component_name, size_t component_size, void *prototype_value);
void whisker_ecs_p_set_prototype_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id prototype_entity_id);
whisker_ecs_entity_id whisker_ecs_p_request_entity(whisker_ecs_pool *pool);
whisker_ecs_entity_id whisker_ecs_p_create_entity_deferred(whisker_ecs_pool *pool);
void whisker_ecs_p_init_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id);
void whisker_ecs_p_return_entity(whisker_ecs_pool *pool, whisker_ecs_entity_id entity_id);
void whisker_ecs_p_realloc_entities(whisker_ecs_pool *pool);
void whisker_ecs_p_create_and_return(whisker_ecs_pool *pool, size_t count);

#endif /* WHISKER_ECS_POOL_H */

