/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:17 CST
 */

#include "whisker_std.h"
#include "whisker_block_array.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_entity_types.h"
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
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system*), char *system_name);
E_WHISKER_ECS whisker_ecs_update(whisker_ecs *ecs, double delta_time);

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
void *whisker_ecs_get_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);
void *whisker_ecs_set_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, size_t component_size, whisker_ecs_entity_id entity_id, void *value);
bool whisker_ecs_remove_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);

// macros
#define whisker_ecs_set(en, cm, n, t, e, v) whisker_ecs_set_component(en, cm, #n, sizeof(t), e, v)
#define whisker_ecs_get(en, cm, n, t, e) (t*) whisker_ecs_get_component(en, cm, #n, sizeof(t), e)
#define whisker_ecs_remove(en, cm, n, t, e) (t*) whisker_ecs_get_component(en, cm, #n, e)

#define whisker_ecs_set_tag(en, cm, n, e) whisker_ecs_set_component(en, cm, #n, sizeof(bool), e, &(bool){0})
#define whisker_ecs_remove_tag(en, cm, n, e) whisker_ecs_remove_component(en, cm, #n, e)
#define whisker_ecs_has(en, cm, n, e) whisker_ecs_has_component(en, cm, #n, e)

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
// #define WECS_SYSTEM(system_name, sys, ...) \
// 	void system_ ## system_name ## _fn(whisker_ecs_system_update system); \
// 	void system_ ## system_name ## _init(whisker_ecs *ecs) \
// 	{ \
// 		whisker_ecs_system_update system = { \
// 			.entity_id = 0,	\
// 		}; \
// 		system.system = whisker_ecs_register_system( \
//     		ecs, \
//     		system_ ## system_name ## _fn, \
//             #system_name, \
//     		"", \
//     		"" \
// 		); \
// 		int index = 0; \
// 		__VA_ARGS__ \
// 	} \
// 	inline void system_ ## system_name ## _fn(whisker_ecs_system_update system) \
// 		{ \
// 			int index = 0; \
// 			__VA_ARGS__ \
// 			double delta_time = system.system->delta_time; \
// 			whisker_ecs_entity_id entity_id = system.entity_id; \
// 			whisker_ecs_entity_index entity_index = system.entity_id.index; \
// 			sys \
// 		}
//
// #define WECS_SYSTEM_END }

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
// #define WECS_USES_ARCHETYPE(idx, ...) \
// 	if (system.system->delta_time == 0) { whisker_ecs_s_set_custom_archetype(system.system, idx, whisker_ecs_archetype_from_named_entities(system.system->entities,#__VA_ARGS__)); };
// 	
// #define WECS_MATCHES_ARCHETYPE(idx, entity) \
// 	whisker_ecs_a_match(whisker_ecs_s_get_custom_archetype(system.system, idx), system.system->entities->entities[entity.index].archetype)

// the most basic type of archetype definition is READs or WRITEs
// archetype matching is done by READs, and WRITEs archetype is used by the
// system scheduler in other ways
// both of these create the read/write component in the system's scope
// #define WECS_READS(type, name, idx) \
// 	WECS_HAS(name, idx) /* add name to read archetype */ \
// 	WECS_INIT_CACHE(type, name, idx, false) /* init read cache */ \
// 	WECS_DECLARE_STORE(type, name, idx, read) \
// 	WECS_DECLARE(type, name, idx, read)

// note: using WRITEs does NOT add to the system's matching archetype, it is
// declared as an optional write whether or not the component exists on the
// entity or not, and regardless of whether the write happens or not
// #define WECS_WRITES(type, name, idx) \
// 	WECS_WRITES_TAG(name, idx) \
// 	WECS_INIT_CACHE(type, name, idx, true) \
// 	WECS_DECLARE_STORE(type, name, idx, write) \
// 	WECS_DECLARE(type, name, idx, write)

// setup a cache store without updating the system's read/write archetype
// #define WECS_INIT_CACHE(type, name, idx, mode) \
// 	whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, sizeof(type), system.entity_id, mode, (system.entities == NULL), false);

// a shortcut to setup a READs tag, and a WRITEs component within the scope
// same as WRITEs, but it DOES add to the system's matching archetype
// note: this should be used for components which exist and get updated, not
// optional writes
// #define WECS_READ_WRITES(type, name, idx_read, idx_write) \
// 	WECS_INIT_CACHE(type, name, idx_read, true) \
// 	WECS_HAS(name, idx_read) \
// 	WECS_DECLARE_STORE(type, name, idx_write, write) \
// 	WECS_DECLARE(type, name, idx, write)

// base macro used to declare read/write mode components in scope accessed via a
// declared store
// #define WECS_DECLARE(type, name, idx, mode) \
// 	type *name = wss_get(name##_##mode##_store, system.entity_id.index, true);



// setup the component cache and declare a read store, but do NOT update the
// system's archetype for READ or WRITE
// #define WECS_READS_ALL(type, name, idx) \
// 	WECS_INIT_CACHE(type, name, idx, false) \
// 	WECS_DECLARE_STORE(type, name, idx, read)
// #define WECS_WRITES_ALL(type, name, idx) \
// 	WECS_INIT_CACHE(type, name, idx, true) \
// 	WECS_DECLARE_STORE(type, name, idx, write)

// base macro to declare component store in scope with different mode
// #define WECS_DECLARE_STORE(type, name, idx, mode) \
// 	whisker_sparse_set *name##_##mode##_store = system.system->components_cache->components[idx];

// these macros allow specifying which tags the system is interested in
// this is accomplished by adding the tag to the system archetype during init
// the specified size is 0, since tags have no data
// note: this also allows HAS being used with normal components which are not
// read within the system, but the system still cares that the entity has it
// #define WECS_HAS(name, idx) \
// 	if (system.system->delta_time == 0) { whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, 0, system.entity_id, false, (system.entities == NULL), true); };

// WRITES_TAG is the same as WRITES, it just sets up the write archetype
// #define WECS_WRITES_TAG(name, idx) \
// 	if (system.system->delta_time == 0) { whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, 0, system.entity_id, true, (system.entities == NULL), true); };
// READS_TAG is different, it doesn't setup the archetype
// it's an alternative to HAS
// #define WECS_READS_TAG(name, idx) \
// 	if (system.system->delta_time == 0) { whisker_ecs_s_get_component_by_name_or_index(system.system, #name, idx, 0, system.entity_id, true, (system.entities == NULL), false); };


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
// #define WECS_GET_READ_E(name, idx, entity) \
// 	wss_get(name##_read_store, entity.index, true)
// note: this sets the type size to 1, because we want to trigger obtaining the
// component and since this is used after WRITE_ALL the component array is
// already initialised with the correct type size
// #define WECS_GET_WRITE_E(name, idx, entity) \
// 	wss_get(name##_write_store, entity.index, true); WECS_TAG_ON_E(name, idx, entity)

// this macro gets a non-read/write specific component for an entity using the
// main ECS interface, meaning it has to lookup the component id from name
// note: must be used ONLY in cases where the type is guranteed to have an
// existing component array, since we pass size 0
// #define WECS_GET_E(name, entity) \
// 	whisker_ecs_get_component(system.system->entities, system.system->components, #name, 0, entity)

// uses the main ECS interface to check if an entity has an archetype (tag)
// #define WECS_HAS_TAG_E(name, idx, entity) \
// 	(whisker_ecs_a_has_id(system.system->entities->entities[entity.index].archetype, system.system->components_cache_archetypes[idx]) != -1)

// shortcuts to remove components from entities, which really just removes the
// archetype and leaves the data as-is in the component array
// #define WECS_REMOVE(type, name, idx) \
// 	WECS_TAG_OFF(name, idx)
// #define WECS_REMOVE_E(type, name, idx, entity, value) \
// 	WECS_TAG_OFF_E(name, idx)

// the tag on/off macros use the system's indexes to obtain the archetype ID to
// set the archetype using the ECS main interface
// it expects to be used in a system which WRITES_TAG in the archetype
// #define WECS_TAG_ON(name, idx) \
// 	whisker_ecs_archetype_set(system.system->entities, system.entity_id, system.system->components_cache_archetypes[idx]);
// #define WECS_TAG_OFF(name, idx) \
// 	whisker_ecs_archetype_remove(system.system->entities, system.entity_id, system.system->components_cache_archetypes[idx]);

// _E variants allowing setting any entities tag
// #define WECS_TAG_ON_E(name, idx, entity) \
// 	whisker_ecs_archetype_set(system.system->entities, entity, system.system->components_cache_archetypes[idx]);
// #define WECS_TAG_OFF_E(name, idx, entity) \
// 	whisker_ecs_archetype_remove(system.system->entities, entity, system.system->components_cache_archetypes[idx]);

#endif /* WHISKER_ECS_H */

