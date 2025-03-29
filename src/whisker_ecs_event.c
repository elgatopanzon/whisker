/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_event
 * @created     : Friday Mar 28, 2025 13:20:43 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_event.h"

// create an entity to represent an event
whisker_ecs_entity_id whisker_ecs_ev_create_event(whisker_ecs_entities *entities, whisker_ecs_components *components)
{
	whisker_ecs_entity_id ev = whisker_ecs_e_create(entities);

	// set the entity to dead and add it to the deferred entities
	entities->entities[ev.index].destroyed = true;
	whisker_ecs_e_add_deffered_action(entities, (whisker_ecs_entity_deferred_action){.id = ev, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_CREATE});

	// set the t_event component on the entity
	whisker_ecs_ev_set_data_f(entities, components, ev, whisker_ecs_e_create_named(entities, "t_event"), 0, NULL);

	// make the entity unmanaged
	whisker_ecs_e_make_unmanaged(entities, ev);

	return ev;
}

// create an event without attached data
whisker_ecs_entity_id whisker_ecs_ev_create_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id)
{
	whisker_ecs_entity_id ev = whisker_ecs_ev_create_event(entities, components);
	whisker_ecs_ev_set_data_f(entities, components, ev, event_component_id, 0, NULL);
	return ev;
}

// create an event with attached data
whisker_ecs_entity_id whisker_ecs_ev_create_with_data_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data)
{
	whisker_ecs_entity_id ev = whisker_ecs_ev_create_event(entities, components);
	whisker_ecs_ev_set_data_f(entities, components, ev, event_component_id, component_size, event_data);
	return ev;
}

// create an event entity and immediately fire it
void whisker_ecs_ev_create_and_fire_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id)
{
	whisker_ecs_entity_id ev = whisker_ecs_ev_create_event(entities, components);
	whisker_ecs_ev_set_data_f(entities, components, ev, event_component_id, 0, NULL);
	whisker_ecs_ev_fire(entities, ev);
}

// set component data on an event entity
void whisker_ecs_ev_set_data_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_entity_id, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data)
{
	whisker_ecs_c_create_deferred_action(components, event_component_id, event_entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_SET, event_data, component_size);
}

// fire off an event entity
void whisker_ecs_ev_fire(whisker_ecs_entities *entities, whisker_ecs_entity_id event_entity_id)
{
	// firing an event is just making it managed again
	whisker_ecs_e_make_managed(entities, event_entity_id);
}

// fire an event, attaching it instead to the given entity ID
void whisker_ecs_ev_fire_on_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id, whisker_ecs_entity_id fire_on_entity_id)
{
	// add the event to the provided entity ID
	whisker_ecs_ev_set_fire_on_data_f(entities, components, event_component_id, 0, NULL, fire_on_entity_id);
}

// fire an event, attaching it with data to the given entity ID
void whisker_ecs_ev_fire_on_with_data_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data, whisker_ecs_entity_id fire_on_entity_id)
{
	whisker_ecs_ev_set_fire_on_data_f(entities, components, event_component_id, component_size, event_data, fire_on_entity_id);
}

// set data on an entity, will be removed along with the event
void whisker_ecs_ev_set_fire_on_data_f(whisker_ecs_entities *entities, whisker_ecs_components *components, whisker_ecs_entity_id event_component_id, size_t component_size, void *event_data, whisker_ecs_entity_id fire_on_entity_id)
{
	whisker_ecs_ev_set_data_f(entities, components, fire_on_entity_id, event_component_id, component_size, event_data);

	// make sure the event data is culled for this event
	struct whisker_ecs_event_cull_event_component cull_component = {
		.entity_id = fire_on_entity_id,
		.component_id = event_component_id,
	};

	// create a data cull event for this entity to remove the fire_on data
	whisker_ecs_entity_id cull_ev = whisker_ecs_ev_create_with_data_f(entities, components, whisker_ecs_e_create_named(entities, "t_event_cull_data"), sizeof(cull_component), &cull_component);
	whisker_ecs_ev_fire(entities, cull_ev);
}

/*****************
*  ECS systems  *
*****************/

// system interested in t_event entities
// the system should be added to the last executed phase to ensure that all
// event entities are destroyed and not picked up twice
void whisker_ecs_ev_system_cull_events(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_event", "", "");
	while (whisker_ecs_s_iterate(itor)) 
	{
		debug_log(DEBUG, ecs:system_cull_events, "event: culling event entity %zu", itor->entity_id.id);
		whisker_ecs_e_add_deffered_action(context->entities, (whisker_ecs_entity_deferred_action){.id = itor->entity_id, .action = WHISKER_ECS_ENTITY_DEFERRED_ACTION_DESTROY});
	}
}

// companion system to the t_event cull system.
// this system will accept components of type struct whisker_ecs_event_cull_event_component
// each entity is a request to issue a deferred removal of component data.
// this is because, in the case of fire_on, we don't have a t_event component
// and we don't want to cull the actual entity.
void whisker_ecs_ev_system_cull_event_components(whisker_ecs_system_context *context)
{
	whisker_ecs_system_iterator *itor = whisker_ecs_s_get_iterator(context, 0, "t_event_cull_data", "", "");
	while (whisker_ecs_s_iterate(itor)) 
	{
		// issue the deferred component removal request
		struct whisker_ecs_event_cull_event_component *cull = whisker_ecs_itor_get(itor, 0);

		debug_log(DEBUG, ecs:system_cull_events, "event: culling fire_on component %zu from entity %zu", cull->component_id.id, cull->entity_id.id);

		whisker_ecs_c_create_deferred_action(context->components, cull->component_id, cull->entity_id, WHISKER_ECS_COMPONENT_DEFERRED_ACTION_REMOVE, NULL, 0);
	}
}
