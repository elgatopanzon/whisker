/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_component
 * @created     : Thursday Feb 13, 2025 17:59:17 CST
 */

#include "whisker_std.h"
#include "whisker_sparse_set.h"
#include "whisker_ecs_entity.h"

#ifndef WHISKER_ECS_COMPONENT_H
#define WHISKER_ECS_COMPONENT_H

#define WHISKER_ECS_COMPONENT_SET_REALLOC_BLOCK_SIZE (256 / sizeof(whisker_sparse_set *))
#define WHISKER_ECS_COMPONENT_REALLOC_BLOCK_SIZE_MULTIPLIER 1024
#define WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REALLOC_BLOCK_SIZE (16384 / sizeof(struct whisker_ecs_component_deferred_action))
#define WHISKER_ECS_COMPONENT_DEFERRED_ACTION_DATA_REALLOC_BLOCK_SIZE 16384

enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION
{ 
	WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET,
	WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE,
	WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE_ALL,
};

struct whisker_ecs_component_deferred_action
{
	whisker_ecs_entity_id component_id;
	whisker_ecs_entity_id entity_id;
	size_t data_offset;
	size_t data_size;
	enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action;
};

typedef struct whisker_ecs_components
{
	whisker_arr_declare(whisker_sparse_set *, components);
	whisker_sparse_set *changed_components;
	pthread_mutex_t grow_components_mutex;
	whisker_arr_declare(struct whisker_ecs_component_deferred_action, deferred_actions);
	whisker_arr_declare(void, deferred_actions_data);
	pthread_mutex_t deferred_actions_mutex;
} whisker_ecs_components;


// components struct management
whisker_ecs_components * whisker_ecs_c_create_components();
void whisker_ecs_c_init_components(whisker_ecs_components *components);
whisker_ecs_components *whisker_ecs_c_create_and_init_components();
void whisker_ecs_c_free_components(whisker_ecs_components *components);
void whisker_ecs_c_free_components_all(whisker_ecs_components *components);

// component array management
void whisker_ecs_c_create_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size);
void whisker_ecs_c_grow_components_(whisker_ecs_components *components, size_t capacity);
whisker_sparse_set *whisker_ecs_c_get_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id);
void whisker_ecs_c_free_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id);
void whisker_ecs_c_sort_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id);
void whisker_ecs_c_set_component_array_changed(whisker_ecs_components *components, whisker_ecs_entity_id component_id);

// component deferred actions functions
void whisker_ecs_c_create_deferred_action(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id, enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action, void *data, size_t data_size);

// component management
void* whisker_ecs_c_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void whisker_ecs_c_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void* component);
bool whisker_ecs_c_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void whisker_ecs_c_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void whisker_ecs_c_remove_all_components(whisker_ecs_components *components, whisker_ecs_entity_id entity_id);

#endif /* WHISKER_ECS_COMPONENT_H */

