/**
 * @author      : ElGatoPanzon
 * @file        : whisker_rendering
 * @created     : Monday Mar 30, 2026 11:17:25 CST
 * @description : Rendering base module
 */

#include "whisker_rendering.h"
#include "whisker_rendering_camera.h"
#include "whisker_rendering_camera.c"

#include "whisker_rendering_scaling.c"

void wm_rendering_init(struct w_ecs_world *world, struct w_rendering_display_config *display_config, struct w_rendering_render_config *render_config)
{
	// set world resources
	w_ecs_set_module_resource(world, WM_RENDERING_DISPLAY_CONFIG_RESOURCE_ID, display_config);
	w_ecs_set_module_resource(world, WM_RENDERING_RENDER_CONFIG_RESOURCE_ID, render_config);

	// init a render state resource
	struct w_rendering_render_state *render_state = w_mem_xcalloc_t(1, struct w_rendering_render_state);
	w_ecs_set_module_resource(world, WM_RENDERING_RENDER_STATE_RESOURCE_ID, render_state);

	// register custom rendering phases
	struct w_scheduler_phase phase = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_RENDER};
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_PRE_SCALE);
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_ON_SCALE);
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_POST_SCALE);
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_PRE_FILTER);
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_ON_FILTER);
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_POST_FILTER);
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_PRE_DRAW);
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_ON_DRAW);
	w_ecs_register_system_phase_at(world, &phase, WM_RENDER_PHASE_POST_DRAW);

	// assign phases order
	// Chain: POST_RENDER -> scale -> filter -> draw
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_PRE_SCALE, WM_PHASE_POST_RENDER);
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_ON_SCALE, WM_RENDER_PHASE_PRE_SCALE);
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_POST_SCALE, WM_RENDER_PHASE_ON_SCALE);
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_PRE_FILTER, WM_RENDER_PHASE_POST_SCALE);
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_ON_FILTER, WM_RENDER_PHASE_PRE_FILTER);
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_POST_FILTER, WM_RENDER_PHASE_ON_FILTER);
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_PRE_DRAW, WM_RENDER_PHASE_POST_FILTER);
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_ON_DRAW, WM_RENDER_PHASE_PRE_DRAW);
	w_ecs_set_system_phase_runs_after(world, WM_RENDER_PHASE_POST_DRAW, WM_RENDER_PHASE_ON_DRAW);

	// scaling systems
	whisker_rendering_scaling_rect_init_register(world);
	whisker_rendering_scaling_stretch_register(world);
	whisker_rendering_scaling_fit_register(world);
	whisker_rendering_scaling_integer_register(world);


	/*******************
	*  camera module  *
	*******************/
	
	// init camera state resource
	struct w_rendering_camera_state *camera_state = w_mem_xcalloc_t(1, struct w_rendering_camera_state);
	w_ecs_set_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID, camera_state);

	// register camera systems
	camera_request_active_camera_register(world);
}

void wm_rendering_free(struct w_ecs_world *world)
{
	w_ecs_clear_module_resource(world, WM_RENDERING_DISPLAY_CONFIG_RESOURCE_ID);
	w_ecs_clear_module_resource(world, WM_RENDERING_RENDER_CONFIG_RESOURCE_ID);

	free(w_ecs_get_module_resource(world, WM_RENDERING_RENDER_STATE_RESOURCE_ID));
	w_ecs_clear_module_resource(world, WM_RENDERING_RENDER_STATE_RESOURCE_ID);

	free(w_ecs_get_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID));
	w_ecs_clear_module_resource(world, WM_RENDERING_CAMERA_STATE_RESOURCE_ID);
}
