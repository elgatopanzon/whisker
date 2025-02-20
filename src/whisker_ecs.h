/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:17 CST
 */

#include "whisker_std.h"
#include "whisker_block_array.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_archetype.h"
#include "whisker_ecs_component.h"
#include "whisker_ecs_system.h"
#include "whisker_ecs_err.h"

#ifndef WHISKER_ECS_H
#define WHISKER_ECS_H

typedef struct whisker_ecs
{
	whisker_ecs_entities* entities;
	whisker_ecs_components* components;
} whisker_ecs;

E_WHISKER_ECS whisker_ecs_create(whisker_ecs **ecs);
void whisker_ecs_free(whisker_ecs *ecs);

// system functions
E_WHISKER_ECS whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(whisker_ecs_entity_id, double, struct whisker_ecs_system *), char *component_archetype_names);
E_WHISKER_ECS whisker_ecs_update(whisker_ecs *ecs, double delta_time);

// component functions
whisker_ecs_entity_id whisker_ecs_component_id(whisker_ecs *ecs, char* component_name);
whisker_block_array* whisker_ecs_get_components(whisker_ecs *ecs, char* component_name, size_t component_size);
whisker_block_array* whisker_ecs_get_component(whisker_ecs *ecs, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS whisker_ecs_set_component(whisker_ecs *ecs, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id, void* value);
E_WHISKER_ECS whisker_ecs_remove_component(whisker_ecs *ecs, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id);

// archetype shortcut functions
whisker_ecs_entity_id* whisker_ecs_archetype_from_named_entities(whisker_ecs *ecs, char* entity_names);
E_WHISKER_ECS whisker_ecs_archetype_set(whisker_ecs *ecs, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id);
E_WHISKER_ECS whisker_ecs_archetype_remove(whisker_ecs *ecs, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id);
E_WHISKER_ECS whisker_ecs_set_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS whisker_ecs_remove_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_component_archetype(whisker_ecs *ecs, char* component_name, whisker_ecs_entity_id entity_id);

// short macros and types
typedef whisker_ecs wecs;
#define wecs_create whisker_ecs_create
#define wecs_free whisker_ecs_free

// component macros
#define whisker_ecs_set(ecs, n, t, e, v) whisker_ecs_set_component(ecs, #n, sizeof(t), e, v)
#define whisker_ecs_get(ecs, n, t, e) (t*) whisker_ecs_get_component(ecs, #n, sizeof(t), e)
#define whisker_ecs_has(ecs, n, t, e) (t*) whisker_ecs_get_component(ecs, #n, sizeof(t), e)

#define whisker_ecs_set_tag(ecs, n, e) whisker_ecs_set_component_archetype(ecs, #n, e)
#define whisker_ecs_remove_tag(ecs, n, e) whisker_ecs_remove_component_archetype(ecs, #n, e)
#define whisker_ecs_has_tag(ecs, n, e) (t*) whisker_ecs_has_component_archetype(ecs, #n, e)

#endif /* WHISKER_ECS_H */

