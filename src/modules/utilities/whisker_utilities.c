/**
 * @author      : ElGatoPanzon
 * @file        : whisker_utilities
 * @created     : Saturday Mar 07, 2026 12:49:55 CST
 * @description : Utility systems and components
 */

#include "whisker_utilities.h"
#include "whisker_utilities_entity_lifecycle.h"

void wm_utils_init(struct w_ecs_world *world)
{
	wm_utils_timer_update_system_register(world);
	wm_utils_oscillator_update_system_register(world);
	wm_utils_random_update_system_register(world);

	// register entity lifecycle hooks
	w_ecs_register_update_hook(world, W_WORLD_HOOK_UPDATE_BEGIN, w_entity_lifecycle_destroy_end_of_frame_hook_);
	w_ecs_register_update_hook(world, W_WORLD_HOOK_UPDATE_TIMESTEP_BEGIN, w_entity_lifecycle_destroy_end_of_fixed_frame_hook_);
	w_ecs_register_update_hook(world, W_WORLD_HOOK_UPDATE_PHASE_BEGIN, w_entity_lifecycle_destroy_end_of_phase_hook_);
	w_ecs_register_update_hook(world, W_WORLD_HOOK_UPDATE_BEGIN, w_entity_lifecycle_destroy_lifetime_expired_hook_);
	w_ecs_register_update_hook(world, W_WORLD_HOOK_UPDATE_BEGIN, w_entity_lifecycle_cleanup_created_this_frame_tag_hook_);

	// register entity create hook to set created_this_frame_t tag
	w_ecs_register_entity_create_hook(world, w_entity_lifecycle_set_created_this_frame_hook_);
}

void wm_utils_free(struct w_ecs_world *world)
{
	(void)world;
}
