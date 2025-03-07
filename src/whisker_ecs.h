/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:17 CST
 */

#include "whisker_std.h"
#include "whisker_block_array.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_component.h"
#include "whisker_ecs_system.h"
#include "whisker_ecs_err.h"

#ifndef WHISKER_ECS_H
#define WHISKER_ECS_H

typedef struct whisker_ecs
{
	whisker_ecs_entities *entities;
	whisker_ecs_components *components;
	whisker_ecs_systems *systems;
} whisker_ecs;

E_WHISKER_ECS whisker_ecs_create(whisker_ecs **ecs);
void whisker_ecs_free(whisker_ecs *ecs);

// system functions
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system*), char *system_name, char *process_phase_name);
E_WHISKER_ECS whisker_ecs_update(whisker_ecs *ecs, double delta_time);
whisker_ecs_entity_id whisker_ecs_register_process_phase(whisker_ecs *ecs, char *phase_name, double update_rate_sec);

// entity shortcut functions
whisker_ecs_entity_id whisker_ecs_create_entity(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_create_named_entity(whisker_ecs_entities *entities, char* name);
bool whisker_ecs_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_create_entity_deferred(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(whisker_ecs_entities *entities, char* name);
bool whisker_ecs_destroy_entity_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
bool whisker_ecs_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);

// component functions
whisker_ecs_entity_id whisker_ecs_component_id(whisker_ecs_entities *entities, char* component_name);
void *whisker_ecs_get_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);
void *whisker_ecs_set_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, size_t component_size, whisker_ecs_entity_id entity_id, void *value);
bool whisker_ecs_remove_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);

void *whisker_ecs_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void *whisker_ecs_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value);
bool whisker_ecs_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);

// macros
#define whisker_ecs_set_named(en, cm, n, t, e, v) whisker_ecs_set_named_component(en, cm, #n, sizeof(t), e, v)
#define whisker_ecs_get_named(en, cm, n, e) whisker_ecs_get_named_component(en, cm, #n, e)
#define whisker_ecs_remove_named(en, cm, n, t, e) (t*) whisker_ecs_remove_named_component(en, cm, #n, e)
#define whisker_ecs_has_named(en, cm, n, e) whisker_ecs_has_named_component(en, cm, #n, e)

#define whisker_ecs_set_named_tag(en, cm, n, e) whisker_ecs_set_named_component(en, cm, #n, sizeof(bool), e, &(bool){0})
#define whisker_ecs_remove_named_tag(en, cm, n, e) whisker_ecs_remove_named_component(en, cm, #n, e)

#define whisker_ecs_set(cm, n, t, e, v) whisker_ecs_set_component(cm, n, sizeof(t), e, v)
#define whisker_ecs_get(cm, n, e) whisker_ecs_get_component(cm, n, e)
#define whisker_ecs_remove(cm, n, t, e) (t*) whisker_ecs_remove_component(cm, n, e)

#define whisker_ecs_set_tag(cm, n, e) whisker_ecs_set_component(cm, n, sizeof(bool), e, &(bool){0})
#define whisker_ecs_remove_tag(cm, n, e) whisker_ecs_remove_component(cm, n, e)
#define whisker_ecs_has(cm, n, e) whisker_ecs_has_component(cm, n, e)

// short macros: general component
#define wecs_set_n whisker_ecs_set_named
#define wecs_get_n whisker_ecs_get_named
#define wecs_remove_n whisker_ecs_remove_named
#define wecs_has_n whisker_ecs_has_named

#define wecs_set_nt whisker_ecs_set_named_tag
#define wecs_remove_nt whisker_ecs_remove_named_tag

#define wecs_set whisker_ecs_set
#define wecs_get whisker_ecs_get
#define wecs_remove whisker_ecs_remove

#define wecs_set_t whisker_ecs_set_tag
#define wecs_remove_t whisker_ecs_remove_tag
#define wecs_has whisker_ecs_has

#endif /* WHISKER_ECS_H */

