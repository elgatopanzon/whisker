/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_events_hooks
 * @created     : Tuesday Apr 28, 2026 14:37:20 CST
 * @description : 
 */

#include "whisker_events.h"

#ifndef WHISKER_EVENTS_HOOKS_H
#define WHISKER_EVENTS_HOOKS_H


// cleanup hook
w_ecs_update_hook(wm_events_cleanup, END, {
	struct wm_events_state *state = wm_events_get_state(world);
	if (!state) return;

	// remove all queued event components
	w_ecs_world_do_unbuffered(world, {
		for (size_t i = 0; i < state->removal_buffer_length; i++)
		{
			w_pack32x2 pair = state->removal_buffer[i];
			w_entity_id owner = pair.left;
			w_entity_id comp_id = pair.right;
			w_component_remove(&world->components, comp_id, owner);
		}
	});

	state->removal_buffer_length = 0;
});


#endif /* WHISKER_EVENTS_HOOKS_H */

