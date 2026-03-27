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
#define W_ENTITY_LIFECYCLE_DESTROY_TAG "destroy_t"
#define W_ENTITY_LIFECYCLE_DESTROY_END_OF_FRAME_TAG "destroy_end_of_frame_t"
#define W_ENTITY_LIFECYCLE_DESTROY_END_OF_FIXED_FRAME_TAG "destroy_end_of_fixed_frame_t"
#define W_ENTITY_LIFECYCLE_DESTROY_END_OF_PHASE_TAG "destroy_end_of_phase_t"

// entity state tags
#define W_ENTITY_LIFECYCLE_DISABLED_TAG "disabled_t"
#define W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG "created_this_frame_t"

// lifetime timer name and finished tag
#define W_ENTITY_LIFECYCLE_LIFETIME_TIMER_NAME "entity_lifetime"
#define W_ENTITY_LIFECYCLE_LIFETIME_FINISHED_TAG W_ENTITY_LIFECYCLE_LIFETIME_TIMER_NAME W_TIMER_COMPONENT_FINISHED


/*****************************
 *  macros                   *
 *****************************
 * convenience macros to mark entities for destruction or state changes
 * destruction macros set both the generic destroy_t tag and a timing-specific tag
 */

// mark entity for destruction at end of frame
#define w_entity_destroy_end_of_frame(w, e) { \
	w_ecs_set_tag(w, w_ecs_get_component_by_name(w, W_ENTITY_LIFECYCLE_DESTROY_TAG), e); \
	w_ecs_set_tag(w, w_ecs_get_component_by_name(w, W_ENTITY_LIFECYCLE_DESTROY_END_OF_FRAME_TAG), e); \
} \

// mark entity for destruction at end of fixed timestep
#define w_entity_destroy_end_of_fixed_frame(w, e) { \
	w_ecs_set_tag(w, w_ecs_get_component_by_name(w, W_ENTITY_LIFECYCLE_DESTROY_TAG), e); \
	w_ecs_set_tag(w, w_ecs_get_component_by_name(w, W_ENTITY_LIFECYCLE_DESTROY_END_OF_FIXED_FRAME_TAG), e); \
} \

// mark entity for destruction at end of phase
#define w_entity_destroy_end_of_phase(w, e) { \
	w_ecs_set_tag(w, w_ecs_get_component_by_name(w, W_ENTITY_LIFECYCLE_DESTROY_TAG), e); \
	w_ecs_set_tag(w, w_ecs_get_component_by_name(w, W_ENTITY_LIFECYCLE_DESTROY_END_OF_PHASE_TAG), e); \
} \

// disable entity (skipped by most queries when filtered)
#define w_entity_set_disabled(w, e) { \
	w_ecs_set_tag(w, w_ecs_get_component_by_name(w, W_ENTITY_LIFECYCLE_DISABLED_TAG), e); \
} \

// re-enable a disabled entity
#define w_entity_set_enabled(w, e) { \
	w_ecs_remove_tag(w, w_ecs_get_component_by_name(w, W_ENTITY_LIFECYCLE_DISABLED_TAG), e); \
} \

// set entity lifetime in seconds using timer utility
// when timer finishes, entity_lifetime_finished_tag is set on the entity
// the lifetime hook then destroys the entity
#define w_entity_set_lifetime(w, e, duration) { \
	w_timer_create(w, e, W_ENTITY_LIFECYCLE_LIFETIME_TIMER_NAME, duration, false, true); \
} \


/*****************************
 *  hooks and systems        *
 *****************************
 * hook functions that run during world update lifecycle to handle entity
 * destruction and tag cleanup. Register these with w_ecs_register_update_hook
 */

// macro to define lifecycle hook functions
#define W_ENTITY_LIFECYCLE_HOOK(name, query, code) \
	static inline void w_entity_lifecycle_##name##_hook_(void *world_, void *action_) \
	{ \
		(void)action_; \
		struct w_ecs_world *world = world_; \
		w_query_for_each( \
			world,  \
			query, \
		{ \
			{ code }; \
		}); \
	} \

// destroy entities marked for end-of-frame destruction
W_ENTITY_LIFECYCLE_HOOK(
	destroy_end_of_frame,
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_FRAME_TAG),
{
	w_ecs_return_entity(world, itor.entity_id);
});

// destroy entities marked for end-of-phase destruction
W_ENTITY_LIFECYCLE_HOOK(
	destroy_end_of_phase,
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_PHASE_TAG),
{
	w_ecs_return_entity(world, itor.entity_id);
});

// destroy entities marked for end-of-fixed-frame destruction
W_ENTITY_LIFECYCLE_HOOK(
	destroy_end_of_fixed_frame,
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_FIXED_FRAME_TAG),
{
	w_ecs_return_entity(world, itor.entity_id);
});

// remove created_this_frame_t tag from all entities that have it
W_ENTITY_LIFECYCLE_HOOK(
	cleanup_created_this_frame_tag,
	w_query_read(W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG),
{
	w_ecs_remove_str(world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, itor.entity_id);
});

// destroy entities whose lifetime timer has finished
W_ENTITY_LIFECYCLE_HOOK(
	destroy_lifetime_expired,
	w_query_read(W_ENTITY_LIFECYCLE_LIFETIME_FINISHED_TAG),
{
	w_ecs_return_entity(world, itor.entity_id);
});

// entity create hook: sets created_this_frame_t tag on newly created entities
static inline void w_entity_lifecycle_set_created_this_frame_hook_(void *world_, void *entity_)
{
	struct w_ecs_world *world = world_;
	w_entity_id *entity = entity_;
	w_ecs_set_tag(world, w_ecs_get_component_by_name(world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG), *entity);
}

#endif /* WHISKER_UTILITIES_ENTITY_LIFECYCLE_H */

