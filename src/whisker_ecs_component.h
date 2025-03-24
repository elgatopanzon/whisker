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

typedef struct whisker_ecs_components
{
	whisker_arr_declare(whisker_sparse_set *, components);
	whisker_sparse_set *changed_components;
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

// component management
void* whisker_ecs_c_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void whisker_ecs_c_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void* component, bool sort);
bool whisker_ecs_c_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void whisker_ecs_c_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id, bool sort);

#endif /* WHISKER_ECS_COMPONENT_H */

