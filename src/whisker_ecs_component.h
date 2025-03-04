/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_component
 * @created     : Thursday Feb 13, 2025 17:59:17 CST
 */

#include "whisker_std.h"
#include "whisker_sparse_set.h"
#include "whisker_ecs_types.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_err.h"

#ifndef WHISKER_ECS_COMPONENT_H
#define WHISKER_ECS_COMPONENT_H

// components struct management
E_WHISKER_ECS_COMP whisker_ecs_c_create_components(whisker_ecs_components **components);
void whisker_ecs_c_free_components(whisker_ecs_components *components);

// component array management
E_WHISKER_ECS_COMP whisker_ecs_c_create_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size);
E_WHISKER_ECS_COMP whisker_ecs_c_grow_components_(whisker_ecs_components *components, size_t capacity);
E_WHISKER_ECS_COMP whisker_ecs_c_get_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_sparse_set **component_array);
E_WHISKER_ECS_COMP whisker_ecs_c_free_component_array(whisker_ecs_components *components, whisker_ecs_entity_id component_id);

// component management
void* whisker_ecs_c_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS_COMP whisker_ecs_c_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void* component);
bool whisker_ecs_c_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS_COMP whisker_ecs_c_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);

#endif /* WHISKER_ECS_COMPONENT_H */

