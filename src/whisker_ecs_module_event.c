/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_module_event
 * @created     : Sunday Mar 30, 2025 21:08:51 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_module_event.h"

static void whisker_ecs_module_event_set_pool(whisker_ecs *ecs, whisker_ecs_pool *entity_pool);
static whisker_ecs_pool *whisker_ecs_module_event_get_pool(struct whisker_ecs_world *world);
static whisker_ecs_entity_id whisker_ecs_module_event_get_pool_entity(struct whisker_ecs_world *world);
static whisker_ecs_entity_id whisker_ecs_module_event_get_module_entity(struct whisker_ecs_world *world);

void whisker_ecs_module_event_init(whisker_ecs *ecs, whisker_ecs_pool *entity_pool)
{
	// set the pool component
	whisker_ecs_module_event_set_pool(ecs, entity_pool);

	whisker_ecs_register_system(ecs, whisker_ecs_module_event_system_cull_event_components, "w_module_event_cull_events", WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_THREADED_AUTO);
	whisker_ecs_register_system(ecs, whisker_ecs_module_event_system_cull_events, "w_module_event_cull_event_data", WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_THREADED_AUTO);

	// disable propagation of changes to ensure that events don't trigger new
	// events
	entity_pool->propagate_component_changes = false;

	// set the event components for the prototype entity
	whisker_ecs_p_set_prototype_named_tag(entity_pool, w_module_event);
	struct whisker_ecs_module_event_cull_event_component cull = {0};
	whisker_ecs_p_set_prototype_named_component(entity_pool, w_module_event_cull_data, cull, &cull);
}

static void whisker_ecs_module_event_set_pool(whisker_ecs *ecs, whisker_ecs_pool *entity_pool)
{
	struct whisker_ecs_module_event_pool_component pool_component = {
		.pool = entity_pool,
	};
	whisker_ecs_set_component(ecs->world, whisker_ecs_module_event_get_pool_entity(ecs->world), sizeof(pool_component), whisker_ecs_module_event_get_module_entity(ecs->world), &pool_component);
}

static whisker_ecs_pool *whisker_ecs_module_event_get_pool(struct whisker_ecs_world *world)
{
	struct whisker_ecs_module_event_pool_component *pool_component = whisker_ecs_get_component(world, whisker_ecs_module_event_get_pool_entity(world), whisker_ecs_module_event_get_module_entity(world));
	whisker_ecs_pool *pool = pool_component->pool;

	return pool;
}

static whisker_ecs_entity_id whisker_ecs_module_event_get_pool_entity(struct whisker_ecs_world *world)
{
	return whisker_ecs_create_named_entity(world, "w_module_event_pool");
}
static whisker_ecs_entity_id whisker_ecs_module_event_get_module_entity(struct whisker_ecs_world *world)
{
	return whisker_ecs_create_named_entity(world, "w_module_event");
}


// create an entity to represent an event
whisker_ecs_entity_id whisker_ecs_module_event_create_event(struct whisker_ecs_world *world)
{
	whisker_ecs_pool *pool = whisker_ecs_module_event_get_pool(world);
	whisker_ecs_entity_id ev = whisker_ecs_p_request_entity(pool);

	pool->world->entities->entities[ev.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(pool->world->entities, (whisker_ecs_entity_deferred_action){.id = ev, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	// set the t_event component on the entity
	whisker_ecs_module_event_set_data_f(world, ev, whisker_ecs_e_create_named(pool->world->entities, "w_module_event"), sizeof(bool), &(bool){0});

	// make the entity unmanaged
	whisker_ecs_e_make_unmanaged(pool->world->entities, ev);

	return ev;
}

// create an event without attached data
whisker_ecs_entity_id whisker_ecs_module_event_create_f(struct whisker_ecs_world *world, whisker_ecs_entity_id event_component_id)
{
	whisker_ecs_entity_id ev = whisker_ecs_module_event_create_event(world);
	whisker_ecs_module_event_set_data_f(world, ev, event_component_id, 0, &(bool){0});
	return ev;
}

// create an event with attached data
whisker_ecs_entity_id whisker_ecs_module_event_create_with_data_f(struct whisker_ecs_world *world, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data)
{
	whisker_ecs_entity_id ev = whisker_ecs_module_event_create_event(world);
	whisker_ecs_module_event_set_data_f(world, ev, event_component_id, component_size, event_data);
	return ev;
}

// create an event entity and immediately fire it
void whisker_ecs_module_event_create_and_fire_f(struct whisker_ecs_world *world, whisker_ecs_entity_id event_component_id)
{
	whisker_ecs_entity_id ev = whisker_ecs_module_event_create_event(world);
	whisker_ecs_module_event_set_data_f(world, ev, event_component_id, sizeof(bool), &(bool){0});
	whisker_ecs_module_event_fire(world, ev);
}

// set component data on an event entity
void whisker_ecs_module_event_set_data_f(struct whisker_ecs_world *world, whisker_ecs_entity_id event_entity_id, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data)
{
	whisker_ecs_c_create_deferred_action(world->components, event_component_id, event_entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET, event_data, component_size, false);
}

// fire off an event entity
void whisker_ecs_module_event_fire(struct whisker_ecs_world *world, whisker_ecs_entity_id event_entity_id)
{
	// firing an event is just making it managed again
	whisker_ecs_e_make_managed(world->entities, event_entity_id);
}

// fire an event, attaching it instead to the given entity ID
void whisker_ecs_module_event_fire_on_f(struct whisker_ecs_world *world, whisker_ecs_entity_id event_component_id, whisker_ecs_entity_id fire_on_entity_id)
{
	// add the event to the provided entity ID
	whisker_ecs_module_event_set_fire_on_data_f(world, event_component_id, sizeof(bool), &(bool){0}, fire_on_entity_id);
}

// fire an event, attaching it with data to the given entity ID
void whisker_ecs_module_event_fire_on_with_data_f(struct whisker_ecs_world *world, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data, whisker_ecs_entity_id fire_on_entity_id)
{
	whisker_ecs_module_event_set_fire_on_data_f(world, event_component_id, component_size, event_data, fire_on_entity_id);
}

// set data on an entity, will be removed along with the event
void whisker_ecs_module_event_set_fire_on_data_f(struct whisker_ecs_world *world, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data, whisker_ecs_entity_id fire_on_entity_id)
{
	whisker_ecs_module_event_set_data_f(world, fire_on_entity_id, event_component_id, component_size, event_data);

	// make sure the event data is culled for this event
	struct whisker_ecs_module_event_cull_event_component cull_component = {
		.entity_id = fire_on_entity_id,
		.component_id = event_component_id,
	};

	// create a data cull event for this entity to remove the fire_on data
	whisker_ecs_entity_id cull_ev = whisker_ecs_module_event_create_with_data_f(world, whisker_ecs_e_create_named(world->entities, "w_module_event_cull_data"), sizeof(cull_component), &cull_component);
	whisker_ecs_module_event_fire(world, cull_ev);
}

/*****************
*  ECS systems  *
*****************/

// system interested in t_event entities
// the system should be added to the last executed phase to ensure that all
// event entities are destroyed and not picked up twice
void whisker_ecs_module_event_system_cull_events(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, STR(w_module_event), "", "");
	while (whisker_ecs_s_iterate(itor)) 
	{
		/* debug_log(DEBUG, ecs:system_cull_events, "culling event entity %zu", itor->entity_id.id); */

		whisker_ecs_c_create_deferred_action(context->world->components, itor->component_ids_rw[0], itor->entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE, NULL, 0, false);

		whisker_ecs_e_destroy_deferred(context->world->entities, itor->entity_id);
	}
}

// companion system to the event cull system.
// this system will accept components of type struct whisker_ecs_module_eventent_cull_event_component
// each entity is a request to issue a deferred removal of component data.
// this is because, in the case of fire_on, we don't have a t_event component
// and we don't want to cull the actual entity.
void whisker_ecs_module_event_system_cull_event_components(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, STR(w_module_event_cull_data), "", "");
	while (whisker_ecs_s_iterate(itor)) 
	{
		// issue the deferred component removal request
		struct whisker_ecs_module_event_cull_event_component *cull = whisker_ecs_itor_get(itor, 0);

		if (cull->component_id.id > 0 && cull->entity_id.id > 0)
		{
			/* debug_log(DEBUG, ecs:system_cull_events, "event: culling fire_on component %s (%zu) from entity %zu", whisker_ecs_e(context->world->entities, cull->component_id)->name, cull->component_id.id, cull->entity_id.id); */

			whisker_ecs_c_create_deferred_action(context->world->components, cull->component_id, cull->entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE, NULL, 0, false);
		}
		whisker_ecs_c_create_deferred_action(context->world->components, itor->component_ids_rw[0], itor->entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE, NULL, 0, false);
	}
}
