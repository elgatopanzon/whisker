/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_event
 * @created     : Friday Mar 28, 2025 13:20:38 CST
 */

#include "whisker_std.h"
#include "whisker_ecs_entity.h"
#include "whisker_ecs_component.h"
#include "whisker_ecs_system.h"
#include "whisker_ecs_pool.h"

#ifndef WHISKER_ECS_EVENT_H
#define WHISKER_ECS_EVENT_H

// macros accepting named components
#define whisker_ecs_ev_create(p, i) \
	whisker_ecs_ev_create_f(p, i);\
	// debug_log(DEBUG, ecs:event_create, "creating event %zu", i.id);

#define whisker_ecs_ev_create_named(p, n) \
	whisker_ecs_ev_create_f(p, whisker_ecs_e_create_named(e, #n)); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s", #n);

#define whisker_ecs_ev_create_with_data(p, i, t, v) \
	whisker_ecs_ev_create_with_data_f(p, i, sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu with data type %s", i.id, #t);

#define whisker_ecs_ev_create_with_data_named(p, n, t, v) \
	whisker_ecs_ev_create_with_data_f(p, whisker_ecs_e_create_named(e, #n), sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s with data type %s", #n, #t);

#define whisker_ecs_ev_create_and_fire(p, i) \
	whisker_ecs_ev_create_and_fire_f(p, i); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing", i.id);

#define whisker_ecs_ev_create_and_fire_named(p, n) \
	whisker_ecs_ev_create_and_fire_f(p, whisker_ecs_e_create_named(e, #n)); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing", #n);


#define whisker_ecs_ev_set_data(p, ev, i, t, v) \
	whisker_ecs_ev_set_data_f(p, ev, i, sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "setting event %zu data with data type %zu", i.id, #t);

#define whisker_ecs_ev_set_data_named(p, ev, n, t, v) \
	whisker_ecs_ev_set_data_f(p, ev, whisker_ecs_e_create_named(e, #n), sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "setting event %s data with data type %s", #n, #t);


#define whisker_ecs_ev_fire_on(p, i, on) \
	whisker_ecs_ev_fire_on_f(p, i, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing on %zu", i.id, on.id);

#define whisker_ecs_ev_fire_on_named(p, n, on) \
	whisker_ecs_ev_fire_on_f(p, whisker_ecs_e_create_named(e, #n), on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing on %zu", #n, on.id);

#define whisker_ecs_ev_fire_on_with_data(p, i, t, v, on) \
	whisker_ecs_ev_fire_on_with_data_f(p, i, sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing on %zu with data type %s", i.id, on.id, #t);

#define whisker_ecs_ev_fire_on_with_data_named(p, n, t, v, on) \
	whisker_ecs_ev_fire_on_with_data_f(p, whisker_ecs_e_create_named(e, #n), sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing on %zu with data type %s", #n, on.id, #t);

#define whisker_ecs_ev_set_fire_on_data(p, i, t, v, on) \
	whisker_ecs_ev_set_fire_on_data_f(p, i, sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "setting fire on event %zu on %zu with data type %s", i.id, on.id, #t);

#define whisker_ecs_ev_set_fire_on_data_named(p, n, t, v, on) \
	whisker_ecs_ev_set_fire_on_data_f(p, whisker_ecs_e_create_named(e, #n), sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "setting fire on event %s on %zu with data type %s", #n, on.id, #t);

// event management functions
whisker_ecs_entity_id whisker_ecs_ev_create_event(whisker_ecs_pool *pool);
whisker_ecs_entity_id whisker_ecs_ev_create_f(whisker_ecs_pool *pool, whisker_ecs_entity_id event_component_id);
whisker_ecs_entity_id whisker_ecs_ev_create_with_data_f(whisker_ecs_pool *pool, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data);
void whisker_ecs_ev_create_and_fire_f(whisker_ecs_pool *pool, whisker_ecs_entity_id event_component_id);
void whisker_ecs_ev_set_data_f(whisker_ecs_pool *pool, whisker_ecs_entity_id event_entity_id, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data);
void whisker_ecs_ev_fire(whisker_ecs_pool *pool, whisker_ecs_entity_id event_entity_id);
void whisker_ecs_ev_fire_on_f(whisker_ecs_pool *pool, whisker_ecs_entity_id event_component_id, whisker_ecs_entity_id fire_on_entity_id);
void whisker_ecs_ev_fire_on_with_data_f(whisker_ecs_pool *pool, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data, whisker_ecs_entity_id fire_on_entity_id);
void whisker_ecs_ev_set_fire_on_data_f(whisker_ecs_pool *pool, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data, whisker_ecs_entity_id fire_on_entity_id);

// ECS components and systems to manage events
struct whisker_ecs_event_cull_event_component 
{
	whisker_ecs_entity_id entity_id;
	whisker_ecs_entity_id component_id;
};

void whisker_ecs_ev_system_cull_events(whisker_ecs_system_context *context);
void whisker_ecs_ev_system_cull_event_components(whisker_ecs_system_context *context);

#endif /* WHISKER_ECS_EVENT_H */

