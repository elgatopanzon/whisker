/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_component_events_hooks
 * @created     : Tuesday Apr 28, 2026 14:39:37 CST
 * @description : 
 */

#include "whisker_component_events.h"

#ifndef WHISKER_COMPONENT_EVENTS_HOOKS_H
#define WHISKER_COMPONENT_EVENTS_HOOKS_H

w_ecs_update_hook(component_events_cleanup, BEGIN, {
	struct wm_component_events_registry *reg = wm_component_events_get_registry(world);
	if (!reg) return;

	// index-based loop: length can grow during iteration
	for (size_t i = 0; i < reg->removal_buffer_length; i++)
	{
		w_pack32x2 pair = reg->removal_buffer[i];
		w_entity_id owner = pair.left;
		w_entity_id tag_id = pair.right;
		w_component_remove(&world->components, tag_id, owner);
	}

	reg->removal_buffer_length = 0;
});

#endif /* WHISKER_COMPONENT_EVENTS_HOOKS_H */

