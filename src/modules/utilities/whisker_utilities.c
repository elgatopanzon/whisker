/**
 * @author      : ElGatoPanzon
 * @file        : whisker_utilities
 * @created     : Saturday Mar 07, 2026 12:49:55 CST
 * @description : Utility systems and components
 */

#include "whisker_utilities.h"
#include "whisker_utilities_timer.h"
#include "whisker_utilities_timer_systems.h"
#include "whisker_utilities_oscillator.h"
#include "whisker_utilities_oscillator_systems.h"
#include "whisker_utilities_random.h"
#include "whisker_utilities_random_systems.h"
#include "whisker_utilities_entity_lifecycle_hooks.h"

void wm_utils_init(struct w_ecs_world *world)
{
	wm_utils_timer_update_system_register(world);
	wm_utils_oscillator_update_system_register(world);
	wm_utils_random_update_system_register(world);

	// register entity lifecycle hooks
	destroy_end_of_frame_lifecycle_hook_register(world);
	destroy_end_of_fixed_frame_lifecycle_hook_register(world);
	destroy_end_of_phase_lifecycle_hook_register(world);
	destroy_lifetime_expired_lifecycle_hook_register(world);
	cleanup_created_this_frame_tag_lifecycle_hook_register(world);

	// register entity create hook to set created_this_frame_t tag
	set_created_this_frame_hook_register(world);
}

void wm_utils_free(struct w_ecs_world *world)
{
	(void)world;
}
