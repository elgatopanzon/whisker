/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_archetype
 * @created     : Tuesday Feb 18, 2025 10:51:38 CST
 */

#include "whisker_std.h"
#include "whisker_ecs_entity_types.h"
#include "whisker_ecs_entity.h"

#ifndef WHISKER_ECS_ARCHETYPE_H
#define WHISKER_ECS_ARCHETYPE_H

// archetype functions
E_WHISKER_ECS_ARCH whisker_ecs_a_set(whisker_ecs_entity_id **archetype, whisker_ecs_entity_id archetype_id);
E_WHISKER_ECS_ARCH whisker_ecs_a_remove(whisker_ecs_entity_id **archetype, whisker_ecs_entity_id archetype_id);
E_WHISKER_ECS_ARCH whisker_ecs_a_free(whisker_ecs_entity_id *archetype);
// E_WHISKER_ECS whisker_ecs_set_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id);
// E_WHISKER_ECS whisker_ecs_remove_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id);
// bool whisker_ecs_has_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id);

// utility function
int whisker_ecs_a_has_id(whisker_ecs_entity_id *archetype, whisker_ecs_entity_id archetype_id);
bool whisker_ecs_a_match(whisker_ecs_entity_id *archetype_a, whisker_ecs_entity_id *archetype_b);
whisker_arr_whisker_ecs_entity_id* whisker_ecs_a_from_named_entities(whisker_ecs_entities *entities, char* entity_names);

#endif /* WHISKER_ECS_ARCHETYPE_H */

