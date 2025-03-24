/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:50 CST
 */

#include <pthread.h>

#include "whisker_std.h"
#include "whisker_sparse_set.h"
#include "whisker_time.h"
#include "whisker_thread_pool.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_component.h"

#ifndef WHISKER_ECS_SYSTEM_H
#define WHISKER_ECS_SYSTEM_H

// default process phase groups and their suggested update rates
#define WHISKER_ECS_PROCESS_PHASE_ON_STARTUP "w_phase_on_startup"
#ifndef WHISKER_ECS_PROCESS_PHASE_ON_STARTUP_RATE
#define WHISKER_ECS_PROCESS_PHASE_ON_STARTUP_RATE 60
#endif

#define WHISKER_ECS_PROCESS_PHASE_PRE_LOAD "w_phase_pre_load"
#ifndef WHISKER_ECS_PROCESS_PHASE_PRE_LOAD_RATE
#define WHISKER_ECS_PROCESS_PHASE_PRE_LOAD_RATE 60
#endif

#define WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE "w_phase_pre_update"
#ifndef WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE_RATE
#define WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE_RATE 60
#endif

#define WHISKER_ECS_PROCESS_PHASE_ON_UPDATE "w_phase_on_update"
#ifndef WHISKER_ECS_PROCESS_PHASE_ON_UPDATE_RATE
#define WHISKER_ECS_PROCESS_PHASE_ON_UPDATE_RATE 60
#endif

#define WHISKER_ECS_PROCESS_PHASE_POST_UPDATE "w_phase_post_update"
#ifndef WHISKER_ECS_PROCESS_PHASE_POST_UPDATE_RATE
#define WHISKER_ECS_PROCESS_PHASE_POST_UPDATE_RATE 60
#endif

#define WHISKER_ECS_PROCESS_PHASE_PRE_RENDER "w_phase_pre_render"
#ifndef WHISKER_ECS_PROCESS_PHASE_PRE_RENDER_RATE
#define WHISKER_ECS_PROCESS_PHASE_PRE_RENDER_RATE 0
#endif

#define WHISKER_ECS_PROCESS_PHASE_ON_RENDER "w_phase_on_render"
#ifndef WHISKER_ECS_PROCESS_PHASE_ON_RENDER_RATE
#define WHISKER_ECS_PROCESS_PHASE_ON_RENDER_RATE 0
#endif

#define WHISKER_ECS_PROCESS_PHASE_POST_RENDER "w_phase_post_render"
#ifndef WHISKER_ECS_PROCESS_PHASE_POST_RENDER_RATE
#define WHISKER_ECS_PROCESS_PHASE_POST_RENDER_RATE 0
#endif

#define WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER "w_phase_final_render"
#ifndef WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER_RATE
#define WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER_RATE 0
#endif

#define WHISKER_ECS_PROCESS_PHASE_FINAL "w_phase_final"
#ifndef WHISKER_ECS_PROCESS_PHASE_FINAL_RATE
#define WHISKER_ECS_PROCESS_PHASE_FINAL_RATE 0
#endif

typedef struct whisker_ecs_system_iterator
{
	// the master index points to the sparse set we're currently iterating
	size_t master_index;

	// current cursor position in the master iterator
	size_t cursor;
	size_t cursor_max;
	size_t count;
	whisker_ecs_entity_id entity_id;

	// array of component name IDs to match with
	whisker_arr_declare(whisker_ecs_entity_id, component_ids_rw);
	whisker_arr_declare(whisker_ecs_entity_id, component_ids_w);
	whisker_arr_declare(whisker_ecs_entity_id, component_ids_opt);

	// component arrays, including read/write/optional
	whisker_arr_declare(whisker_sparse_set *, component_arrays);
	whisker_arr_declare(size_t, component_arrays_cursors);
} whisker_ecs_system_iterator;

typedef struct whisker_ecs_system_context
{
	// entity ID of system
	whisker_ecs_entity_id system_entity_id;

	// iterators used by this context
	whisker_sparse_set *iterators;

	// pointers to ECS entities and components
	whisker_ecs_components *components;
	whisker_ecs_entities *entities;

	// pointer to the system's time step instance
	whisker_time_step *process_phase_time_step;

	// delta time since the last system update
	double delta_time;

	// managed thread id
	uint64_t thread_id;
	uint64_t thread_max;

	// system pointer from main system
	void (*system_ptr)(struct whisker_ecs_system_context*);
} whisker_ecs_system_context;

// component acting as a system container
typedef struct whisker_ecs_system
{
	whisker_ecs_entity_id entity_id;
	whisker_ecs_entity_id process_phase_id;
	void (*system_ptr)(struct whisker_ecs_system_context*);
	int8_t thread_id;
	int8_t thread_count;
	double last_update;
	double delta_time;
	whisker_time_step *process_phase_time_step;

	whisker_ecs_components *components;
	whisker_ecs_entities *entities;
	// whisker_sparse_set *iterators;

	whisker_arr_declare(whisker_ecs_system_context *, thread_contexts);
	whisker_thread_pool *thread_pool;
} whisker_ecs_system;

typedef struct whisker_ecs_system_process_phase
{
	whisker_ecs_entity_id id;
	whisker_time_step time_step;
} whisker_ecs_system_process_phase;

typedef struct whisker_ecs_systems
{
	whisker_arr_declare(whisker_ecs_system, systems);
	whisker_arr_declare(whisker_ecs_system_process_phase, process_phases);
	size_t system_id;
} whisker_ecs_systems;

// system management functions
whisker_ecs_systems * whisker_ecs_s_create_systems();
void whisker_ecs_s_init_systems(whisker_ecs_systems *systems);
whisker_ecs_systems * whisker_ecs_s_create_and_init_systems();
void whisker_ecs_s_free_systems(whisker_ecs_systems *systems);
void whisker_ecs_s_free_systems_all(whisker_ecs_systems *systems);

whisker_ecs_system_context *whisker_ecs_s_create_system_context(whisker_ecs_system *system);
void whisker_ecs_s_free_system_context(whisker_ecs_system_context *context);

// system operation functions
whisker_ecs_system* whisker_ecs_s_register_system(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system system);
void whisker_ecs_s_free_system(whisker_ecs_system *system);
void whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time);
void whisker_ecs_s_update_system(whisker_ecs_system *system, whisker_ecs_system_context *context);
void whisker_ecs_s_update_system_thread_(void *context);

// system process phases functions
void whisker_ecs_s_register_process_phase(whisker_ecs_systems *systems, whisker_ecs_entity_id component_id, double update_rate_sec);
void whisker_ecs_s_deregister_process_phase(whisker_ecs_systems *systems, whisker_ecs_entity_id component_id);
void whisker_ecs_s_reset_process_phases(whisker_ecs_systems *systems);

// system component functions
void *whisker_ecs_s_get_component(whisker_ecs_system *system, size_t index, size_t size, whisker_ecs_entity_id entity_id, bool read_or_write);
void whisker_ecs_s_init_component_cache(whisker_ecs_system *system, char *name, int index, size_t size, bool read_or_write);
int whisker_ecs_s_get_component_name_index(whisker_ecs_system *system, char* component_names, char* component_name);

// system iterator functions
whisker_ecs_system_iterator *whisker_ecs_s_create_iterator();
whisker_ecs_system_iterator *whisker_ecs_s_get_iterator(whisker_ecs_system_context *context, size_t itor_index, char *read_components, char *write_components, char *optional_components);
void whisker_ecs_s_free_iterator(whisker_ecs_system_iterator *itor);
bool whisker_ecs_s_iterate(whisker_ecs_system_iterator *itor);
void whisker_ecs_s_init_iterator(whisker_ecs_system_context *context, whisker_ecs_system_iterator *itor, char *read_components, char *write_components, char *optional_components);

#define whisker_ecs_itor_get(itor, idx) itor->component_arrays[idx]->dense + itor->component_arrays_cursors[idx] * itor->component_arrays[idx]->element_size

#endif /* WHISKER_ECS_SYSTEM_H */

