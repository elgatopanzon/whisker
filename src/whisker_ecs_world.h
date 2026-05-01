/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_world
 * @created     : Wednesday Mar 04, 2026 20:27:06 CST
 * @description : ECS world main API
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_arena.h"
#include "whisker_string_table.h"
#include "whisker_entity_registry.h"
#include "whisker_component_registry.h"
#include "whisker_system_registry.h"
#include "whisker_scheduler.h"
#include "whisker_hook_registry.h"
#include "whisker_command_buffer.h"
#include "whisker_query_registry.h"
#include "whisker_singleton_registry.h"

#ifndef WHISKER_ECS_WORLD_H
#define WHISKER_ECS_WORLD_H

/***************
*  macro API  *
***************/
// short macros with implicit world in scope

#define w_return(entity) w_ecs_return_entity(world, entity);
#define w_request() w_ecs_request_entity(world);
#define w_request_named(name) w_ecs_request_entity_with_name(world, name);

// component short forms for type-safe components only
#define w_set(entity, name, ptr) ((void)sizeof(name), name##_set(world, entity, ptr))
#define w_set_default(entity, name) ((void)sizeof(name), name##_set_default(world, entity))
#define w_set_value(entity, name, ...) ((void)sizeof(name), name##_set_value(world, entity, ((name){__VA_ARGS__})))
#define w_set_tag(entity, name, state) ((void)sizeof(name), name##_set_tag_state(world, entity, state))
#define w_exists(entity, name) ((void)sizeof(name), name##_exists(world, entity))
#define w_remove(entity, name) ((void)sizeof(name), name##_remove(world, entity))
#define w_get(entity, name) ((void)sizeof(name), name##_get(world, entity))

#define w_set_g(entity, name, gname, ptr) ((void)sizeof(name), name##_set_generic(world, entity, gname, ptr))
#define w_set_default_g(entity, name, gname) ((void)sizeof(name), name##_set_default_generic(world, entity, gname))
#define w_set_value_g(entity, name, gname, ...) ((void)sizeof(name), name##_set_value_generic(world, entity, gname, ((name){__VA_ARGS__})))
#define w_set_tag_g(entity, name, gname, state) ((void)sizeof(name), name##_set_tag_state_generic(world, entity, gname, state))
#define w_exists_g(entity, name, gname) ((void)sizeof(name), name##_exists_generic(world, entity, gname))
#define w_remove_g(entity, name, gname) ((void)sizeof(name), name##_remove_generic(world, entity, gname))
#define w_get_g(entity, name, gname) ((void)sizeof(name), name##_get_generic(world, entity, gname))

#define w_set_id(entity, name, id, ptr) ((void)sizeof(name), w_ecs_set_component_(world, name##_type_id_, id, entity, (name*)ptr, sizeof(name)))
#define w_get_id(entity, name, id) ((void)sizeof(name), w_ecs_get_component_(world, name##_type_id_, id, entity))
#define w_has_id(entity, name, id) ((void)sizeof(name), w_ecs_has_component_(world, name##_type_id_, id, entity))
#define w_remove_id(entity, name, id) ((void)sizeof(name), w_ecs_remove_component_(world, name##_type_id_, id, entity))

#define w_name(name) ((void)sizeof(name), name##_name_)
#define w_gname(name, gname) ((void)sizeof(name), #name##_gname)
#define w_id(name) ((void)sizeof(name), name##_get_id(world))
#define w_gid(name, gname) ((void)sizeof(name), name##_get_generic_id(world, gname))


enum W_COMPONENT_ACTION
{
	W_COMPONENT_ACTION_SET = 0,
	W_COMPONENT_ACTION_REMOVE = 1,
};

struct w_component_action_payload
{
	enum W_COMPONENT_ACTION action;
	uint type_id;
	w_entity_id type_entity_id;
	w_entity_id entity_id;
	size_t data_size;
};

enum W_WORLD_UPDATE_RESULT
{
	W_WORLD_UPDATE_RESULT_INIT = 0,
	W_WORLD_UPDATE_RESULT_CONTINUE = 1,
	W_WORLD_UPDATE_RESULT_RESTART = 2,
	W_WORLD_UPDATE_RESULT_SHUTDOWN = 3,
};

enum W_WORLD_HOOK_TYPE
{
	W_WORLD_HOOK_TYPE_UPDATE = 0,
	W_WORLD_HOOK_TYPE_COMPONENT_SET,
	W_WORLD_HOOK_TYPE_COMPONENT_REMOVE,
	W_WORLD_HOOK_TYPE_COMPONENT_PRE_SET,
	W_WORLD_HOOK_TYPE_COMPONENT_PRE_REMOVE,
	W_WORLD_HOOK_TYPE_COMPONENT_ID_PRE_SET,
	W_WORLD_HOOK_TYPE_COMPONENT_ID_PRE_REMOVE,
	W_WORLD_HOOK_TYPE_ENTITY_CREATE,
	W_WORLD_HOOK_TYPE_ENTITY_DESTROY,
	W_WORLD_HOOK_TYPE_COUNT,
};

enum W_WORLD_HOOK
{
	W_WORLD_HOOK_UPDATE_BEGIN,
	W_WORLD_HOOK_UPDATE_END,
	W_WORLD_HOOK_UPDATE_TIMESTEP_BEGIN,
	W_WORLD_HOOK_UPDATE_TIMESTEP_END,
	W_WORLD_HOOK_UPDATE_PHASE_BEGIN,
	W_WORLD_HOOK_UPDATE_PHASE_END,
	W_WORLD_HOOK_STARTUP,
	W_WORLD_HOOK_RESTART,
	W_WORLD_HOOK_SHUTDOWN,
	W_WORLD_HOOK_ENTITY_CREATE,
	W_WORLD_HOOK_ENTITY_DESTROY,
};

struct w_ecs_world 
{
	// general memory
	struct w_arena *arena;
	struct w_string_table *string_table;

	// core ECS data
	struct w_entity_registry entities;
	struct w_component_registry components;
	struct w_system_registry systems;

	// scheduling
	struct w_scheduler scheduler;
	w_array_declare(struct w_scheduler_job, scheduler_jobs);
	bool scheduler_jobs_dirty;

	// hooks
	struct w_hook_registry hooks[W_WORLD_HOOK_TYPE_COUNT];

	// buffering
	struct w_command_buffer command_buffer;
	bool buffering_enabled;

	// queries
	struct w_query_registry queries;

	// singletons
	struct w_singleton_registry singletons;

	// module resources (indexed by module ID for fast access)
	w_array_declare(void *, module_resources);

	enum W_WORLD_UPDATE_RESULT update_result;
};

// init an ECS world with self-managed core ECS and scheduler
void w_ecs_world_init(struct w_ecs_world *world, struct w_string_table *string_table, struct w_arena *arena);

// free an ECS world's core ECS data and scheduler
void w_ecs_world_free(struct w_ecs_world *world);



/**************
*  core API  *
**************/

// update the world with 1 tick
enum W_WORLD_UPDATE_RESULT w_ecs_update(struct w_ecs_world *world);

/****************
*  entity API  *
****************/

// request a new entity ID
w_entity_id w_ecs_request_entity(struct w_ecs_world *world);

// request a new entity ID with a persistent name
// will return existing entity ID if entity exists with this name
// (note: name is not set until sync point)
w_entity_id w_ecs_request_entity_with_name(struct w_ecs_world *world, char *name);

// return an entity ID for reuse
void w_ecs_return_entity(struct w_ecs_world *world, w_entity_id entity);

// set an entity name, clears the previous name
void w_ecs_set_entity_name(struct w_ecs_world *world, w_entity_id entity, char *name);

// clear an entities name if it has one
// (note: this converts it from a persistent to anonymous entity)
void w_ecs_clear_entity_name(struct w_ecs_world *world, w_entity_id entity);

// get an entities name, if it has one
char *w_ecs_get_entity_name(struct w_ecs_world *world, w_entity_id entity);

// get the entity ID for the given name, if it exists
w_entity_id w_ecs_get_entity_by_name(struct w_ecs_world *world, char *name);

#define w_ecs_is_valid_entity(e) w_entity_is_valid(e)
#define w_ecs_alive_entity_count(w) w_entity_alive_count(w->entities)
#define w_ecs_recycled_entity_count(w) w_entity_recycled_count(w->entities)
#define w_ecs_total_entity_count(w) w_entity_total_count(w->entities)


/*******************
*  component API  *
*******************/

#define W_ECS_PASTE(a, b) a##b

// component typedef
#define w_ecs_define_component_typedef(type, name, suffix) \
	typedef type name; \
	__attribute__((unused)) static char *name##_name_ = #name; \
	__attribute__((unused)) static uint64_t name##suffix##_type_id_ = W_COMPONENT_TYPE##_##type; \
	__attribute__((unused)) static uint64_t name##suffix##_type_size_ = sizeof(type); \
	__attribute__((unused)) static w_entity_id name##suffix##_component_id_ = W_ENTITY_INVALID; \

#define w_ecs_component_name(name) ((void)sizeof(name), name##_name_)

#define W_ECS_SET_COMP_ID_CACHE(name) if (W_ECS_PASTE(name, _component_id_) == W_ENTITY_INVALID) { W_ECS_PASTE(name, _component_id_) = w_ecs_get_component_by_name(world, #name); }
#define W_ECS_COMP_ID(name) W_ECS_PASTE(name, _component_id_)

#define w_ecs_declare_shared_component_functions(type, name, ...) \
	__attribute__((unused)) static inline w_entity_id name##_get_id(struct w_ecs_world *world) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
		return name##_component_id_; \
	} \
	__attribute__((unused)) static inline w_entity_id name##_get_generic_id(struct w_ecs_world *world, const char *generic_name) { \
		char _name[strlen(name##_name_) + strlen(generic_name) + 2]; \
		snprintf(_name, sizeof(_name), "%s_%s", generic_name, name##_name_); \
		return w_ecs_get_component_by_name(world, _name); \
	} \

#define w_ecs_define_component_impl_(type, name, ...) \
	w_ecs_declare_shared_component_functions(type, name); \
	__attribute__((unused)) static inline name name##_default() { \
    	return (type){__VA_ARGS__}; \
	} \
	__attribute__((unused)) static inline void name##_set_default(struct w_ecs_world *world, w_entity_id entity) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
    	w_ecs_set_component_(world, W_COMPONENT_TYPE_##type, name##_component_id_, entity, (type*)&(type){__VA_ARGS__}, sizeof(type)); \
	} \
	__attribute__((unused)) static inline void name##_set_value(struct w_ecs_world *world, w_entity_id entity, type value) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
    	w_ecs_set_component_(world, W_COMPONENT_TYPE_##type, name##_component_id_, entity, (type*)&value, sizeof(type)); \
	} \
	__attribute__((unused)) static inline void name##_set(struct w_ecs_world *world, w_entity_id entity, type *value) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
    	w_ecs_set_component_(world, W_COMPONENT_TYPE_##type, name##_component_id_, entity, (type*)value, sizeof(type)); \
	} \
	__attribute__((unused)) static inline bool name##_exists(struct w_ecs_world *world, w_entity_id entity) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
    	return w_ecs_has_component_(world, name##_component_id_, entity); \
	} \
	__attribute__((unused)) static inline type *name##_get(struct w_ecs_world *world, w_entity_id entity) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
    	return w_ecs_get_component_(world, name##_component_id_, entity); \
	} \
	__attribute__((unused)) static inline void name##_remove(struct w_ecs_world *world, w_entity_id entity) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
    	w_ecs_remove_component_(world, name##_component_id_, entity); \
	} \
	\
	__attribute__((unused)) static inline void name##_set_generic_default(struct w_ecs_world *world, w_entity_id entity, const char *generic_name) { \
    	w_ecs_set_component_(world, W_COMPONENT_TYPE_##type, name##_get_generic_id(world, generic_name), entity, (type*)&(type){__VA_ARGS__}, sizeof(type)); \
	} \
	__attribute__((unused)) static inline void name##_set_generic_value(struct w_ecs_world *world, w_entity_id entity, const char *generic_name, type value) { \
    	w_ecs_set_component_(world, W_COMPONENT_TYPE_##type, name##_get_generic_id(world, generic_name), entity, (type*)&value, sizeof(type)); \
	} \
	__attribute__((unused)) static inline void name##_set_generic(struct w_ecs_world *world, w_entity_id entity, const char *generic_name, type *value) { \
    	w_ecs_set_component_(world, W_COMPONENT_TYPE_##type, name##_get_generic_id(world, generic_name), entity, (type*)value, sizeof(type)); \
	} \
	__attribute__((unused)) static inline bool name##_exists_generic(struct w_ecs_world *world, w_entity_id entity, const char *generic_name) { \
    	return w_ecs_has_component_(world, name##_get_generic_id(world, generic_name), entity); \
	} \
	__attribute__((unused)) static inline type *name##_get_generic(struct w_ecs_world *world, w_entity_id entity, const char *generic_name) { \
    	return w_ecs_get_component_(world, name##_get_generic_id(world, generic_name), entity); \
	} \
	__attribute__((unused)) static inline void name##_remove_generic(struct w_ecs_world *world, w_entity_id entity, const char *generic_name) { \
    	w_ecs_remove_component_(world, name##_get_generic_id(world, generic_name), entity); \
	} \

#define w_ecs_define_tag_impl_(name) \
	w_ecs_declare_shared_component_functions(bool, name); \
	__attribute__((unused)) static inline bool name##_tag_exists(struct w_ecs_world *world, w_entity_id entity) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
    	return w_ecs_has_component_(world, name##_component_id_, entity); \
	} \
	__attribute__((unused)) static inline void name##_set_tag_state(struct w_ecs_world *world, w_entity_id entity, bool state) { \
		W_ECS_SET_COMP_ID_CACHE(name); \
		if (state && !name##_tag_exists(world, entity)) { \
    		w_ecs_set_component_(world, W_COMPONENT_TYPE_bool, name##_component_id_, entity, (bool*)&(bool){false}, sizeof(bool)); \
    	} \
    	else if (!state) \
		{ \
    		w_ecs_remove_component_(world, name##_component_id_, entity); \
		} \
	} \
	\
	__attribute__((unused)) static inline bool name##_tag_exists_generic(struct w_ecs_world *world, w_entity_id entity, const char *generic_name) { \
    	return w_ecs_has_component_(world, name##_get_generic_id(world, generic_name), entity); \
	} \
	__attribute__((unused)) static inline void name##_set_tag_state_generic(struct w_ecs_world *world, w_entity_id entity, const char *generic_name, bool state) { \
		if (state && !name##_tag_exists_generic(world, entity, generic_name)) { \
    		w_ecs_set_component_(world, W_COMPONENT_TYPE_bool, name##_get_generic_id(world, generic_name), entity, (bool*)&(bool){false}, sizeof(bool)); \
    	} \
    	else if (!state) \
		{ \
    		w_ecs_remove_component_(world, name##_get_generic_id(world, generic_name), entity); \
		} \
	} \

#define w_ecs_define_component(type, name, ...) \
	w_ecs_define_component_typedef(type, name, ); \
	w_ecs_define_component_impl_(type, name, __VA_ARGS__) \

#define w_ecs_define_tag(name) \
	w_ecs_define_component_typedef(bool, name, ); \
	w_ecs_define_tag_impl_(name) \


// use the macros for set/get/remove/has
// entity ID + component ID (base macros)
#define w_ecs_set_ex(w, t, tt, te, e, d) w_ecs_set_component_(w, tt##_##t, te, e, (t*)d, sizeof(t));
#define w_ecs_set(w, t, te, e, d) w_ecs_set_component_(w, W_COMPONENT_TYPE##_##t, te, e, (t*)d, sizeof(t));

#define w_ecs_get(w, t, te, e) (t *)w_ecs_get_component_(w, te, e);

#define w_ecs_remove(w, te, e) w_ecs_remove_component_(w, te, e);

#define w_ecs_has(w, te, e) w_ecs_has_component_(w, te, e)

// entity ID + string component name (_str suffix)
#define w_ecs_set_str_ex(w, t, tt, n, e, d) w_ecs_set_component_(w, tt##_##t, w_ecs_get_component_by_name(w, n), e, (t*)d, sizeof(t));
#define w_ecs_set_str(w, t, n, e, d) w_ecs_set_component_(w, W_COMPONENT_TYPE##_##t, w_ecs_get_component_by_name(w, n), e, (t*)d, sizeof(t));

#define w_ecs_get_str(w, t, n, e) (t *)w_ecs_get_component_(w, w_ecs_get_component_by_name(w, n), e);

#define w_ecs_remove_str(w, n, e) w_ecs_remove_component_(w, w_ecs_get_component_by_name(w, n), e);

#define w_ecs_has_str(w, n, e) w_ecs_has_component_(w, w_ecs_get_component_by_name(w, n), e)

// string entity name + component ID (_entity_str suffix)
#define w_ecs_set_entity_str_ex(w, t, tt, te, en, d) w_ecs_set_component_(w, tt##_##t, te, w_ecs_get_entity_by_name(w, en), (t*)d, sizeof(t));
#define w_ecs_set_entity_str(w, t, te, en, d) w_ecs_set_component_(w, W_COMPONENT_TYPE##_##t, te, w_ecs_get_entity_by_name(w, en), (t*)d, sizeof(t));

#define w_ecs_get_entity_str(w, t, te, en) (t *)w_ecs_get_component_(w, te, w_ecs_get_entity_by_name(w, en));

#define w_ecs_remove_entity_str(w, te, en) w_ecs_remove_component_(w, te, w_ecs_get_entity_by_name(w, en));

#define w_ecs_has_entity_str(w, te, en) w_ecs_has_component_(w, te, w_ecs_get_entity_by_name(w, en))

// string entity name + string component name (_both_str suffix)
#define w_ecs_set_both_str_ex(w, t, tt, n, en, d) w_ecs_set_component_(w, tt##_##t, w_ecs_get_component_by_name(w, n), w_ecs_get_entity_by_name(w, en), (t*)d, sizeof(t));
#define w_ecs_set_both_str(w, t, n, en, d) w_ecs_set_component_(w, W_COMPONENT_TYPE##_##t, w_ecs_get_component_by_name(w, n), w_ecs_get_entity_by_name(w, en), (t*)d, sizeof(t));

#define w_ecs_get_both_str(w, t, n, en) (t *)w_ecs_get_component_(w, w_ecs_get_component_by_name(w, n), w_ecs_get_entity_by_name(w, en));

#define w_ecs_remove_both_str(w, n, en) w_ecs_remove_component_(w, w_ecs_get_component_by_name(w, n), w_ecs_get_entity_by_name(w, en));

#define w_ecs_has_both_str(w, n, en) w_ecs_has_component_(w, w_ecs_get_component_by_name(w, n), w_ecs_get_entity_by_name(w, en))

// unsafe variants (skip bounds checks, caller must ensure validity)
#define w_ecs_set_unsafe_ex(w, t, tt, te, e, d) w_ecs_unsafe_set_component_(w, tt##_##t, te, e, (t *)d, sizeof(t));
#define w_ecs_set_unsafe(w, t, te, e, d) w_ecs_unsafe_set_component_(w, W_COMPONENT_TYPE##_##t, te, e, (t *)d, sizeof(t));

#define w_ecs_get_unsafe(w, t, te, e) (t *)w_ecs_unsafe_get_component_(w, te, e);

#define w_ecs_has_unsafe(w, te, e) w_ecs_unsafe_has_component_(w, te, e)

// tags wrapped as uint8_t (entity ID + component ID)
#define w_ecs_set_tag(w, te, e) \
    w_ecs_set(w, uint8_t, te, e, &(uint8_t){0})
#define w_ecs_has_tag(w, te, e) w_ecs_has(w, te, e)
#define w_ecs_remove_tag(w, te, e) w_ecs_remove(w, te, e)

// tags with string component name (entity ID + string component name)
#define w_ecs_set_tag_str(w, n, e) \
    w_ecs_set(w, uint8_t, w_ecs_get_component_by_name(w, n), e, &(uint8_t){0})
#define w_ecs_has_tag_str(w, n, e) w_ecs_has(w, w_ecs_get_component_by_name(w, n), e)
#define w_ecs_remove_tag_str(w, n, e) w_ecs_remove(w, w_ecs_get_component_by_name(w, n), e)

// tags with string entity name (string entity name + component ID)
#define w_ecs_set_tag_entity_str(w, te, en) \
    w_ecs_set_entity_str(w, uint8_t, te, en, &(uint8_t){0})
#define w_ecs_has_tag_entity_str(w, te, en) w_ecs_has_entity_str(w, te, en)
#define w_ecs_remove_tag_entity_str(w, te, en) w_ecs_remove_entity_str(w, te, en)

// tags with both strings (string entity name + string component name)
#define w_ecs_set_tag_both_str(w, n, en) \
    w_ecs_set_both_str(w, uint8_t, n, en, &(uint8_t){0})
#define w_ecs_has_tag_both_str(w, n, en) w_ecs_has_both_str(w, n, en)
#define w_ecs_remove_tag_both_str(w, n, en) w_ecs_remove_both_str(w, n, en)


// set a component on an entity
// (note: this is not thread-safe, it will create the component type)
void *w_ecs_set_component_(struct w_ecs_world *world, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size);

// get the component data on an entity, if it exists
void *w_ecs_get_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id);

// remove a component from an entity
void w_ecs_remove_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id);

// check if an entity has a component
bool w_ecs_has_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id);

// remove all components from an entity, firing remove hooks for each
void w_ecs_remove_all_components_(struct w_ecs_world *world, w_entity_id entity_id);

// get component ID by name
// (note: unsafe, it will create the type and name)
w_entity_id w_ecs_get_component_by_name(struct w_ecs_world *world, char *name);

// get name of a component, if the component exists
char *w_ecs_get_component_name(struct w_ecs_world *world, w_entity_id type_entity_id);


// unsafe: set a component on an entity, skips safe checks
void *w_ecs_unsafe_set_component_(struct w_ecs_world *world, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size);
// unsafe: get a component from an entity, skips safe checks
void *w_ecs_unsafe_get_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id);
// unsafe: check if component has an entity, skips safe checks
bool w_ecs_unsafe_has_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id);

// get the component entry for the component ID, if it exists
struct w_component_entry *w_ecs_get_component_entry(struct w_ecs_world *world, w_entity_id type_entity_id);


/****************
*  system API  *
****************/

#define w_ecs_declare_system_register_fn(name, phase) \
	static inline void name##_register(struct w_ecs_world *world) { \
		struct w_system sys = { \
			.phase_id = phase, \
			.enabled = true, \
			.update = name, \
		}; \
		w_ecs_register_system(world, #name, &sys); \
	} \

#define w_ecs_system(name, phase, query, work) \
	static inline void name(void *ctx, double delta_time) { \
		struct w_ecs_world *world = ctx; \
		(void)delta_time; \
		w_query_for_each(world, query, { \
			work; \
		}); \
	} \
	w_ecs_declare_system_register_fn(name, phase) \

#define w_ecs_simple_system(name, phase, work) \
	static inline void name(void *ctx, double delta_time) { \
		struct w_ecs_world *world = ctx; \
		(void)delta_time; \
		work; \
	} \
	w_ecs_declare_system_register_fn(name, phase) \

// register a system with the ECS scheduler
size_t w_ecs_register_system(struct w_ecs_world *world, char *name, struct w_system *system);
size_t w_ecs_set_system_state(struct w_ecs_world *world, size_t system_id, bool system_state);
struct w_system *w_ecs_get_system_entry(struct w_ecs_world *world, size_t system_id);

// get system ID by name
#define w_ecs_get_system_id_by_name(w, name) w_system_get_id_by_name(&(w)->systems, name)

// get system entry by name
#define w_ecs_get_system_entry_str(w, name) w_system_get_system_entry_str(&(w)->systems, name)

// set system state by name
#define w_ecs_set_system_state_str(w, name, state) \
	do { \
		size_t *_id = w_system_get_id_by_name(&(w)->systems, name); \
		if (_id) w_ecs_set_system_state(w, *_id, state); \
	} while (0)

// register a scheduler phase (auto-assigns sequential ID)
size_t w_ecs_register_system_phase(struct w_ecs_world *world, struct w_scheduler_phase *phase);

// register a scheduler phase at a specific ID (sparse/explicit)
size_t w_ecs_register_system_phase_at(struct w_ecs_world *world, struct w_scheduler_phase *phase, size_t id);

// get a scheduler phase by ID
struct w_scheduler_phase *w_ecs_get_system_phase(struct w_ecs_world *world, size_t phase_id);

// set enabled state of a scheduler phase
void w_ecs_set_system_phase_state(struct w_ecs_world *world, size_t phase_id, bool state);

// set a phase to run before another phase
void w_ecs_set_system_phase_runs_before(struct w_ecs_world *world, size_t phase_id, size_t runs_before_phase_id);

// set a phase to run after another phase
void w_ecs_set_system_phase_runs_after(struct w_ecs_world *world, size_t phase_id, size_t runs_after_phase_id);

// set a list of phases as a chain
void w_ecs_set_phase_chain_(struct w_ecs_world* world, uint* phases, size_t count);
#define w_ecs_set_phase_chain(world, ...) do { \
    uint _phases[] = {__VA_ARGS__}; \
    w_ecs_set_phase_chain_(world, _phases, sizeof(_phases)/sizeof(_phases[0])); \
} while(0)

// reset all scheduler phases
void w_ecs_reset_system_phases(struct w_ecs_world *world);

// register a scheduler time step (auto-assigns sequential ID)
size_t w_ecs_register_system_time_step(struct w_ecs_world *world, struct w_scheduler_time_step *time_step);

// register a scheduler time step at a specific ID (sparse/explicit)
size_t w_ecs_register_system_time_step_at(struct w_ecs_world *world, struct w_scheduler_time_step *time_step, size_t id);

// get a scheduler time step by ID
struct w_scheduler_time_step *w_ecs_get_system_time_step(struct w_ecs_world *world, size_t time_step_id);

// set enabled state of a scheduler time step
void w_ecs_set_system_time_step_state(struct w_ecs_world *world, size_t time_step_id, bool state);

// set a time step to run before another time step
void w_ecs_set_system_time_step_runs_before(struct w_ecs_world *world, size_t time_step_id, size_t runs_before_time_step_id);

// set a time step to run after another time step
void w_ecs_set_system_time_step_runs_after(struct w_ecs_world *world, size_t time_step_id, size_t runs_after_time_step_id);

// reset all scheduler time steps
void w_ecs_reset_system_time_steps(struct w_ecs_world *world);

// scheduler: run all systems registered to a phase directly (bypasses scheduler)
void w_ecs_run_phase_systems(struct w_ecs_world *world, size_t phase_id);

/***********************
*  module resources   *
***********************/

// initial capacity for module_resources array
#define W_MODULE_RESOURCES_INITIAL_CAPACITY 8

// set a module resource pointer by ID (auto-grows array if needed)
static inline void w_ecs_set_module_resource(struct w_ecs_world *world, size_t id, void *ptr)
{
	size_t needed = id + 1;
	size_t current_capacity = world->module_resources_size / sizeof(void *);
	if (needed > current_capacity)
	{
		size_t old_capacity = current_capacity;
		w_array_ensure_alloc_block_size(world->module_resources, needed, W_MODULE_RESOURCES_INITIAL_CAPACITY);
		size_t new_capacity = world->module_resources_size / sizeof(void *);
		for (size_t i = old_capacity; i < new_capacity; i++)
			world->module_resources[i] = NULL;
	}
	world->module_resources[id] = ptr;
	if (needed > world->module_resources_length)
		world->module_resources_length = needed;
}

// get a module resource pointer by ID, returns NULL if out of bounds
static inline void *w_ecs_get_module_resource(struct w_ecs_world *world, size_t id)
{
	if (id >= world->module_resources_length) return NULL;
	return world->module_resources[id];
}

// clear a module resource slot (set to NULL)
static inline void w_ecs_clear_module_resource(struct w_ecs_world *world, size_t id)
{
	if (id < world->module_resources_length)
		world->module_resources[id] = NULL;
}


/*******************
*  singleton API  *
*******************/

// set a singleton pointer by name
#define w_ecs_singleton_set(w, name, ptr) w_singleton_registry_set(&(w)->singletons, name, ptr)

// get a singleton pointer by name, returns NULL if not found
#define w_ecs_singleton_get(w, name) w_singleton_registry_get(&(w)->singletons, name)

// check if a singleton exists by name
#define w_ecs_singleton_has(w, name) w_singleton_registry_has(&(w)->singletons, name)

// remove a singleton by name, returns true if it existed
#define w_ecs_singleton_remove(w, name) w_singleton_registry_remove(&(w)->singletons, name)


/*****************
*  queries API  *
*****************/

// parse and return query struct
struct w_query *w_ecs_get_query(struct w_ecs_world *world, char *query);


/***********
*  hooks  *
***********/
static inline void w_ecs_update_hook_flush_command_buffer_(void *world, void *action);

// register a hook to fire during world update lifecycle (returns hook ID)
size_t w_ecs_register_update_hook(struct w_ecs_world *world, uint update_type, w_hook_fn hook_fn);
// unregister a world update lifecycle hook by type and hook ID
void w_ecs_unregister_update_hook(struct w_ecs_world *world, uint update_type, size_t hook_id);

// register a hook to fire on first update (returns hook ID)
size_t w_ecs_register_startup_hook(struct w_ecs_world *world, w_hook_fn hook_fn);
// unregister a startup hook by ID
void w_ecs_unregister_startup_hook(struct w_ecs_world *world, size_t hook_id);

// register a hook to fire when update_result is RESTART (returns hook ID)
size_t w_ecs_register_restart_hook(struct w_ecs_world *world, w_hook_fn hook_fn);
// unregister a restart hook by ID
void w_ecs_unregister_restart_hook(struct w_ecs_world *world, size_t hook_id);

// register a hook to fire when update_result is SHUTDOWN (returns hook ID)
size_t w_ecs_register_shutdown_hook(struct w_ecs_world *world, w_hook_fn hook_fn);
// unregister a shutdown hook by ID
void w_ecs_unregister_shutdown_hook(struct w_ecs_world *world, size_t hook_id);

// register a hook to fire when a component of the given type is set (returns hook ID)
size_t w_ecs_register_component_set_hook(struct w_ecs_world *world, uint type_id, w_hook_fn hook_fn);
// unregister a component set hook by type and hook ID
void w_ecs_unregister_component_set_hook(struct w_ecs_world *world, uint type_id, size_t hook_id);

// register a hook to fire when a component of the given type is removed (returns hook ID)
size_t w_ecs_register_component_remove_hook(struct w_ecs_world *world, uint type_id, w_hook_fn hook_fn);
// unregister a component remove hook by type and hook ID
void w_ecs_unregister_component_remove_hook(struct w_ecs_world *world, uint type_id, size_t hook_id);

// register a global hook to fire before any component is set (returns hook ID)
size_t w_ecs_register_component_pre_set_hook(struct w_ecs_world *world, w_hook_fn hook_fn);
// unregister a global pre-set hook by ID
void w_ecs_unregister_component_pre_set_hook(struct w_ecs_world *world, size_t hook_id);

// register a global hook to fire before any component is removed (returns hook ID)
size_t w_ecs_register_component_pre_remove_hook(struct w_ecs_world *world, w_hook_fn hook_fn);
// unregister a global pre-remove hook by ID
void w_ecs_unregister_component_pre_remove_hook(struct w_ecs_world *world, size_t hook_id);

// register a hook to fire before a specific component is set (returns hook ID)
size_t w_ecs_register_component_id_pre_set_hook(struct w_ecs_world *world, w_entity_id comp_id, w_hook_fn hook_fn);
// unregister a component-specific pre-set hook by ID
void w_ecs_unregister_component_id_pre_set_hook(struct w_ecs_world *world, w_entity_id comp_id, size_t hook_id);

// register a hook to fire before a specific component is removed (returns hook ID)
size_t w_ecs_register_component_id_pre_remove_hook(struct w_ecs_world *world, w_entity_id comp_id, w_hook_fn hook_fn);
// unregister a component-specific pre-remove hook by ID
void w_ecs_unregister_component_id_pre_remove_hook(struct w_ecs_world *world, w_entity_id comp_id, size_t hook_id);

// register a hook to fire when an entity is created (returns hook ID)
size_t w_ecs_register_entity_create_hook(struct w_ecs_world *world, w_hook_fn hook_fn);
// unregister an entity create hook by ID
void w_ecs_unregister_entity_create_hook(struct w_ecs_world *world, size_t hook_id);

// register a hook to fire when an entity is destroyed (returns hook ID)
size_t w_ecs_register_entity_destroy_hook(struct w_ecs_world *world, w_hook_fn hook_fn);
// unregister an entity destroy hook by ID
void w_ecs_unregister_entity_destroy_hook(struct w_ecs_world *world, size_t hook_id);


/***************************
*  hook definition macros  *
***************************/

// define an update lifecycle hook function and its register function
// subtype: BEGIN, END, TIMESTEP_BEGIN, TIMESTEP_END, PHASE_BEGIN, PHASE_END
// inside work: world (struct w_ecs_world *) and action (struct w_scheduler_action *) are pre-cast
#define w_ecs_update_hook(name, subtype, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_update_hook(world, W_WORLD_HOOK_UPDATE_##subtype, name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		struct w_scheduler_action *action = (struct w_scheduler_action *)data; \
		(void)world; (void)action; \
		work; \
	} \

// define a startup hook function and its register function
// fires on first update (when update_result is INIT)
// inside work: world (struct w_ecs_world *) is pre-cast
#define w_ecs_startup_hook(name, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_startup_hook(world, name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		(void)world; (void)data; \
		work; \
	} \

// define a restart hook function and its register function
// fires when update_result is RESTART
// inside work: world (struct w_ecs_world *) is pre-cast
#define w_ecs_restart_hook(name, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_restart_hook(world, name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		(void)world; (void)data; \
		work; \
	} \

// define a shutdown hook function and its register function
// fires when update_result is SHUTDOWN
// inside work: world (struct w_ecs_world *) is pre-cast
#define w_ecs_shutdown_hook(name, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_shutdown_hook(world, name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		(void)world; (void)data; \
		work; \
	} \

// define a component set hook function and its register function
// fires when a component of the given type_id is set
// inside work: world (struct w_ecs_world *) and payload (struct w_component_action_payload *) are pre-cast
#define w_ecs_component_set_hook(name, type_id, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_component_set_hook(world, (type_id), name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		struct w_component_action_payload *payload = (struct w_component_action_payload *)data; \
		(void)world; (void)payload; \
		work; \
	} \

// define a component remove hook function and its register function
// fires when a component of the given type_id is removed
// inside work: world (struct w_ecs_world *) and payload (struct w_component_action_payload *) are pre-cast
#define w_ecs_component_remove_hook(name, type_id, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_component_remove_hook(world, (type_id), name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		struct w_component_action_payload *payload = (struct w_component_action_payload *)data; \
		(void)world; (void)payload; \
		work; \
	} \

// define a global component pre-set hook function and its register function
// fires before any component is set
// inside work: world (struct w_ecs_world *) and payload (struct w_component_action_payload *) are pre-cast
#define w_ecs_component_pre_set_hook(name, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_component_pre_set_hook(world, name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		struct w_component_action_payload *payload = (struct w_component_action_payload *)data; \
		(void)world; (void)payload; \
		work; \
	} \

// define a global component pre-remove hook function and its register function
// fires before any component is removed
// inside work: world (struct w_ecs_world *) and payload (struct w_component_action_payload *) are pre-cast
#define w_ecs_component_pre_remove_hook(name, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_component_pre_remove_hook(world, name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		struct w_component_action_payload *payload = (struct w_component_action_payload *)data; \
		(void)world; (void)payload; \
		work; \
	} \

// define a component-specific pre-set hook function and its register function
// fires before the component with comp_id is set
// inside work: world (struct w_ecs_world *) and payload (struct w_component_action_payload *) are pre-cast
#define w_ecs_component_id_pre_set_hook(name, comp_id, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_component_id_pre_set_hook(world, (comp_id), name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		struct w_component_action_payload *payload = (struct w_component_action_payload *)data; \
		(void)world; (void)payload; \
		work; \
	} \

// define a component-specific pre-remove hook function and its register function
// fires before the component with comp_id is removed
// inside work: world (struct w_ecs_world *) and payload (struct w_component_action_payload *) are pre-cast
#define w_ecs_component_id_pre_remove_hook(name, comp_id, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_component_id_pre_remove_hook(world, (comp_id), name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		struct w_component_action_payload *payload = (struct w_component_action_payload *)data; \
		(void)world; (void)payload; \
		work; \
	} \

// define an entity create hook function and its register function
// fires when an entity is created
// inside work: world (struct w_ecs_world *) and entity (w_entity_id *) are pre-cast
#define w_ecs_entity_create_hook(name, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_entity_create_hook(world, name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		w_entity_id entity = *(w_entity_id *)data; \
		(void)world; (void)entity; \
		work; \
	} \

// define an entity destroy hook function and its register function
// fires when an entity is destroyed
// inside work: world (struct w_ecs_world *) and entity (w_entity_id *) are pre-cast
#define w_ecs_entity_destroy_hook(name, work) \
	static void name(void *ctx, void *data); \
	static inline void name##_register(struct w_ecs_world *world) { \
		w_ecs_register_entity_destroy_hook(world, name); \
	} \
	static void name(void *ctx, void *data) { \
		struct w_ecs_world *world = (struct w_ecs_world *)ctx; \
		w_entity_id entity = *(w_entity_id *)data; \
		(void)world; (void)entity; \
		work; \
	} \


// temporarily disable buffering to execute code block directly
#define w_ecs_world_do_unbuffered(w, block) do { \
	bool _was_buffered = (w)->buffering_enabled; \
	(w)->buffering_enabled = false; \
	block \
	(w)->buffering_enabled = _was_buffered; \
} while(0)

/******************************
*  command buffer functions  *
******************************/
void w_ecs_cmd_set_entity_name(void *world, void *entity_name_payload);
void w_ecs_cmd_return_entity(void *world, void *entity);
void w_ecs_cmd_clear_entity_name(void *world, void *entity);
void w_ecs_cmd_set_component(void *world, void *payload);
void w_ecs_cmd_remove_component(void *world, void *payload);


/******************************
*  world init helper macros  *
******************************/
// these macros allow creating and freeing the entire world and its
// dependencies, essentially managed, to reduce boilerplate required

#define w_ecs_world_init_full(world_name) \
	struct w_arena _##world_name##_arena; \
	w_arena_init(&_##world_name##_arena, 0); \
	struct w_string_table _##world_name##_string_table; \
	    w_string_table_init(&_##world_name##_string_table, &_##world_name##_arena, \
                        WHISKER_STRING_TABLE_REALLOC_SIZE, \
                        WHISKER_STRING_TABLE_BUCKETS_SIZE, NULL); \
	struct w_ecs_world world_name; \
    w_ecs_world_init(&world_name, &_##world_name##_string_table, &_##world_name##_arena); \

#define w_ecs_world_update(world, custom_loop) \
	while (w_ecs_update(&world) != W_WORLD_UPDATE_RESULT_SHUTDOWN) { \
		custom_loop \
	} \

#define w_ecs_world_free_full(world) \
	w_ecs_world_free(&world); \
    w_string_table_free(&_##world##_string_table); \
    w_arena_free(&_##world##_arena); \

#define w_ecs_world_bootstrap(world_name, init_code) \
	w_ecs_world_init_full(world_name); \
	{ init_code }; \
	w_ecs_world_update(world_name, {}); \
	w_ecs_world_free_full(world_name); \

#define w_ecs_world_bootstrap_custom(world_name, init_code, custom_loop, dispose_code) \
	w_ecs_world_init_full(world_name); \
	{ init_code }; \
	w_ecs_world_update(world_name, custom_loop); \
	{ dispose_code }; \
	w_ecs_world_free_full(world_name); \


/***********
*  DEBUG  *
***********/

// print schedule with system names resolved from world->systems
void w_ecs_world_debug_print_schedule(struct w_ecs_world *world);

#endif /* WHISKER_ECS_WORLD_H */

