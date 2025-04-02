/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : wm_event
 * @created     : Sunday Mar 30, 2025 21:06:52 CST
 */

#include "whisker_std.h"
#include "whisker_macros.h"
#include "whisker_ecs.h"

#ifndef wm_event_H
#define wm_event_H

/*
 * events module
 * this module allows sending events that systems can react to.
 * the init function accepts an ECS instance and an entity pool instance.
 * the entity pool is registered with the ECS using the module's component store
 * this allows using the module itself to be stateless
 */

// macros accepting named components
#define wm_event_create(w, i) \
	wm_event_create_f(w, i);\
	// debug_log(DEBUG, ecs:event_create, "creating event %zu", i.id);

#define wm_event_create_named(w, n) \
	wm_event_create_f(w, w_create_named_entity(w, #n)); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s", #n);

#define wm_event_create_with_data(w, i, t, v) \
	wm_event_create_with_data_f(w, i, sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu with data type %s", i.id, #t);

#define wm_event_create_with_data_named(w, n, t, v) \
	wm_event_create_with_data_f(w, w_create_named_entity(w, #n), sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s with data type %s", #n, #t);

#define wm_event_create_and_fire(w, i) \
	wm_event_create_and_fire_f(w, i); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing", i.id);

#define wm_event_create_and_fire_named(w, n) \
	wm_event_create_and_fire_f(w, w_create_named_entity(w, #n)); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing", #n);


#define wm_event_set_data(w, ev, i, t, v) \
	wm_event_set_data_f(w, ev, i, sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "setting event %zu data with data type %zu", i.id, #t);

#define wm_event_set_data_named(w, ev, n, t, v) \
	wm_event_set_data_f(w, ev, w_create_named_entity(w, #n), sizeof(t), v); \
	// debug_log(DEBUG, ecs:event_create, "setting event %s data with data type %s", #n, #t);


#define wm_event_fire_on(w, i, on) \
	wm_event_fire_on_f(w, i, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing on %zu", i.id, on.id);

#define wm_event_fire_on_named(w, n, on) \
	wm_event_fire_on_f(w, w_create_named_entity(w, #n), on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing on %zu", #n, on.id);

#define wm_event_fire_on_with_data(w, i, t, v, on) \
	wm_event_fire_on_with_data_f(w, i, sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %zu and firing on %zu with data type %s", i.id, on.id, #t);

#define wm_event_fire_on_with_data_named(w, n, t, v, on) \
	wm_event_fire_on_with_data_f(w, w_create_named_entity(w, #n), sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "creating event %s and firing on %zu with data type %s", #n, on.id, #t);

#define wm_event_set_fire_on_data(w, i, t, v, on) \
	wm_event_set_fire_on_data_f(w, i, sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "setting fire on event %zu on %zu with data type %s", i.id, on.id, #t);

#define wm_event_set_fire_on_data_named(w, n, t, v, on) \
	wm_event_set_fire_on_data_f(w, w_create_named_entity(w, #n), sizeof(t), v, on); \
	// debug_log(DEBUG, ecs:event_create, "setting fire on event %s on %zu with data type %s", #n, on.id, #t);

void wm_event_init(struct w_ecs *ecs, struct w_pool *entity_pool);

// event management functions
w_entity_id wm_event_create_event(struct w_world *world);
w_entity_id wm_event_create_f(struct w_world *world, w_entity_id event_component_id);
w_entity_id wm_event_create_with_data_f(struct w_world *world, w_entity_id event_component_id, size_t component_size, void *event_data);
void wm_event_create_and_fire_f(struct w_world *world, w_entity_id event_component_id);
void wm_event_set_data_f(struct w_world *world, w_entity_id event_entity_id, w_entity_id event_component_id, size_t component_size, void *event_data);
void wm_event_fire(struct w_world *world, w_entity_id event_entity_id);
void wm_event_fire_on_f(struct w_world *world, w_entity_id event_component_id, w_entity_id fire_on_entity_id);
void wm_event_fire_on_with_data_f(struct w_world *world, w_entity_id event_component_id, size_t component_size, void *event_data, w_entity_id fire_on_entity_id);
void wm_event_set_fire_on_data_f(struct w_world *world, w_entity_id event_component_id, size_t component_size, void *event_data, w_entity_id fire_on_entity_id);

struct wm_event_pool_component
{
	struct w_pool *pool;
};

struct wm_event_cull_event_component 
{
	w_entity_id entity_id;
	w_entity_id component_id;
};

// ECS systems
void wm_event_system_cull_events(struct w_sys_context *context);
void wm_event_system_cull_event_components(struct w_sys_context *context);

#endif /* wm_event_H */

