/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_camera
 * @created     : Monday Apr 06, 2026 18:24:15 CST
 */

#include "whisker_std.h"

#include "whisker_rendering_camera.h"


/*************
*  systems  *
*************/

w_ecs_system(
	camera_request_active_camera,
	WM_PHASE_PRE_RENDER,
	w_query_read(W_RENDERING_CAMERA_TAG)
	w_query_read(W_RENDERING_CAMERA_REQUEST_ACTIVATE_CAMERA),
{
	struct w_rendering_camera_state *camera_state = w_ecs_get_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID);

	camera_state->camera_entity_id = itor.entity_id;

	// remove request tag after processing to prevent repeated activation
	w_ecs_remove_tag_str(world, W_RENDERING_CAMERA_REQUEST_ACTIVATE_CAMERA, itor.entity_id);
});

