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

#define w_ecs_define_event(name) \
	w_ecs_define_component_typedef(bool, name, ); \
	w_ecs_define_tag_impl_(name) \

// fire an event: creates entity, sets component 'name', marks for destroy_end_of_frame
// returns: w_entity_id of the event entity
#define w_event_fire(w, name) \
({ \
	(void)sizeof(name); \
	w_entity_id _e = w_ecs_request_entity(w); \
	name##_set_tag_state(w, _e, true); \
	w_entity_destroy_end_of_frame(w, _e); \
	_e; \
})

// set data component on event entity with combined name (name_data_name)
#define w_event_set_data(w, e, name, data_name, data) \
do { \
	(void)sizeof(name); \
	(void)sizeof(data_name); \
	data_name##_set_generic(w, e, name##_name_, &data); \
} while(0)

// fire event on existing entity: sets component 'name', queues for cleanup at end of frame
#define w_event_fire_on(w, e, name) \
do { \
	(void)sizeof(name); \
	name##_set_tag_state(w, e, true); \
	struct wm_events_state *_state = wm_events_get_state(w); \
	w_array_ensure_alloc_block_size(_state->removal_buffer, _state->removal_buffer_length + 1, WM_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE); \
	_state->removal_buffer[_state->removal_buffer_length++] = (w_pack32x2){ .left = (e), .right = (name##_component_id_) }; \
} while(0)

// set data on event fired on existing entity (queues data component for cleanup too)
#define w_event_set_data_on(w, e, name, data_name, data) \
do { \
	(void)sizeof(name); \
	(void)sizeof(data_name); \
	data_name##_set_generic(w, e, name##_name_, &data); \
	w_entity_id _evt_data_comp_id = data_name##_get_generic_id(w, name##_name_); \
	struct wm_events_state *_state = wm_events_get_state(w); \
	w_array_ensure_alloc_block_size(_state->removal_buffer, _state->removal_buffer_length + 1, WM_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE); \
	_state->removal_buffer[_state->removal_buffer_length++] = (w_pack32x2){ .left = (e), .right = (_evt_data_comp_id) }; \
} while(0)


#endif /* WHISKER_EVENTS_H */
