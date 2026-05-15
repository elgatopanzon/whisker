/**
 * @author      : ElGatoPanzon
 * @file        : whisker_rendering
 * @created     : Monday Mar 30, 2026 11:17:25 CST
 * @description : Rendering base module
 */

#include "whisker_rendering.h"

#include "whisker_rendering_core_systems.h"
#include "whisker_rendering_window_systems.h"
#include "whisker_rendering_framebuffer_systems.h"
#include "whisker_rendering_scaling_systems.h"
#include "whisker_rendering_draw_systems.h"
#include "whisker_rendering_camera_systems.h"
#include "whisker_rendering_shapes_systems.h"
#include "whisker_rendering_shapes_procedural_systems.h"
#include "whisker_rendering_transform_modifier_systems.h"
#include "whisker_rendering_text_systems.h"

void wm_rendering_init(struct w_ecs_world *world, struct w_rendering_display_config *display_config, struct w_rendering_render_config *render_config)
{
	// set world resources
	w_ecs_set_module_resource(world, WM_RENDERING_DISPLAY_CONFIG_RESOURCE_ID, display_config);
	w_ecs_set_module_resource(world, WM_RENDERING_RENDER_CONFIG_RESOURCE_ID, render_config);

	// init a render state resource
	struct w_rendering_render_state *render_state = w_mem_xcalloc_t(1, struct w_rendering_render_state);
	w_ecs_set_module_resource(world, WM_RENDERING_RENDER_STATE_RESOURCE_ID, render_state);

	// init the render dispatch buffer
	struct w_dispatch_buffer *render_buffer = w_mem_xcalloc_t(1, *render_buffer);
	w_dispatch_buffer_init(render_buffer);
	w_ecs_set_module_resource(world, WM_RENDERING_RENDER_DISPATCH_BUFFER_RESOURCE_ID, render_buffer);

	// register custom rendering phases
	struct w_scheduler_phase phase_pre_scale = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_PRE_SCALE"};
	struct w_scheduler_phase phase_on_scale = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_ON_SCALE"};
	struct w_scheduler_phase phase_post_scale = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_POST_SCALE"};
	struct w_scheduler_phase phase_pre_filter = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_PRE_FILTER"};
	struct w_scheduler_phase phase_on_filter = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_ON_FILTER"};
	struct w_scheduler_phase phase_post_filter = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_POST_FILTER"};
	struct w_scheduler_phase phase_pre_draw = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_PRE_DRAW"};
	struct w_scheduler_phase phase_on_draw = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_ON_DRAW"};
	struct w_scheduler_phase phase_post_draw = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_POST_DRAW"};
	struct w_scheduler_phase phase_pre_world = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_PRE_WORLD"};
	struct w_scheduler_phase phase_begin_world = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_BEGIN_WORLD"};
	struct w_scheduler_phase phase_world_background = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_WORLD_BACKGROUND"};
	struct w_scheduler_phase phase_on_world = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_ON_WORLD"};
	struct w_scheduler_phase phase_world_foreground = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_WORLD_FOREGROUND"};
	struct w_scheduler_phase phase_end_world = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_END_WORLD"};
	struct w_scheduler_phase phase_post_world = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_POST_WORLD"};
	struct w_scheduler_phase phase_pre_sync = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_PRE_SYNC"};
	struct w_scheduler_phase phase_on_sync = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_ON_SYNC"};
	struct w_scheduler_phase phase_post_sync = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_POST_SYNC"};
	struct w_scheduler_phase phase_pre_overlay = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_PRE_OVERLAY"};
	struct w_scheduler_phase phase_on_overlay = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_ON_OVERLAY"};
	struct w_scheduler_phase phase_post_overlay = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER, .name = "RENDER_POST_OVERLAY"};

	w_ecs_register_system_phase_at(world, &phase_pre_scale, WM_RENDER_PHASE_PRE_SCALE);
	w_ecs_register_system_phase_at(world, &phase_on_scale, WM_RENDER_PHASE_ON_SCALE);
	w_ecs_register_system_phase_at(world, &phase_post_scale, WM_RENDER_PHASE_POST_SCALE);
	w_ecs_register_system_phase_at(world, &phase_pre_filter, WM_RENDER_PHASE_PRE_FILTER);
	w_ecs_register_system_phase_at(world, &phase_on_filter, WM_RENDER_PHASE_ON_FILTER);
	w_ecs_register_system_phase_at(world, &phase_post_filter, WM_RENDER_PHASE_POST_FILTER);
	w_ecs_register_system_phase_at(world, &phase_pre_draw, WM_RENDER_PHASE_PRE_DRAW);
	w_ecs_register_system_phase_at(world, &phase_on_draw, WM_RENDER_PHASE_ON_DRAW);
	w_ecs_register_system_phase_at(world, &phase_post_draw, WM_RENDER_PHASE_POST_DRAW);
	w_ecs_register_system_phase_at(world, &phase_pre_world, WM_RENDER_PHASE_PRE_WORLD);
	w_ecs_register_system_phase_at(world, &phase_begin_world, WM_RENDER_PHASE_BEGIN_WORLD);
	w_ecs_register_system_phase_at(world, &phase_world_background, WM_RENDER_PHASE_WORLD_BACKGROUND);
	w_ecs_register_system_phase_at(world, &phase_on_world, WM_RENDER_PHASE_ON_WORLD);
	w_ecs_register_system_phase_at(world, &phase_world_foreground, WM_RENDER_PHASE_WORLD_FOREGROUND);
	w_ecs_register_system_phase_at(world, &phase_end_world, WM_RENDER_PHASE_END_WORLD);
	w_ecs_register_system_phase_at(world, &phase_post_world, WM_RENDER_PHASE_POST_WORLD);
	w_ecs_register_system_phase_at(world, &phase_pre_sync, WM_RENDER_PHASE_PRE_SYNC);
	w_ecs_register_system_phase_at(world, &phase_on_sync, WM_RENDER_PHASE_ON_SYNC);
	w_ecs_register_system_phase_at(world, &phase_post_sync, WM_RENDER_PHASE_POST_SYNC);
	w_ecs_register_system_phase_at(world, &phase_pre_overlay, WM_RENDER_PHASE_PRE_OVERLAY);
	w_ecs_register_system_phase_at(world, &phase_on_overlay, WM_RENDER_PHASE_ON_OVERLAY);
	w_ecs_register_system_phase_at(world, &phase_post_overlay, WM_RENDER_PHASE_POST_OVERLAY);

	// assign phases order
	// pre-sync: PRE_RENDER -> pre/on/post sync
	w_ecs_set_phase_chain(world,
		WM_PHASE_PRE_RENDER,
		WM_RENDER_PHASE_PRE_SYNC,
		WM_RENDER_PHASE_ON_SYNC,
		WM_RENDER_PHASE_POST_SYNC
	);

	// world render phases: ON_RENDER -> world pre/on/post
	w_ecs_set_phase_chain(world,
		WM_PHASE_ON_RENDER,
		WM_RENDER_PHASE_PRE_WORLD,
		WM_RENDER_PHASE_BEGIN_WORLD,
		WM_RENDER_PHASE_WORLD_BACKGROUND,
		WM_RENDER_PHASE_ON_WORLD,
		WM_RENDER_PHASE_WORLD_FOREGROUND,
		WM_RENDER_PHASE_END_WORLD,
		WM_RENDER_PHASE_POST_WORLD
	);

	// overlay render phases: POST_WORLD -> overlay pre/on/post
	w_ecs_set_phase_chain(world,
		WM_RENDER_PHASE_POST_WORLD,
		WM_RENDER_PHASE_PRE_OVERLAY,
		WM_RENDER_PHASE_ON_OVERLAY,
		WM_RENDER_PHASE_POST_OVERLAY
	);

	// final draw phases: POST_RENDER -> scale -> filter -> draw
	w_ecs_set_phase_chain(world,
		WM_PHASE_POST_RENDER,
		WM_RENDER_PHASE_PRE_SCALE,
		WM_RENDER_PHASE_ON_SCALE,
		WM_RENDER_PHASE_POST_SCALE,
		WM_RENDER_PHASE_PRE_FILTER,
		WM_RENDER_PHASE_ON_FILTER,
		WM_RENDER_PHASE_POST_FILTER,
		WM_RENDER_PHASE_PRE_DRAW,
		WM_RENDER_PHASE_ON_DRAW,
		WM_RENDER_PHASE_POST_DRAW,
	);


	/******************
	*  core systems  *
	******************/
	w_rendering_core_flush_render_dispatch_buffer_dummy_register(world);
	w_rendering_core_dispatch_render_state_sync_register(world);
	w_rendering_core_sync_transform_components_register(world);


	/********************
	*  window systems  *
	********************/
	w_rendering_window_dispatch_init_window_register(world);
	w_rendering_window_dispatch_close_window_register(world);
	w_rendering_window_dispatch_handle_window_close_register(world);


	/*************************
	*  framebuffer systems  *
	*************************/
	w_rendering_framebuffer_dispatch_init_main_framebuffer_register(world);
	w_rendering_framebuffer_dispatch_activate_main_framebuffer_register(world);
	w_rendering_framebuffer_dispatch_clear_color_register(world);
	w_rendering_framebuffer_dispatch_deactivate_main_framebuffer_register(world);
	w_rendering_framebuffer_dispatch_set_filter_register(world);


	/*********************
	*  scaling systems  *
	*********************/
	w_rendering_scaling_rect_init_register(world);
	w_rendering_scaling_stretch_register(world);
	w_rendering_scaling_fit_register(world);
	w_rendering_scaling_integer_register(world);


	/******************
	*  draw systems  *
	******************/
	// order dependant
	w_rendering_draw_dispatch_draw_begin_register(world);
	w_rendering_draw_dispatch_draw_clear_color_register(world);
	w_rendering_draw_dispatch_draw_main_framebuffer_register(world);
	w_rendering_draw_dispatch_draw_end_register(world);
	

	/************************************
	*  transform modification systems  *
	************************************/
	w_rendering_transform_mod_billboard_sync_register(world);
	w_rendering_transform_mod_scale_screen_sync_register(world);

	/*******************
	*  camera module  *
	*******************/
	// init camera state resource
	struct w_rendering_camera_state *camera_state = w_mem_xcalloc_t(1, struct w_rendering_camera_state);

	// init a default camera
	camera_state->camera_entity_id = W_ENTITY_INVALID;

	camera_state->camera_position = ((w_vec3){ 0.0f, 0.0f, 6.0f });
    camera_state->camera_rotation = rotation_3d_default();
    camera_state->camera_up = camera_up_default();
    camera_state->camera_fov_deg = camera_fov_deg_default();
    camera_state->camera_near_clip = camera_near_clip_default();
    camera_state->camera_far_clip = camera_far_clip_default();
    camera_state->camera_projection = camera_projection_default();

	w_ecs_set_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID, camera_state);

	// register camera systems
	w_rendering_camera_request_active_camera_register(world);
	w_rendering_camera_state_sync_register(world);
	w_rendering_camera_look_at_target_sync_register(world);
	w_rendering_camera_dispatch_begin_camera_3d_register(world);
	w_rendering_camera_dispatch_end_camera_3d_register(world);


	/********************
	*  shapes systems  *
	********************/
	w_rendering_shapes_dispatch_draw_points_register(world);
	w_rendering_shapes_dispatch_draw_lines_register(world);
	w_rendering_shapes_dispatch_draw_triangles_register(world);
	w_rendering_shapes_dispatch_draw_rects_register(world);
	w_rendering_shapes_dispatch_draw_cubes_register(world);
	w_rendering_shapes_procedural_dispatch_draw_grid_register(world);
	w_rendering_shapes_procedural_dispatch_draw_circle_register(world);
	w_rendering_shapes_procedural_dispatch_draw_cylinder_register(world);


	/******************
	*  text systems  *
	******************/
	w_rendering_text_dispatch_draw_text_register(world);
}

void wm_rendering_free(struct w_ecs_world *world)
{
	w_ecs_clear_module_resource(world, WM_RENDERING_DISPLAY_CONFIG_RESOURCE_ID);
	w_ecs_clear_module_resource(world, WM_RENDERING_RENDER_CONFIG_RESOURCE_ID);

	free(w_ecs_get_module_resource(world, WM_RENDERING_RENDER_STATE_RESOURCE_ID));
	w_ecs_clear_module_resource(world, WM_RENDERING_RENDER_STATE_RESOURCE_ID);

	struct w_dispatch_buffer *render_buffer = w_ecs_get_module_resource(world, WM_RENDERING_RENDER_DISPATCH_BUFFER_RESOURCE_ID);
	w_dispatch_buffer_free(render_buffer);
	free_null(render_buffer);
	w_ecs_clear_module_resource(world, WM_RENDERING_RENDER_DISPATCH_BUFFER_RESOURCE_ID);

	free(w_ecs_get_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID));
	w_ecs_clear_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID);
}
