/**
 * @author      : ElGatoPanzon
 * @file        : whisker_events
 * @created     : Wednesday Apr 01, 2026 16:56:23 CST
 * @description : Fire-and-forget event components with automatic cleanup
 */

#ifndef WHISKER_EVENTS_H
#define WHISKER_EVENTS_H

#include "whisker.h"
#include "modules/utilities/whisker_utilities_entity_lifecycle.h"

/*****************************
 *  config                   *
 *****************************/

#include "modules/whisker_module_ids.h"

// module resource ID for event state
#define WM_EVENTS_MODULE_RESOURCE_ID WM_MODULE_RESOURCE_ID(EVENTS, 0)

#ifndef WM_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE
#define WM_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE 128
#endif

/*****************************
 *  data structures          *
 *****************************/

// module state for event cleanup tracking
struct wm_events_state
{
	struct w_arena *arena;

	// deferred removal buffer: pairs of {owner_entity, component_id}
	w_array_declare(w_pack32x2, removal_buffer);
};


/*****************************
 *  module API               *
 *****************************/

// initialize the events module
void wm_events_init(struct w_ecs_world *world);

// cleanup the events module
void wm_events_free(struct w_ecs_world *world);

// get module state
struct wm_events_state *wm_events_get_state(struct w_ecs_world *world);


/*****************************
 *  macros                   *
 *****************************/

// fire an event: creates entity, sets component 'name', marks for destroy_end_of_frame
// returns: w_entity_id of the event entity
#define w_event_fire(w, name) \
({ \
	w_entity_id _e = w_ecs_request_entity(w); \
	w_ecs_set_tag_str(w, name, _e); \
	w_entity_destroy_end_of_frame(w, _e); \
	_e; \
})

// set data component on event entity with combined name (name_data_name)
#define w_event_set_data(w, e, name, type, data_name, data) \
do { \
	char _comp_name[256]; \
	snprintf(_comp_name, sizeof(_comp_name), "%s_%s", name, data_name); \
	w_ecs_set_str(w, type, _comp_name, e, data); \
} while(0)

// fire event on existing entity: sets component 'name', queues for cleanup at end of frame
#define w_event_fire_on(w, e, name) \
do { \
	w_entity_id _comp_id = w_ecs_get_component_by_name(w, name); \
	w_ecs_set_tag(w, _comp_id, e); \
	struct wm_events_state *_state = wm_events_get_state(w); \
	w_array_ensure_alloc_block_size(_state->removal_buffer, _state->removal_buffer_length + 1, WM_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE); \
	_state->removal_buffer[_state->removal_buffer_length++] = (w_pack32x2){ .left = (e), .right = (_comp_id) }; \
} while(0)

// fire event on existing named entity: looks up entity, sets component, queues for cleanup
#define w_event_fire_on_str(w, entity_name, name) \
do { \
	w_entity_id _e = w_ecs_get_entity_by_name(w, entity_name); \
	if (w_ecs_is_valid_entity(_e)) { \
		w_event_fire_on(w, _e, name); \
	} \
} while(0)

// set data on event fired on existing entity (queues data component for cleanup too)
#define w_event_set_data_on(w, e, name, type, data_name, data) \
do { \
	char _comp_name[256]; \
	snprintf(_comp_name, sizeof(_comp_name), "%s_%s", name, data_name); \
	w_entity_id _comp_id = w_ecs_get_component_by_name(w, _comp_name); \
	w_ecs_set_str(w, type, _comp_name, e, data); \
	struct wm_events_state *_state = wm_events_get_state(w); \
	w_array_ensure_alloc_block_size(_state->removal_buffer, _state->removal_buffer_length + 1, WM_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE); \
	_state->removal_buffer[_state->removal_buffer_length++] = (w_pack32x2){ .left = (e), .right = (_comp_id) }; \
} while(0)


/*****************************
 *  cleanup hook             *
 *****************************/

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


#endif /* WHISKER_EVENTS_H */
