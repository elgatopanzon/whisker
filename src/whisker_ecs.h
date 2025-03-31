/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:17 CST
 */

#include "whisker_std.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_component.h"
#include "whisker_ecs_system.h"
#include "whisker_ecs_pool.h"
#include "whisker_thread_pool.h"
#include "whisker_ecs_event.h"

#ifndef WHISKER_ECS_H
#define WHISKER_ECS_H

#define WHISKER_ECS_PROCESS_PHASE_TIME_STEP_DEFAULT 0
#define WHISKER_ECS_PROCESS_PHASE_TIME_STEP_FIXED 1
#define WHISKER_ECS_PROCESS_PHASE_TIME_STEP_RENDERING 2

// default process phase groups and their time step configs
// the default for all is 0 (variable update rate)
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_RATE 60
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_UNCAPPED false
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_CLAMP true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_SNAP true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_AVERAGE true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_ACCUMULATION true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_ACCUMULATION_CLAMP true
#define WHISKER_ECS_PROCESS_PHASE_DEFAULT_UPDATE_COUNT_MAX 1

// the on_startup phase will run once at startup only
#define WHISKER_ECS_PROCESS_PHASE_ON_STARTUP "w_phase_on_startup"

// pre_load is the first phase of every update loop
#define WHISKER_ECS_PROCESS_PHASE_PRE_LOAD "w_phase_pre_load"

// the second phase is pre_update
#define WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE "w_phase_pre_update"

// fixed_update is a special phase running at the target update rate
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE "w_phase_fixed_update"
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_RATE 60
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_CLAMP true
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_SNAP true
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_AVERAGE true
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION true
#define WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION_CLAMP true

// after fixed_update, the main on_update phase is run
#define WHISKER_ECS_PROCESS_PHASE_ON_UPDATE "w_phase_on_update"

// after on_update, post_update is run
#define WHISKER_ECS_PROCESS_PHASE_POST_UPDATE "w_phase_post_update"

// the last phase before the rendering phases is final
#define WHISKER_ECS_PROCESS_PHASE_FINAL "w_phase_final"


#define WHISKER_ECS_PROCESS_PHASE_PRE_RENDER "w_phase_pre_render"
#define WHISKER_ECS_PROCESS_PHASE_PRE_RENDER_UNCAPPED true

#define WHISKER_ECS_PROCESS_PHASE_ON_RENDER "w_phase_on_render"
#define WHISKER_ECS_PROCESS_PHASE_ON_RENDER_UNCAPPED true

#define WHISKER_ECS_PROCESS_PHASE_POST_RENDER "w_phase_post_render"
#define WHISKER_ECS_PROCESS_PHASE_POST_RENDER_UNCAPPED true

#define WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER "w_phase_final_render"
#define WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER_UNCAPPED true

#define WHISKER_ECS_PROCESS_THREADED_AUTO -1
#define WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD 0

struct whisker_ecs_component_sort_request 
{
	whisker_ecs_components *components;
	whisker_ecs_entity_id component_id;
};

typedef struct whisker_ecs
{
	whisker_ecs_entities *entities;
	whisker_ecs_components *components;
	whisker_ecs_systems *systems;
	whisker_ecs_system_context system_update_context;
	whisker_thread_pool *general_thread_pool;
	whisker_arr_declare(struct whisker_ecs_component_sort_request, component_sort_requests);
	whisker_ecs_pool *events_entity_pool;
} whisker_ecs;


whisker_ecs *whisker_ecs_create();
void whisker_ecs_free(whisker_ecs *ecs);

// system functions
whisker_ecs_system *whisker_ecs_register_system(whisker_ecs *ecs, void (*system_ptr)(struct whisker_ecs_system_context*), char *system_name, char *process_phase_name, size_t thread_count);
size_t whisker_ecs_register_process_phase_time_step(whisker_ecs *ecs, whisker_time_step time_step);
whisker_ecs_entity_id whisker_ecs_register_process_phase(whisker_ecs *ecs, char *phase_name, size_t time_step_id);
void whisker_ecs_set_process_phase_order(whisker_ecs *ecs, char **phase_names, size_t phase_count);

// system update functions
void whisker_ecs_update(whisker_ecs *ecs, double delta_time);
void whisker_ecs_update_process_deferred_actions(whisker_ecs *ecs);
void whisker_ecs_update_generate_component_events_(whisker_ecs *ecs);
void whisker_ecs_update_process_deferred_component_actions_(whisker_ecs *ecs);
void whisker_ecs_update_process_changed_components_(whisker_ecs *ecs);
void whisker_ecs_sort_component_thread_func_(void *component_sort_request, whisker_thread_pool_context *t);
void whisker_ecs_sort_component_thread_func_all_(void *component_sort_request, whisker_thread_pool_context *t);
void whisker_ecs_update_process_deferred_entity_actions_(whisker_ecs *ecs);

// entity shortcut functions
whisker_ecs_entity_id whisker_ecs_create_entity(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_create_named_entity(whisker_ecs_entities *entities, char* name);
void whisker_ecs_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
void whisker_ecs_soft_destroy_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
void whisker_ecs_soft_revive_entity(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
whisker_ecs_entity_id whisker_ecs_create_entity_deferred(whisker_ecs_entities *entities);
whisker_ecs_entity_id whisker_ecs_create_named_entity_deferred(whisker_ecs_entities *entities, char* name);
void whisker_ecs_destroy_entity_deferred(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);
bool whisker_ecs_is_alive(whisker_ecs_entities *entities, whisker_ecs_entity_id entity_id);

// component functions
whisker_ecs_entity_id whisker_ecs_component_id(whisker_ecs_entities *entities, char* component_name);
void *whisker_ecs_get_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);
void *whisker_ecs_set_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, size_t component_size, whisker_ecs_entity_id entity_id, void *value);
void whisker_ecs_remove_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_named_component(whisker_ecs_entities *entities, whisker_ecs_components *components, char *component_name, whisker_ecs_entity_id entity_id);

void *whisker_ecs_get_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void *whisker_ecs_set_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value);
void whisker_ecs_remove_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
bool whisker_ecs_has_component(whisker_ecs_components *components, whisker_ecs_entity_id component_id, whisker_ecs_entity_id entity_id);
void whisker_ecs_create_deferred_component_action(whisker_ecs_components *components, whisker_ecs_entity_id component_id, size_t component_size, whisker_ecs_entity_id entity_id, void *value, enum WHISKER_ECS_COMPONENT_DEFERRED_ACTION action);

// built-in systems
void whisker_ecs_system_deregister_startup_phase(whisker_ecs_system_context *context);

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

