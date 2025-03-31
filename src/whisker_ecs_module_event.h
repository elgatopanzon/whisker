/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_module_event
 * @created     : Sunday Mar 30, 2025 21:06:52 CST
 */

#include "whisker_std.h"
#include "whisker_macros.h"
#include "whisker_ecs.h"
#include "whisker_ecs_pool.h"

#ifndef WHISKER_ECS_MODULE_EVENT_H
#define WHISKER_ECS_MODULE_EVENT_H

/*
 * events module
 * this module allows sending events that systems can react to.
 * the init function accepts an ECS instance and an entity pool instance.
 * the entity pool is registered with the ECS using the module's component store
 * this allows using the module itself to be stateless
 */

// macros accepting named components
#define whisker_ecs_module_event_create(e, c, i) \
	whisker_ecs_module_event_create_f(e, c, i);\
	// debug_log(DEBUG, ecs:event_create, "creating event %zu", i.id);

#define whisker_ecs_module_event_create_named(e, c, n) \
	whisker_ecs_module_event_create_f(e, c, whisker_ecs_e_create_named(e, #n)); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s", #n);

#define whisker_ecs_module_event_create_with_data(e, c, i, t, v) \
	whisker_ecs_module_event_create_with_data_f(e, c, i, sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu with data type %s", i.id, #t);

#define whisker_ecs_module_event_create_with_data_named(e, c, n, t, v) \
	whisker_ecs_module_event_create_with_data_f(e, c, whisker_ecs_e_create_named(e, #n), sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s with data type %s", #n, #t);

#define whisker_ecs_module_event_create_and_fire(e, c, i) \
	whisker_ecs_module_event_create_and_fire_f(e, c, i); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing", i.id);

#define whisker_ecs_module_event_create_and_fire_named(e, c, n) \
	whisker_ecs_module_event_create_and_fire_f(e, c, whisker_ecs_e_create_named(e, #n)); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing", #n);


#define whisker_ecs_module_event_set_data(e, c, ev, i, t, v) \
	whisker_ecs_module_event_set_data_f(e, c, ev, i, sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "setting event %zu data with data type %zu", i.id, #t);

#define whisker_ecs_module_event_set_data_named(e, c, ev, n, t, v) \
	whisker_ecs_module_event_set_data_f(e, c, ev, whisker_ecs_e_create_named(e, #n), sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "setting event %s data with data type %s", #n, #t);


#define whisker_ecs_module_event_fire_on(e, c, i, on) \
	whisker_ecs_module_event_fire_on_f(e, c, i, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing on %zu", i.id, on.id);

#define whisker_ecs_module_event_fire_on_named(e, c, n, on) \
	whisker_ecs_module_event_fire_on_f(e, c, whisker_ecs_e_create_named(e, #n), on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing on %zu", #n, on.id);

#define whisker_ecs_module_event_fire_on_with_data(e, c, i, t, v, on) \
	whisker_ecs_module_event_fire_on_with_data_f(e, c, i, sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing on %zu with data type %s", i.id, on.id, #t);

#define whisker_ecs_module_event_fire_on_with_data_named(e, c, n, t, v, on) \
	whisker_ecs_module_event_fire_on_with_data_f(e, c, whisker_ecs_e_create_named(e, #n), sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing on %zu with data type %s", #n, on.id, #t);

#define whisker_ecs_module_event_set_fire_on_data(e, c, i, t, v, on) \
	whisker_ecs_module_event_set_fire_on_data_f(e, c, i, sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "setting fire on event %zu on %zu with data type %s", i.id, on.id, #t);

#define whisker_ecs_module_event_set_fire_on_data_named(e, c, n, t, v, on) \
	whisker_ecs_module_event_set_fire_on_data_f(e, c, whisker_ecs_e_create_named(e, #n), sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "setting fire on event %s on %zu with data type %s", #n, on.id, #t);

void whisker_ecs_module_event_init(whisker_ecs *ecs, whisker_ecs_pool *entity_pool);

// event management functions
whisker_ecs_entity_id whisker_ecs_module_event_create_event(whisker_ecs_entities *entities, whisker_ecs_components *components);
whisker_ecs_entity_id whisker_ecs_module_event_create_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id);
whisker_ecs_entity_id whisker_ecs_module_event_create_with_data_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data);
void whisker_ecs_module_event_create_and_fire_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id);
void whisker_ecs_module_event_set_data_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_entity_id, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data);
void whisker_ecs_module_event_fire(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_entity_id);
void whisker_ecs_module_event_fire_on_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id, whisker_ecs_entity_id fire_on_entity_id);
void whisker_ecs_module_event_fire_on_with_data_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data, whisker_ecs_entity_id fire_on_entity_id);
void whisker_ecs_module_event_set_fire_on_data_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data, whisker_ecs_entity_id fire_on_entity_id);

struct whisker_ecs_module_event_pool_component
{
	whisker_ecs_pool *pool;
};

struct whisker_ecs_module_event_cull_event_component 
{
	whisker_ecs_entity_id entity_id;
	whisker_ecs_entity_id component_id;
};

// ECS systems
void whisker_ecs_module_event_system_cull_events(whisker_ecs_system_context *context);
void whisker_ecs_module_event_system_cull_event_components(whisker_ecs_system_context *context);

#endif /* WHISKER_ECS_MODULE_EVENT_H */

