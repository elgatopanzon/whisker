/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_camera_systems
 * @created     : Tuesday Apr 28, 2026 14:42:31 CST
 * @description : 
 */

#include "whisker_math.h"
#include "whisker_components.h"
#include "whisker_rendering.h"
#include "whisker_rendering_camera.h"
#include "whisker_rendering_commands.h"

#ifndef WHISKER_RENDERING_CAMERA_SYSTEMS_H
#define WHISKER_RENDERING_CAMERA_SYSTEMS_H

w_ecs_system(
	w_rendering_camera_request_active_camera,
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
	w_rendering_camera_state_sync,
	WM_RENDER_PHASE_ON_SYNC,
{
	struct w_rendering_camera_state *camera_state = w_ecs_get_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID);

	// skip when no active camera
	if (camera_state->camera_entity_id == W_ENTITY_INVALID) return;
	w_entity_id entity = camera_state->camera_entity_id;

	// set camera state directly from active entity components
	camera_state->camera_position = *w_get(entity, position_3d);
	camera_state->camera_rotation = *w_get(entity, rotation_3d);
	camera_state->camera_up = *w_get(entity, camera_up);
	camera_state->camera_fov_deg = *w_get(entity, camera_fov_deg);
	camera_state->camera_near_clip = *w_get(entity, camera_near_clip);
	camera_state->camera_far_clip = *w_get(entity, camera_far_clip);
	camera_state->camera_projection = *w_get(entity, camera_projection);
});

w_ecs_system(
	w_rendering_camera_look_at_target_sync,
	WM_RENDER_PHASE_ON_SYNC,
	w_query(
		w_query_h(camera),
		w_query_r(camera_target),
		w_query_r(camera_up),
		w_query_r(position_3d),
		w_query_w(rotation_3d)
	),
{
	// we negate the position to make it -Z as look_rotation uses +Z
	*w_query_get(rotation_3d) = w_quat_look_rotation(
    	w_vec3_sub(*w_query_get(position_3d), *w_query_get(camera_target)),
    	*w_query_get(camera_up)
	);
});

/**********************
*  dispatch systems  *
**********************/

w_ecs_simple_system(
	w_rendering_camera_dispatch_begin_camera_3d,
	WM_RENDER_PHASE_BEGIN_WORLD,
{
	// grab the camera state and render state
	struct w_rendering_camera_state *camera_state = w_rendering_get_camera_state(world);
	struct w_rendering_render_config *render_config = w_rendering_get_render_config(world);

	// compute aspect ratio from render width and height
	// note: this is sync'd in from the context already
    float aspect = (float)render_config->render_resolution.x / (float)render_config->render_resolution.y;

	// handle perspective and orthographic camera modes
	double top;
	double right;
    switch (camera_state->camera_projection) {
    	case W_RENDERING_CAMERA_PROJECTION_PERSPECTIVE:
        	top = camera_state->camera_near_clip * tan(camera_state->camera_fov_deg * 0.5 * W_DEG2RAD);
        	right = top * aspect;
    		break;
    	case W_RENDERING_CAMERA_PROJECTION_ORTHOGRAPHIC:
        	top = camera_state->camera_fov_deg / 2.0;
        	right = top * aspect;
    		break;
    	default:
    		break;
    }

	// calculate the view matrix from the camera
    /* w_mat4 camera_matrix = w_mat4_look_at(camera_state->camera_position, camera_state->camera_target, camera_state->camera_up); */
    w_mat4 camera_matrix = w_mat4_inverse(w_mat4_from_trs(
       camera_state->camera_position,
       camera_state->camera_rotation,
       ((w_vec3){1,1,1})
   	));

	w_rendering_dispatch_render_cmd_value(
		W_RENDERING_CMD_CAMERA_BEGIN_3D,
		phase_priority,
		struct w_rendering_cmd_camera_begin_3d,
			.projection_type = camera_state->camera_projection,
			.left = -right, .right = right,
			.bottom = -top, .top = top,
			.near_clip = camera_state->camera_near_clip,
			.far_clip = camera_state->camera_far_clip,
			.view_matrix = camera_matrix,
	);
});

w_ecs_simple_system(
	w_rendering_camera_dispatch_end_camera_3d, 
	WM_RENDER_PHASE_END_WORLD,
{
	// currently no payload, upstream just implements this
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_CAMERA_END_3D,
		phase_priority
	);
});

#endif /* WHISKER_RENDERING_CAMERA_SYSTEMS_H */

