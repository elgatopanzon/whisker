/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_module_event
 * @created     : Sunday Mar 30, 2025 21:08:51 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_module_event.h"

static void wm_event_set_pool(struct w_ecs *ecs, struct w_pool *entity_pool);
static struct w_pool *wm_event_get_pool(struct w_world *world);
static w_entity_id wm_event_get_pool_entity(struct w_world *world);
static w_entity_id wm_event_get_module_entity(struct w_world *world);

void wm_event_init(struct w_ecs *ecs, struct w_pool *entity_pool)
{
	// set the pool component
	wm_event_set_pool(ecs, entity_pool);

	w_register_system(ecs, wm_event_system_cull_event_components, "w_module_event_cull_events", W_PHASE_FINAL, W_SCHED_THREAD_AUTO);
	w_register_system(ecs, wm_event_system_cull_events, "w_module_event_cull_event_data", W_PHASE_FINAL, W_SCHED_THREAD_AUTO);

	// disable propagation of changes to ensure that events don't trigger new
	// events
	entity_pool->propagate_component_changes = false;

	// set the event components for the prototype entity
	w_pool_set_prototype_named_tag(entity_pool, w_module_event);
	struct wm_event_cull_event_component cull = {0};
	w_pool_set_prototype_named_component(entity_pool, w_module_event_cull_data, cull, &cull);
}

static void wm_event_set_pool(struct w_ecs *ecs, struct w_pool *entity_pool)
{
	struct wm_event_pool_component pool_component = {
		.pool = entity_pool,
	};
	w_set_component(ecs->world, wm_event_get_pool_entity(ecs->world), sizeof(pool_component), wm_event_get_module_entity(ecs->world), &pool_component);
}

static struct w_pool *wm_event_get_pool(struct w_world *world)
{
	struct wm_event_pool_component *pool_component = w_get_component(world, wm_event_get_pool_entity(world), wm_event_get_module_entity(world));
	struct w_pool *pool = pool_component->pool;

	return pool;
}

static w_entity_id wm_event_get_pool_entity(struct w_world *world)
{
	return w_create_named_entity_non_deferred(world, "w_module_event_pool");
}
static w_entity_id wm_event_get_module_entity(struct w_world *world)
{
	return w_create_named_entity_non_deferred(world, "w_module_event");
}


// create an entity to represent an event
w_entity_id wm_event_create_event(struct w_world *world)
{
	struct w_pool *pool = wm_event_get_pool(world);
	w_entity_id ev = w_request_pool_entity(pool);

	pool->world->entities->entities[ev.index].destroyed = true;
	w_create_deferred_entity_action(pool->world, ev, W_ENTITY_DEFERRED_ACTION_CREATE);

	// set the t_event component on the entity
	wm_event_set_data_f(world, ev, w_create_named_entity_non_deferred(pool->world, "w_module_event"), sizeof(bool), &(bool){0});

	// make the entity unmanaged
	w_set_entity_unmanaged(pool->world, ev);

	return ev;
}

// create an event without attached data
w_entity_id wm_event_create_f(struct w_world *world, w_entity_id event_component_id)
{
	w_entity_id ev = wm_event_create_event(world);
	wm_event_set_data_f(world, ev, event_component_id, 0, &(bool){0});
	return ev;
}

// create an event with attached data
w_entity_id wm_event_create_with_data_f(struct w_world *world, w_entity_id event_component_id, size_t component_size, void *event_data)
{
	w_entity_id ev = wm_event_create_event(world);
	wm_event_set_data_f(world, ev, event_component_id, component_size, event_data);
	return ev;
}

// create an event entity and immediately fire it
void wm_event_create_and_fire_f(struct w_world *world, w_entity_id event_component_id)
{
	w_entity_id ev = wm_event_create_event(world);
	wm_event_set_data_f(world, ev, event_component_id, sizeof(bool), &(bool){0});
	wm_event_fire(world, ev);
}

// set component data on an event entity
void wm_event_set_data_f(struct w_world *world, w_entity_id event_entity_id, w_entity_id event_component_id, size_t component_size, void *event_data)
{
	w_create_deferred_component_action_(world, event_component_id, component_size, event_entity_id, event_data, W_COMPONENT_DEFERRED_ACTION_SET, false);
}

// fire off an event entity
void wm_event_fire(struct w_world *world, w_entity_id event_entity_id)
{
	// firing an event is just making it managed again
	w_set_entity_managed(world, event_entity_id);
}

// fire an event, attaching it instead to the given entity ID
void wm_event_fire_on_f(struct w_world *world, w_entity_id event_component_id, w_entity_id fire_on_entity_id)
{
	// add the event to the provided entity ID
	wm_event_set_fire_on_data_f(world, event_component_id, sizeof(bool), &(bool){0}, fire_on_entity_id);
}

// fire an event, attaching it with data to the given entity ID
void wm_event_fire_on_with_data_f(struct w_world *world, w_entity_id event_component_id, size_t component_size, void *event_data, w_entity_id fire_on_entity_id)
{
	wm_event_set_fire_on_data_f(world, event_component_id, component_size, event_data, fire_on_entity_id);
}

// set data on an entity, will be removed along with the event
void wm_event_set_fire_on_data_f(struct w_world *world, w_entity_id event_component_id, size_t component_size, void *event_data, w_entity_id fire_on_entity_id)
{
	wm_event_set_data_f(world, fire_on_entity_id, event_component_id, component_size, event_data);

	// make sure the event data is culled for this event
	struct wm_event_cull_event_component cull_component = {
		.entity_id = fire_on_entity_id,
		.component_id = event_component_id,
	};

	// create a data cull event for this entity to remove the fire_on data
	w_entity_id cull_ev = wm_event_create_with_data_f(world, w_create_named_entity_non_deferred(world, "w_module_event_cull_data"), sizeof(cull_component), &cull_component);
	wm_event_fire(world, cull_ev);
}

/*****************
*  ECS systems  *
*****************/

// system interested in t_event entities
// the system should be added to the last executed phase to ensure that all
// event entities are destroyed and not picked up twice
void wm_event_system_cull_events(struct w_sys_context *context)
{
	struct w_iterator *itor = w_query(context, 0, STR(w_module_event), "", "");
	while (w_iterate(itor)) 
	{
		/* debug_log(DEBUG, ecs:system_cull_events, "culling event entity %zu", itor->entity_id.id); */

		w_create_deferred_component_action_(context->world, itor->component_ids_rw[0], 0, itor->entity_id, NULL, W_COMPONENT_DEFERRED_ACTION_REMOVE, false);

		w_destroy_entity(context->world, itor->entity_id);
	}
}

// companion system to the event cull system.
// this system will accept components of type struct whisker_ecs_module_eventent_cull_event_component
// each entity is a request to issue a deferred removal of component data.
// this is because, in the case of fire_on, we don't have a t_event component
// and we don't want to cull the actual entity.
void wm_event_system_cull_event_components(struct w_sys_context *context)
{
	struct w_iterator *itor = w_query(context, 0, STR(w_module_event_cull_data), "", "");
	while (w_iterate(itor)) 
	{
		// issue the deferred component removal request
		struct wm_event_cull_event_component *cull = w_itor_get(itor, 0);

		if (cull->component_id.id > 0 && cull->entity_id.id > 0)
		{
			/* debug_log(DEBUG, ecs:system_cull_events, "event: culling fire_on component %s (%zu) from entity %zu", w_get_entity(context->world->entities, cull->component_id)->name, cull->component_id.id, cull->entity_id.id); */

			w_create_deferred_component_action_(context->world, cull->component_id, 0, cull->entity_id, NULL, W_COMPONENT_DEFERRED_ACTION_REMOVE, false);
		}
		w_create_deferred_component_action_(context->world, itor->component_ids_rw[0], 0, itor->entity_id, NULL, W_COMPONENT_DEFERRED_ACTION_REMOVE, false);
	}
}
