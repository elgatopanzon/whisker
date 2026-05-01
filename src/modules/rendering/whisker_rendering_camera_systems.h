/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_camera_systems
 * @created     : Tuesday Apr 28, 2026 14:42:31 CST
 * @description : 
 */

#include "whisker_components.h"
#include "whisker_rendering.h"
#include "whisker_rendering_camera.h"

#ifndef WHISKER_RENDERING_CAMERA_SYSTEMS_H
#define WHISKER_RENDERING_CAMERA_SYSTEMS_H

w_ecs_system(
	camera_request_active_camera,
	WM_RENDER_PHASE_PRE_SYNC,
	w_query(
		w_query_h(camera),
		w_query_h(req_active_camera),
	),
{
	struct w_rendering_camera_state *camera_state = w_ecs_get_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID);

	camera_state->camera_entity_id = entity;

	debug_printf("activating camera entity: %d", entity);

	// remove request tag after processing
	w_set_tag(entity, req_active_camera, false);
});


w_ecs_simple_system(
	camera_state_sync,
	WM_RENDER_PHASE_ON_SYNC,
{
	struct w_rendering_camera_state *camera_state = w_ecs_get_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID);

	// skip when no active camera
	if (camera_state->camera_entity_id == W_ENTITY_INVALID) return;
	w_entity_id entity = camera_state->camera_entity_id;

	// set camera state directly from active entity components
	camera_state->camera_position = *w_get(entity, position_3d);
	camera_state->camera_target = *w_get(entity, camera_target);
	camera_state->camera_up = *w_get(entity, camera_up);
	camera_state->camera_fov_deg = *w_get(entity, camera_fov_deg);
	camera_state->camera_near_clip = *w_get(entity, camera_near_clip);
	camera_state->camera_far_clip = *w_get(entity, camera_far_clip);
	camera_state->camera_projection = *w_get(entity, camera_projection);
});


#endif /* WHISKER_RENDERING_CAMERA_SYSTEMS_H */

