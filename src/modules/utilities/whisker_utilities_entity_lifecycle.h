/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_entity_lifecycle
 * @created     : Thursday Mar 26, 2026 16:16:35 CST
 * @description : utilities to manage entity lifecycles
 */

#include "whisker.h"
#include "modules/utilities/whisker_utilities_timer.h"

#ifndef WHISKER_UTILITIES_ENTITY_LIFECYCLE_H
#define WHISKER_UTILITIES_ENTITY_LIFECYCLE_H

/*****************************
 *  tags and components      *
 *****************************
 * tag component names used to mark entities for various lifecycle actions
 * tags are zero-size components used to filter entities in queries
 */

// destruction timing tags
w_ecs_define_tag(req_destroy);
w_ecs_define_tag(req_destroy_end_of_frame);
w_ecs_define_tag(req_destroy_end_of_fixed_frame);
w_ecs_define_tag(req_destroy_end_of_phase);

// entity state tags
w_ecs_define_tag(disabled)
w_ecs_define_tag(created_this_frame)

// timer tag added when lifecycle timer ends
w_ecs_define_tag(entity_lifetime_timer_finished);

/*****************************
 *  macros                   *
 *****************************
 * convenience macros to mark entities for destruction or state changes
 * destruction macros set both the generic destroy_t tag and a timing-specific tag
 */

// mark entity for destruction at end of frame
#define w_entity_destroy_end_of_frame(w, e) { \
	req_destroy_set_tag_state(w, e, true); \
	req_destroy_end_of_frame_set_tag_state(w, e, true); \
} \

// mark entity for destruction at end of fixed timestep
#define w_entity_destroy_end_of_fixed_frame(w, e) { \
	req_destroy_set_tag_state(w, e, true); \
	req_destroy_end_of_fixed_frame_set_tag_state(w, e, true); \
} \

// mark entity for destruction at end of phase
#define w_entity_destroy_end_of_phase(w, e) { \
	req_destroy_set_tag_state(w, e, true); \
	req_destroy_end_of_phase_set_tag_state(w, e, true); \
} \

// disable entity (skipped by most queries when filtered)
#define w_entity_set_disabled(w, e) { \
	disabled_set_tag_state(w, e, true); \
} \

// re-enable a disabled entity
#define w_entity_set_enabled(w, e) { \
	disabled_set_tag_state(w, e, false); \
} \

// set entity lifetime in seconds using timer utility
// when timer finishes, entity_lifetime_finished_tag is set on the entity
// the lifetime hook then destroys the entity
#define w_entity_set_lifetime(w, e, duration) { \
	wm_utils_timer_create(w, e, "entity_lifetime", duration, false, true); \
} \

#endif /* WHISKER_UTILITIES_ENTITY_LIFECYCLE_H */

