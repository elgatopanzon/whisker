/**
 * @author      : ElGatoPanzon
 * @file        : whisker_assets
 * @created     : Monday May 18, 2026 13:07:57 CST
 * @description : Assets module using the Resources module
 */

#include "whisker_assets.h"
#include "whisker_assets_systems.h"

void wm_assets_init(struct w_ecs_world *world)
{
	// sync resources to assets systems
	wm_assets_lifecycle_sync_hot_resource_to_warm_asset_register(world);
	wm_assets_lifecycle_sync_cold_resource_to_cold_asset_register(world);

	// base handlers for asset requests
	wm_assets_lifecycle_handle_request_asset_hot_register(world);
	wm_assets_lifecycle_handle_request_asset_warm_register(world);
	wm_assets_lifecycle_handle_request_asset_cold_register(world);

	// cleanup assets requests
	wm_assets_life_handle_stray_request_asset_hot_register(world);
	wm_assets_life_handle_stray_request_asset_cold_register(world);
}

void wm_assets_free(struct w_ecs_world *world)
{
	(void)world;
}
