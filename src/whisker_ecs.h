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
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system_update), char *system_name, char *read_component_archetype_names, char *write_component_archetype_names);
E_WHISKER_ECS whisker_ecs_update(whisker_ecs *ecs, double delta_time);

// entity shortcut functions
whisker_ecs_entity_id whisker_ecs_create_entity(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_create_named_entity(whisker_ecs_entities *entities, char* name);
bool whisker_ecs_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
bool whisker_ecs_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);

// component functions
whisker_ecs_entity_id whisker_ecs_component_id(whisker_ecs_entities *entities, char* component_name);
whisker_block_array* whisker_ecs_get_components(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size);
whisker_block_array* whisker_ecs_get_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS whisker_ecs_set_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id, void* value);
E_WHISKER_ECS whisker_ecs_remove_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id);

// archetype shortcut functions
whisker_ecs_entity_id* whisker_ecs_archetype_from_named_entities(whisker_ecs *ecs, char* entity_names);
E_WHISKER_ECS whisker_ecs_archetype_set(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id);
E_WHISKER_ECS whisker_ecs_archetype_remove(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id);
E_WHISKER_ECS whisker_ecs_set_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS whisker_ecs_remove_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id);

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

#define whisker_ecs_set(en, cm, n, t, e, v) whisker_ecs_set_component(en, cm, #n, sizeof(t), e, v)
#define whisker_ecs_get(en, cm, n, t, e) (t*) whisker_ecs_get_component(en, cm, #n, sizeof(t), e)
#define whisker_ecs_has(en, cm, n, t, e) (t*) whisker_ecs_get_component(en, cm, #n, sizeof(t), e)

#define whisker_ecs_set_tag(en, n, e) whisker_ecs_set_component_archetype(en, #n, e)
#define whisker_ecs_remove_tag(en, n, e) whisker_ecs_remove_component_archetype(en, #n, e)
#define whisker_ecs_has_tag(en, n, e) whisker_ecs_has_component_archetype(en, #n, e)

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

#define whisker_ecs_sys_get_read(t) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #t, -1, sizeof(t), system.entity_id, false)
#define whisker_ecs_sys_get_write(t) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #t, -1, sizeof(t), system.entity_id, true)
#define whisker_ecs_sys_get_read_alias(n, t) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #n, -1, sizeof(t), system.entity_id, false)
#define whisker_ecs_sys_get_write_alias(n, t) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #n, -1, sizeof(t), system.entity_id, true)

#define whisker_ecs_sys_get_read_e(t, e) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #t, -1, sizeof(t), e, false)
#define whisker_ecs_sys_get_write_e(t, e) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #t, -1, sizeof(t), e, true)
#define whisker_ecs_sys_get_read_alias_e(n, t, e) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #n, -1, sizeof(t), e, false)
#define whisker_ecs_sys_get_write_alias_e(n, t, e) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #n, -1, sizeof(t), e, true)

// short macros: entity
// #define wecs_create_e whisker_ecs_entity_create
// #define wecs_create_ne whisker_ecs_entity_create_named
// #define wecs_alive whisker_ecs_entity_alive
// #define wecs_destroy whisker_ecs_entity_destroy
// #define wecs_alive_e whisker_ecs_entity_alive_e
// #define wecs_destroy_e whisker_ecs_entity_destroy_e

// the following macro allows defining a system function and a corresponding
// system init function accepting a whisker_ecs instance
// note 1: the init function takes advantage of getting each component by index
// in order to set this index on the system's archetype up-front via
// WECS_READS() and WECS_WRITES() macros
// note 2: those macros actually declare and initialise the components variables
// for the current entity, and also the component array as a block array pointer
// note 3: it would be nice to somehow automatically set the index value but
// that would extremely complicate the macro and it's maintenance
#define WECS_SYSTEM(system_name, sys, ...) \
	void system_ ## system_name ## _fn(whisker_ecs_system_update system); \
	void system_ ## system_name ## _init(whisker_ecs *ecs) \
	{ \
		whisker_ecs_system_update system = { \
			.entity_id = 0,	\
		}; \
		system.system = whisker_ecs_register_system( \
    		ecs, \
    		system_ ## system_name ## _fn, \
            #system_name, \
    		"", \
    		"" \
		); \
		__VA_ARGS__ \
	} \
	inline void system_ ## system_name ## _fn(whisker_ecs_system_update system) \
		{ \
			__VA_ARGS__ \
			double delta_time = system.system->delta_time; \
			sys \
		}

#define WECS_SYSTEM_END }

// macros to be used as part of a system definition, since it relies on the
// "system" instance from a system function
// note: there should be no reason to use these macro manually within a function
#define WECS_READS(type, name, idx) WECS_DECLARE(type, name, idx, false)
#define WECS_WRITES(type, name, idx) WECS_DECLARE(type, name, idx, true)
// defines a read archetype and a write component declaration with their own
// indexes
#define WECS_READ_WRITES(type, name, idx_read, idx_write) \
	WECS_HAS(name, idx_read) \
	WECS_DECLARE(type, name, idx_write, true)

// note: system's delta_time is 0 during the init function
#define WECS_DECLARE(type, name, idx, mode) \
	type *name = whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, sizeof(type), system.entity_id, mode);


#define WECS_READS_ALL(type, name, idx) WECS_DECLARE_STORE(type, name, idx, read)
#define WECS_WRITES_ALL(type, name, idx) WECS_DECLARE_STORE(type, name, idx, write)
#define WECS_DECLARE_STORE(type, name, idx, mode) \
	whisker_block_array *name##_##mode##_store = system.system->mode##_components->components[idx];

#define WECS_GET_READ(type, name, entity) wbarr_get_t(name##_read_store, entity.index, type)
#define WECS_GET_WRITE(type, name, entity) wbarr_get_t(name##_write_store, entity.index, type)

#define WECS_SET_FOR(type, name, entity, value) \
	whisker_ecs_set(system.system->ecs, #name, type, entity, value);

// these macros allow specifying which tags the system is interested
// note: it relies on the system instance having a delta_time of 0 which is has
// during the system init function, not the game update call
#define WECS_HAS(name, idx) \
	if (system.system->delta_time == 0) { whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, 0, system.entity_id, false); };
#define WECS_WRITES_TAG(name, idx) \
	if (system.system->delta_time == 0) { whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, 0, system.entity_id, true); };


#endif /* WHISKER_ECS_H */

