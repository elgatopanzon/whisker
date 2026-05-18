/**
 * @author      : ElGatoPanzon
 * @file        : whisker_resources
 * @created     : Sunday May 17, 2026 15:49:12 CST
 * @description : Resources module providing file loading with cold/hot states
 */

#include "whisker_resources.h"
#include "whisker_resources_systems.h"

void wm_resources_init(struct w_ecs_world *world)
{
	// register systems
	wm_resources_lifecycle_ensure_resources_valid_register(world);
	wm_resources_lifecycle_handle_hot_request_register(world);
	wm_resources_lifecycle_handle_cold_request_register(world);
	wm_resources_lifecycle_handle_stale_data_handles_register(world);
}

void wm_resources_free(struct w_ecs_world *world)
{
	(void)world;
}
