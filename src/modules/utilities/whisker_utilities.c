/**
 * @author      : ElGatoPanzon
 * @file        : whisker_utilities
 * @created     : Saturday Mar 07, 2026 12:49:55 CST
 * @description : Utility systems and components
 */

#include "whisker_utilities.h"

void wm_utils_init(struct w_ecs_world *world)
{
	wm_utils_timer_update_system_register(world);
	wm_utils_oscillator_update_system_register(world);
	wm_utils_random_update_system_register(world);
}

void wm_utils_free(struct w_ecs_world *world)
{
	(void)world;
}
