/**
 * @author      : ElGatoPanzon
 * @file        : whisker_rendering
 * @created     : Monday Mar 30, 2026 11:17:25 CST
 * @description : Rendering base module
 */

#include "whisker_rendering.h"
#include "whisker_rendering_camera.h"

void wm_rendering_init(struct w_ecs_world *world, struct w_rendering_display_config *display_config, struct w_rendering_render_config *render_config)
{
	// set world resources
	w_ecs_set_module_resource(world, WM_RENDERING_DISPLAY_CONFIG_RESOURCE_ID, display_config);
	w_ecs_set_module_resource(world, WM_RENDERING_RENDER_CONFIG_RESOURCE_ID, render_config);

	// init a render state resource
	struct w_rendering_render_state *render_state = w_mem_xcalloc_t(1, struct w_rendering_render_state);
	w_ecs_set_module_resource(world, WM_RENDERING_RENDER_STATE_RESOURCE_ID, render_state);


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
