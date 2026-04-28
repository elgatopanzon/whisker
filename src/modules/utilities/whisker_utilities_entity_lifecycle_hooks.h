/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_entity_lifecycle_hooks
 * @created     : Tuesday Apr 28, 2026 13:26:11 CST
 * @description : entity lifecycle module hooks
 */

#include "whisker.h"
#include "whisker_utilities_entity_lifecycle.h"

#ifndef WHISKER_UTILITIES_ENTITY_LIFECYCLE_HOOKS_H
#define WHISKER_UTILITIES_ENTITY_LIFECYCLE_HOOKS_H

// macro to define lifecycle hook functions
/* #define W_ENTITY_LIFECYCLE_HOOK(name, query, code) \ */
/* 	static inline void w_entity_lifecycle_##name##_hook_(void *world_, void *action_) \ */
/* 	{ \ */
/* 		(void)action_; \ */
/* 		struct w_ecs_world *world = world_; \ */
/* 		w_query_for_each( \ */
/* 			world,  \ */
/* 			query, \ */
/* 		{ \ */
/* 			{ code }; \ */
/* 		}); \ */
/* 	} \ */
/*  */
#define W_ENTITY_LIFECYCLE_HOOK(name, subtype, query, work) \
	w_ecs_update_hook(name##_lifecycle_hook, subtype, { \
		w_query_for_each( \
			world,  \
			query, \
		{ \
			{ work }; \
		}); \
	}) \

// destroy entities marked for end-of-frame destruction
W_ENTITY_LIFECYCLE_HOOK(
	destroy_end_of_frame,
	BEGIN,
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_FRAME_TAG),
{
	w_ecs_return_entity(world, itor.entity_id);
});

// destroy entities marked for end-of-phase destruction
W_ENTITY_LIFECYCLE_HOOK(
	destroy_end_of_phase,
	PHASE_BEGIN,
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_PHASE_TAG),
{
	w_ecs_return_entity(world, itor.entity_id);
});

// destroy entities marked for end-of-fixed-frame destruction
W_ENTITY_LIFECYCLE_HOOK(
	destroy_end_of_fixed_frame,
	TIMESTEP_BEGIN,
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_TAG)
	w_query_read(W_ENTITY_LIFECYCLE_DESTROY_END_OF_FIXED_FRAME_TAG),
{
	w_ecs_return_entity(world, itor.entity_id);
});

// remove created_this_frame_t tag from all entities that have it
W_ENTITY_LIFECYCLE_HOOK(
	cleanup_created_this_frame_tag,
	BEGIN,
	w_query_read(W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG),
{
	w_ecs_remove_str(world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG, itor.entity_id);
});

// destroy entities whose lifetime timer has finished
W_ENTITY_LIFECYCLE_HOOK(
	destroy_lifetime_expired,
	BEGIN,
	w_query_read(W_ENTITY_LIFECYCLE_LIFETIME_FINISHED_TAG),
{
	w_ecs_return_entity(world, itor.entity_id);
});

// entity create hook: sets created_this_frame_t tag on newly created entities
w_ecs_entity_create_hook(set_created_this_frame_hook, {
	w_ecs_set_tag(world, w_ecs_get_component_by_name(world, W_ENTITY_LIFECYCLE_CREATED_THIS_FRAME_TAG), *entity);
});

#endif /* WHISKER_UTILITIES_ENTITY_LIFECYCLE_HOOKS_H */

