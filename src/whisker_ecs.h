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
	whisker_ecs_entities *entities;
	whisker_ecs_components *components;
	whisker_ecs_systems *systems;
} whisker_ecs;

E_WHISKER_ECS whisker_ecs_create(whisker_ecs **ecs);
void whisker_ecs_free(whisker_ecs *ecs);

// system functions
E_WHISKER_ECS whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system_update), char *system_name, char *read_component_archetype_names, char *write_component_archetype_names);
E_WHISKER_ECS whisker_ecs_update(whisker_ecs *ecs, double delta_time);

// entity shortcut functions
whisker_ecs_entity_id whisker_ecs_create_entity(whisker_ecs *ecs);
whisker_ecs_entity_id whisker_ecs_create_named_entity(whisker_ecs *ecs, char* name);
bool whisker_ecs_destroy_entity(whisker_ecs *ecs, whisker_ecs_entity_id entity_id);
bool whisker_ecs_is_alive(whisker_ecs *ecs, whisker_ecs_entity_id entity_id);

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

// short macros: core ecs
#define wecs_create whisker_ecs_create
#define wecs_free whisker_ecs_free

// short macros: general component
#define wecs_set whisker_ecs_set
#define wecs_get whisker_ecs_get
#define wecs_has whisker_ecs_has
#define wecs_tag_on whisker_ecs_set_tag
#define wecs_tag_off whisker_ecs_remove_tag
#define wecs_has_tag whisker_ecs_has_tag

// short macros: get read/write components
#define wecs_get_read whisker_ecs_get_read
#define wecs_get_write whisker_ecs_get_write
#define wecs_get_read_e whisker_ecs_get_read_e
#define wecs_get_write_e whisker_ecs_get_write_e
#define wecs_get_read_a whisker_ecs_get_read_a
#define wecs_get_write_a whisker_ecs_get_write_a
#define wecs_get_read_ae whisker_ecs_get_read_ae
#define wecs_get_write_ae whisker_ecs_get_write_ae

#define whisker_ecs_set(ecs, n, t, e, v) whisker_ecs_set_component(ecs, #n, sizeof(t), e, v)
#define whisker_ecs_get(ecs, n, t, e) (t*) whisker_ecs_get_component(ecs, #n, sizeof(t), e)
#define whisker_ecs_has(ecs, n, t, e) (t*) whisker_ecs_get_component(ecs, #n, sizeof(t), e)

#define whisker_ecs_set_tag(ecs, n, e) whisker_ecs_set_component_archetype(ecs, #n, e)
#define whisker_ecs_remove_tag(ecs, n, e) whisker_ecs_remove_component_archetype(ecs, #n, e)
#define whisker_ecs_has_tag(ecs, n, e) whisker_ecs_has_component_archetype(ecs, #n, e)

// the following macros are created specifically to be used inside a system
// update function, requiring an instance of whisker_ecs_system_update named
// "system" and operating on the current system's entity

// create an entity (from within a system)
#define whisker_ecs_sys_create_entity() whisker_ecs_e_create(system->entities)
// create a named entity (from within a system)
#define whisker_ecs_sys_create_named_entity(n) whisker_ecs_e_create_named(system->entities, #n)
// check if system's entity is alive (from within a system)
#define whisker_ecs_sys_is_alive() whisker_ecs_e_is_alive(system->entities, system.entity_id)
// destroy system's entity (from within a system)
#define whisker_ecs_sys_destroy_entity() whisker_ecs_e_destroy(system->entities, system.entity_id)
// check if an entity is alive (from within a system)
#define whisker_ecs_sys_is_alive_e(e) whisker_ecs_e_is_alive(system->entities, e)
// destroy an entity (from within a system)
#define whisker_ecs_sys_destroy_entity_e(e) whisker_ecs_e_destroy(system->entities, e)


// component macros

#define whisker_ecs_sys_get_read(t) (t*)whisker_ecs_s_get_read_component(system.system, #t, sizeof(t), system.entity_id)
#define whisker_ecs_sys_get_write(t) (t*)whisker_ecs_s_get_write_component(system.system, #t, sizeof(t), system.entity_id)
#define whisker_ecs_sys_get_read_alias(n, t) (t*)whisker_ecs_s_get_read_component(system.system, #n, sizeof(t), system.entity_id)
#define whisker_ecs_sys_get_write_alias(n, t) (t*)whisker_ecs_s_get_write_component(system.system, #n, sizeof(t), system.entity_id)

#define whisker_ecs_sys_get_read_e(t, e) (t*)whisker_ecs_s_get_read_component(system.system, #t, sizeof(t), e)
#define whisker_ecs_sys_get_write_e(t, e) (t*)whisker_ecs_s_get_write_component(system.system, #t, sizeof(t), e)
#define whisker_ecs_sys_get_read_alias_e(n, t, e) (t*)whisker_ecs_s_get_read_component(system.system, #n, sizeof(t), e)
#define whisker_ecs_sys_get_write_alias_e(n, t, e) (t*)whisker_ecs_s_get_write_component(system.system, #n, sizeof(t), e)

// short macros: entity
// #define wecs_create_e whisker_ecs_entity_create
// #define wecs_create_ne whisker_ecs_entity_create_named
// #define wecs_alive whisker_ecs_entity_alive
// #define wecs_destroy whisker_ecs_entity_destroy
// #define wecs_alive_e whisker_ecs_entity_alive_e
// #define wecs_destroy_e whisker_ecs_entity_destroy_e

#endif /* WHISKER_ECS_H */

