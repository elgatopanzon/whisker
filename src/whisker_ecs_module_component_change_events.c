/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_module_component_change_events
 * @created     : Monday Mar 31, 2025 12:21:41 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_module_component_change_events.h"

void wm_component_change_events_init(struct w_ecs *ecs)
{
	// register the system
	w_register_system(
		ecs, 
		wm_component_change_events_system, 
		STR(wm_component_change_events_system),
		W_PHASE_POST_PHASE_,
		W_SCHED_THREAD_MAIN_THREAD
	);
}

void wm_component_change_events_system(struct w_sys_context *context)
{
	// use deferred actions to create component events
	if (context->world->components->deferred_actions_length > 0) 
	{
		for (int i = 0; i < context->world->components->deferred_actions_length; ++i)
		{
			struct w_component_deferred_action *action = &context->world->components->deferred_actions[i];

			// holds the name of the event to create
			char* event_name; 
			char* event_name_target; 
			char* component_name = w_get_entity(context->world, action->component_id)->name;

			// skip actions with propagate disabled or any component containing
			if (!action->propagate || (component_name != NULL && strstr(component_name, "ev_") != NULL))
			{
				continue;
			}

			// the string "ev_" which is assumed to be an event
			// HACK: not sure how else to do this
			switch (action->action) {
				case W_COMPONENT_DEFERRED_ACTION_SET:
					// determine if this is an add event or a changed event
					if (!w_has_component(context->world, action->component_id, action->entity_id))
					{
						asprintf(&event_name, "%s_ev_added", component_name);
						asprintf(&event_name_target, "%s_ev_added_to", component_name);
					}
					else
					{
						// compare the existing value to the new value, overwise
						// skip this action
						if (memcmp(context->world->components->deferred_actions_data + action->data_offset, w_sparse_set_get(context->world->components->components[action->component_id.index], action->entity_id.index), action->data_size) == 0)
						{
							continue;
						}

						// if the memory is different consider it changed
						asprintf(&event_name, "%s_ev_changed", component_name);
						asprintf(&event_name_target, "%s_ev_changed_on", component_name);
					}
					break;

				case W_COMPONENT_DEFERRED_ACTION_REMOVE:
					asprintf(&event_name, "%s_ev_removed", component_name);
					asprintf(&event_name_target, "%s_ev_removed_from", component_name);
					break;

				case W_COMPONENT_DEFERRED_ACTION_DUMMY_ADD:
					asprintf(&event_name, "%s_ev_added", component_name);
					asprintf(&event_name_target, "%s_ev_added_to", component_name);
					break;
				case W_COMPONENT_DEFERRED_ACTION_DUMMY_REMOVE:
					asprintf(&event_name, "%s_ev_removed", component_name);
					asprintf(&event_name_target, "%s_ev_removed_from", component_name);
					break;
				default:
					continue;
					break;
			}

			// create and fire event on the entity
			w_entity_id event_entity = w_create_named_entity(context->world, event_name);
			w_entity_id event_entity_target = w_create_named_entity(context->world, event_name_target);


			// first fire the targetted event as a new event
			w_entity_id event_target_entity = action->entity_id;
			w_entity_id ev = wm_event_create_with_data(context->world, event_entity_target, w_entity_id, &event_target_entity);
			wm_event_fire(context->world, ev);

			/* printf("deferred component events: creating %s (%zu as %zu) event on entity %zu\n", event_name, event_entity, ev, action->entity_id); */

			// fire the normal event directly on the entity itself
			// note: events fired on the entity don't survive the entities
			// destruction
			wm_event_fire_on_f(context->world, event_entity, event_target_entity);

			free(event_name);
			free(event_name_target);
		}
	}
}
