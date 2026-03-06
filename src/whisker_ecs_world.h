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

#ifndef WHISKER_ECS_WORLD_H
#define WHISKER_ECS_WORLD_H

enum W_WORLD_UPDATE_RESULT
{ 
	W_WORLD_UPDATE_RESULT_CONTINUE = 0,
	W_WORLD_UPDATE_RESULT_RESTART = 1,
	W_WORLD_UPDATE_RESULT_SHUTDOWN = 2,
};

enum W_WORLD_HOOK
{
	W_WORLD_HOOK_UPDATE_BEGIN,
	W_WORLD_HOOK_UPDATE_END,
	W_WORLD_HOOK_UPDATE_TIMESTEP_BEGIN,
	W_WORLD_HOOK_UPDATE_TIMESTEP_END,
	W_WORLD_HOOK_UPDATE_PHASE_BEGIN,
	W_WORLD_HOOK_UPDATE_PHASE_END,
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
	struct w_hook_registry hooks;

	// buffering
	struct w_command_buffer command_buffer;
	bool buffering_enabled;

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

// set a component on an entity
// (note: this is not thread-safe, it will create the component type)
void *w_ecs_set_component_(struct w_ecs_world *world, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size);

// get the component data on an entity, if it exists
void *w_ecs_get_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id);

// remove a component from an entity
void w_ecs_remove_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id);

// check if an entity has a component
bool w_ecs_has_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id);

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

// register a system with the ECS scheduler
size_t w_ecs_register_system(struct w_ecs_world *world, struct w_system *system);
size_t w_ecs_set_system_state(struct w_ecs_world *world, size_t system_id, bool system_state);
struct w_system *w_ecs_get_system_entry(struct w_ecs_world *world, size_t system_id);

// register a scheduler phase
size_t w_ecs_register_system_phase(struct w_ecs_world *world, struct w_scheduler_phase *phase);

// get a scheduler phase by ID
struct w_scheduler_phase *w_ecs_get_system_phase(struct w_ecs_world *world, size_t phase_id);

// set enabled state of a scheduler phase
void w_ecs_set_system_phase_state(struct w_ecs_world *world, size_t phase_id, bool state);

// set a phase to run before another phase
void w_ecs_set_system_phase_runs_before(struct w_ecs_world *world, size_t phase_id, size_t runs_before_phase_id);

// set a phase to run after another phase
void w_ecs_set_system_phase_runs_after(struct w_ecs_world *world, size_t phase_id, size_t runs_after_phase_id);

// reset all scheduler phases
void w_ecs_reset_system_phases(struct w_ecs_world *world);

// register a scheduler time step
size_t w_ecs_register_system_time_step(struct w_ecs_world *world, struct w_scheduler_time_step *time_step);

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


/***********
*  hooks  *
***********/
static inline void w_ecs_update_hook_flush_command_buffer_(void *world, void *action);


#endif /* WHISKER_ECS_WORLD_H */

