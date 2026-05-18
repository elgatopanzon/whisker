/**
 * @author      : ElGatoPanzon
 * @file        : whisker_assets
 * @created     : Monday May 18, 2026 13:07:57 CST
 * @description : Assets module using the Resources module
 */

#ifndef WHISKER_ASSETS_H
#define WHISKER_ASSETS_H

#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/resources/whisker_resources.h"

enum WM_ASSETS_LOAD_FAILED
{
	WM_ASSETS_LOAD_FAILED_UNKNOWN,
	WM_ASSETS_LOAD_FAILED_MISSING_LOADER,
	WM_ASSETS_LOAD_FAILED_DECODE_FAILED,
	WM_ASSETS_LOAD_FAILED_PARSE_FAILED,
	WM_ASSETS_LOAD_FAILED_INVALID_HEADER,
	WM_ASSETS_LOAD_FAILED_UNSUPPORTED_FORMAT,
};

/****************
*  components  *
****************/

// indicates that the resource backing this asset is hot
w_ecs_define_tag(asset_warm);
// indicates that the asset is fully loaded and usable
// note: this is something implementing modules have to add
w_ecs_define_tag(asset_hot);

// request resource backing the asset to be warm, then loaded
// (implementation specific)
w_ecs_define_tag(req_asset_hot);

// unload from hot when hot, load from cold when cold
w_ecs_define_tag(req_asset_warm);

// fully unload from hot or warm to cold
w_ecs_define_tag(req_asset_cold);

// custom ID indicating an asset failed a parse stage
w_ecs_define_component(int, asset_load_failed);

/************************
*  functions & macros  *
************************/

// initialize the assets module
void wm_assets_init(struct w_ecs_world *world);

// cleanup the assets module
void wm_assets_free(struct w_ecs_world *world);

#endif /* WHISKER_ASSETS_H */
