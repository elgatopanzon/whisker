/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_camera
 * @created     : Monday Apr 06, 2026 18:24:15 CST
 */

#include "whisker_std.h"
#include "whisker_rendering.h"
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

	debug_printf("activating camera entity: %d", itor.entity_id);

	// remove request tag after processing to prevent repeated activation
	w_ecs_remove_tag_str(world, W_RENDERING_CAMERA_REQUEST_ACTIVATE_CAMERA, itor.entity_id);
});


w_ecs_simple_system(
	camera_state_sync,
	WM_PHASE_PRE_RENDER,
{
	struct w_rendering_camera_state *camera_state = w_ecs_get_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID);

	// skip when no active camera
	if (camera_state->camera_entity_id == W_ENTITY_INVALID) return;

	// grab camera components
	w_vec3 camera_position = *w_ecs_get_str(world, w_vec3, "position", camera_state->camera_entity_id); 
	w_vec3 camera_target = *w_ecs_get_str(world, w_vec3, W_RENDERING_CAMERA_COMPONENT_TARGET, camera_state->camera_entity_id); 
	w_vec3 camera_up = *w_ecs_get_str(world, w_vec3, W_RENDERING_CAMERA_COMPONENT_UP, camera_state->camera_entity_id); 
	float camera_fov_deg = *w_ecs_get_str(world, float, W_RENDERING_CAMERA_COMPONENT_FOV, camera_state->camera_entity_id);
	enum W_RENDERING_CAMERA_PROJECTION camera_projection = *w_ecs_get_str(world, uint, W_RENDERING_CAMERA_COMPONENT_PROJECTION, camera_state->camera_entity_id);

	// set camera state from active entity
	camera_state->camera_position = camera_position;
	camera_state->camera_target = camera_target;
	camera_state->camera_up = camera_up;
	camera_state->camera_fov_deg = camera_fov_deg;
	camera_state->camera_projection = camera_projection;
});
