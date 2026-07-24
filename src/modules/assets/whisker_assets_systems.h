/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_assets_systems
 * @created     : Monday May 18, 2026 13:09:31 CST
 * @description : systems for the assets module
 */

#include "whisker_assets.h"

#ifndef WHISKER_ASSETS_SYSTEMS_H
#define WHISKER_ASSETS_SYSTEMS_H

// sync hot resource tags to asset warm tags
w_ecs_system(
	wm_assets_lifecycle_sync_hot_resource_to_warm_asset,
	WM_PHASE_POST_LOAD,
	w_query(
		// pick up hot resources to mark them as warm assets
		w_query_h(resource_hot),
		w_query_n(asset_warm),
	),
{
	w_set_tag(entity, asset_warm, true);
	w_set_tag(entity, asset_hot_pending_, true);
});

// sync cold resource tags to remove asset warm tags
w_ecs_system(
	wm_assets_lifecycle_sync_cold_resource_to_cold_asset,
	WM_PHASE_POST_LOAD,
	w_query(
		// pick up cold resources to remove them from warm assets
		w_query_n(resource_hot),
		w_query_h(asset_warm),
		// ignore if theres already a cold request
		w_query_n(req_asset_cold),
	),
{
	// since resource is going cold, we request the asset to go cold
	// this triggers the implementation's unloading for this asset (which will remove asset_hot)
	w_set_tag(entity, req_asset_cold, true);
	w_set_tag(entity, asset_warm, false);
});

// handle request asset hot
// we dont load the asset, we just ensure the resource is hot
w_ecs_system(
	wm_assets_lifecycle_handle_request_asset_hot,
	WM_PHASE_PRE_LOAD,
	w_query(
		// pick up hot asset request with cold resources
		w_query_h(req_asset_hot),
		w_query_n(resource_hot),

		// skip existing hot resource requests
		w_query_n(req_resource_hot),
	),
{
	w_set_tag(entity, req_resource_hot, true);
});

// handle request asset warm
w_ecs_system(
	wm_assets_lifecycle_handle_request_asset_warm,
	WM_PHASE_PRE_LOAD,
	w_query(
		// pick up warm asset request
		w_query_h(req_asset_warm),
		w_query_n(resource_hot),
		w_query_n(asset_warm),

		// skip existing hot resource requests
		w_query_n(req_resource_hot),
	),
{
	// handle cold resources
	w_set_tag(entity, req_resource_hot, true);

	// note: we dont handle the downgrade here
	// the module that implements the asset picks up this request and handles that
});

// handle cold asset request
w_ecs_system(
	wm_assets_lifecycle_handle_request_asset_cold,
	WM_PHASE_PRE_LOAD,
	w_query(
		w_query_h(req_asset_cold),
		w_query_h(resource_hot),

		// skip existing cold resource requests
		w_query_n(req_resource_cold),
	),
{
	// handle cold resource request
	w_set_tag(entity, req_resource_cold, true);
});

// pick up stray asset hot requests and mark failed
w_ecs_system(
	wm_assets_life_handle_stray_request_asset_hot,
	WM_PHASE_POST,
	w_query(
		w_query_h(req_asset_hot),
		w_query_n(asset_load_failed),
	),
{
	if (w_tag_exists(entity, asset_hot_pending_))
	{
		w_set_tag(entity, asset_hot_pending_, false);
		continue;
	}

	w_set_value(entity, asset_load_failed, WM_ASSETS_LOAD_FAILED_MISSING_LOADER);
	w_set_tag(entity, req_asset_hot, false);
});
// pick up stray asset cold requests and remove
w_ecs_system(
	wm_assets_life_handle_stray_request_asset_cold,
	WM_PHASE_POST,
	w_query(
		w_query_h(req_asset_cold),
		w_query_n(asset_load_failed),
	),
{
	w_set_tag(entity, req_asset_cold, false);
});

#endif /* WHISKER_ASSETS_SYSTEMS_H */
