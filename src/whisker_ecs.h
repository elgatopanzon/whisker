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
void* whisker_ecs_get_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS whisker_ecs_set_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id, void* value);
E_WHISKER_ECS whisker_ecs_remove_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char* component_name, size_t component_size, whisker_ecs_entity_id entity_id);

// archetype shortcut functions
whisker_ecs_entity_id* whisker_ecs_archetype_from_named_entities(whisker_ecs_entities *entities, char* entity_names);
E_WHISKER_ECS whisker_ecs_archetype_set(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id);
E_WHISKER_ECS whisker_ecs_archetype_remove(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id, whisker_ecs_entity_id archetype_id);
E_WHISKER_ECS whisker_ecs_set_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id);
E_WHISKER_ECS whisker_ecs_remove_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_component_archetype(whisker_ecs_entities *entities, char* component_name, whisker_ecs_entity_id entity_id);

// macros
#define whisker_ecs_set(en, cm, n, t, e, v) whisker_ecs_set_component(en, cm, #n, sizeof(t), e, v)
#define whisker_ecs_get(en, cm, n, t, e) (t*) whisker_ecs_get_component(en, cm, #n, sizeof(t), e)

#define whisker_ecs_set_tag(en, n, e) whisker_ecs_set_component_archetype(en, #n, e)
#define whisker_ecs_remove_tag(en, n, e) whisker_ecs_remove_component_archetype(en, #n, e)
#define whisker_ecs_has_tag(en, n, e) whisker_ecs_has_component_archetype(en, #n, e)

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


// the following macros are created specifically to be used inside a system
// update function, requiring an instance of whisker_ecs_system_update named
// "system" and operating on the current system's entity

// create an entity (from within a system)
// #define whisker_ecs_sys_create_entity() whisker_ecs_e_create(system->entities)
// // create a named entity (from within a system)
// #define whisker_ecs_sys_create_named_entity(n) whisker_ecs_e_create_named(system->entities, #n)
// // check if system's entity is alive (from within a system)
// #define whisker_ecs_sys_is_alive() whisker_ecs_e_is_alive(system->entities, system.entity_id)
// // destroy system's entity (from within a system)
// #define whisker_ecs_sys_destroy_entity() whisker_ecs_e_destroy(system->entities, system.entity_id)
// // check if an entity is alive (from within a system)
// #define whisker_ecs_sys_is_alive_e(e) whisker_ecs_e_is_alive(system->entities, e)
// // destroy an entity (from within a system)
// #define whisker_ecs_sys_destroy_entity_e(e) whisker_ecs_e_destroy(system->entities, e)
//
//
// // component macros
//
// #define whisker_ecs_sys_get_read(t) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #t, -1, sizeof(t), system.entity_id, false)
// #define whisker_ecs_sys_get_write(t) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #t, -1, sizeof(t), system.entity_id, true)
// #define whisker_ecs_sys_get_read_alias(n, t) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #n, -1, sizeof(t), system.entity_id, false)
// #define whisker_ecs_sys_get_write_alias(n, t) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #n, -1, sizeof(t), system.entity_id, true)
//
// #define whisker_ecs_sys_get_read_e(t, e) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #t, -1, sizeof(t), e, false)
// #define whisker_ecs_sys_get_write_e(t, e) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #t, -1, sizeof(t), e, true)
// #define whisker_ecs_sys_get_read_alias_e(n, t, e) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #n, -1, sizeof(t), e, false)
// #define whisker_ecs_sys_get_write_alias_e(n, t, e) (t*)whisker_ecs_s_get_component_by_name_or_index(system.system, #n, -1, sizeof(t), e, true)

/////////////////////////////////
//  system declaration macros  //
/////////////////////////////////
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
			whisker_ecs_entity_id entity_id = system.entity_id; \
			whisker_ecs_entity_index entity_index = system.entity_id.index; \
			sys \
		}

#define WECS_SYSTEM_END }

///////////////////////////////
//  system archetype macros  //
///////////////////////////////
// these macros have a dual purpose: 
// 1. they offer an interface to define a system's archetype
// 2. sets up the system's scope making available read/write component
// it relies specificically on the idompotency of the API to declare what the
// system archetype is during the moment a get request is made
// another way of looking at it: if this was not set, the systems would never
// have an archetype and never execute
#define WECS_USES_ARCHETYPE(idx, ...) \
	if (system.system->delta_time == 0) { whisker_ecs_s_set_custom_archetype(system.system, idx, whisker_ecs_archetype_from_named_entities(system.system->entities,#__VA_ARGS__)); };
	
#define WECS_MATCHES_ARCHETYPE(idx, entity) \
	whisker_ecs_a_match(whisker_ecs_s_get_custom_archetype(system.system, idx), system.system->entities->entities[entity.index].archetype)

// the most basic type of archetype definition is READs or WRITEs
// archetype matching is done by READs, and WRITEs archetype is used by the
// system scheduler in other ways
// both of these create the read/write component in the system's scope
#define WECS_READS(type, name, idx) WECS_DECLARE(type, name, idx, false)

// note: using WRITEs does NOT add to the system's matching archetype, it is
// declared as an optional write whether or not the component exists on the
// entity or not
#define WECS_WRITES(type, name, idx) WECS_DECLARE(type, name, idx, true)

// a shortcut to setup a READs tag, and a WRITEs component within the scope
// same as WRITEs, but it DOES add to the system's matching archetype
// note: this should be used for components which exist and get updated, not
// optional writes
#define WECS_READ_WRITES(type, name, idx_read, idx_write) \
	WECS_HAS(name, idx_read) \
	WECS_DECLARE(type, name, idx_write, true)

// base macro used to declare read/write mode components in scope
#define WECS_DECLARE(type, name, idx, mode) \
	type *name = whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, sizeof(type), system.entity_id, mode);


// currently these macros create block array stores of the given component
// however, they should NOT be used for writing
// reason: writing directly to the component array won't trigger archetype
// changes on the entity
#define WECS_READS_ALL(type, name, idx) \
	WECS_HAS(name, idx) \
	WECS_DECLARE_STORE(type, name, idx, read)
#define WECS_WRITES_ALL(type, name, idx) \
	WECS_WRITES_TAG(name, idx) \
	WECS_DECLARE_STORE(type, name, idx, write)

// since the system cache for components is directly linked to the archetype
// index to declare a read store for components which don't belong to the entity
// must be delared as a write store
#define WECS_WRITES_ALL_E(type, name, idx) \
	WECS_DECLARE_STORE(type, name, idx, write)

// base macro to declare component store in scope with different mode
#define WECS_DECLARE_STORE(type, name, idx, mode) \
	whisker_block_array *name##_##mode##_store = system.system->mode##_components->components[idx];

// these macros allow specifying which tags the system is interested in
// this is accomplished by adding the tag to the system archetype during init
// the specified size is 0, since tags have no data
// note: this also allows HAS being used with normal components which are not
// read within the system, but the system still cares that the entity has it
#define WECS_HAS(name, idx) \
	if (system.system->delta_time == 0) { whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, 0, system.entity_id, false); };

// WRITES_TAG is the same as WRITES, it just sets up the write archetype
#define WECS_WRITES_TAG(name, idx) \
	if (system.system->delta_time == 0) { whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, 0, system.entity_id, true); };


///////////////////////////////////
//  system component/tag macros  //
///////////////////////////////////
// these macros are basically shortcuts to the main ECS interface using the
// state contained within the current system to wrap up those actions

// these macros are supposed to be used after setting READS_ALL/WRITES_ALL
// READ_E will get the component from the store declared by READS_ALL
// WRITE_E will get the component from the system via it's index 
// note: if READS_ALL/WRITES_ALL has not been set on the system the stores won't
// exist and the indexes will be invalid
#define WECS_GET_READ_E(name, idx, entity) \
	wbarr_get(name##_read_store, entity.index)
// note: this sets the type size to 1, because we want to trigger obtaining the
// component and since this is used after WRITE_ALL the component array is
// already initialised with the correct type size
#define WECS_GET_WRITE_E(name, idx, entity) \
	wbarr_get(name##_write_store, entity.index)

// this macro gets a non-read/write specific component for an entity using the
// main ECS interface, meaning it has to lookup the component id from name
// note: must be used ONLY in cases where the type is guranteed to have an
// existing component array, since we pass size 0
#define WECS_GET_E(name, entity) \
	whisker_ecs_get_component(system.system->entities, system.system->components, #name, 0, entity)

// uses the main ECS interface to check if an entity has an archetype (tag)
#define WECS_HAS_TAG_E(name, entity) \
	whisker_ecs_has_component_archetype(system.system->entities, #name, entity)

// shortcuts to SET values either on the current entity or any other with _E
// they do not work with the scoped components defined by the archetype, and
// instead they work using the main ECS interface
// programatically it's the same as first getting a component and writing to it
#define WECS_SET(type, name, idx, value) \
	WECS_TAG_ON(name, idx) \
	wbarr_set(name##_write_store, system.entity_id.index, value);
#define WECS_SET_E(type, name, idx, entity, value) \
	WECS_TAG_ON_E(name, idx) \
	wbarr_set(name##_write_store, entity_id.index, value);

// shortcuts to remove components from entities, which really just removes the
// archetype and leaves the data as-is in the component array
#define WECS_REMOVE(type, name, idx) \
	WECS_TAG_OFF(name, idx)
#define WECS_REMOVE_E(type, name, idx, entity, value) \
	WECS_TAG_OFF_E(name, idx)

// the tag on/off macros use the system's indexes to obtain the archetype ID to
// set the archetype using the ECS main interface
// it expects to be used in a system which WRITES_TAG in the archetype
#define WECS_TAG_ON(name, idx) \
	whisker_ecs_archetype_set(system.system->entities, system.entity_id, system.system->write_archetype[idx]);
#define WECS_TAG_OFF(name, idx) \
	whisker_ecs_archetype_remove(system.system->entities, system.entity_id, system.system->write_archetype[idx]);

// _E variants allowing setting any entities tag
#define WECS_TAG_ON_E(name, idx, entity) \
	whisker_ecs_archetype_set(system.system->entities, entity, system.system->write_archetype[idx]);
#define WECS_TAG_OFF_E(name, idx, entity) \
	whisker_ecs_archetype_remove(system.system->entities, entity, system.system->write_archetype[idx]);

#endif /* WHISKER_ECS_H */

