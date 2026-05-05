/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_framebuffer_systems
 * @created     : Friday May 01, 2026 17:27:39 CST
 * @description : 
 */

#include "whisker_rendering.h"
#include "whisker_rendering_commands.h"

#ifndef WHISKER_RENDERING_FRAMEBUFFER_SYSTEMS_H
#define WHISKER_RENDERING_FRAMEBUFFER_SYSTEMS_H

w_ecs_simple_system(
	w_rendering_framebuffer_dispatch_init_main_framebuffer, 
	WM_PHASE_ON_STARTUP,
{
	// currently no payload, upstream just implements this
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_FRAMEBUFFER_MAIN_INIT,
		phase_priority
	);
});

w_ecs_simple_system(
	w_rendering_framebuffer_dispatch_activate_main_framebuffer, 
	WM_PHASE_PRE_RENDER,
{
	// currently no payload, upstream just implements this
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_FRAMEBUFFER_MAIN_ACTIVATE,
		phase_priority
	);
});

w_ecs_simple_system(
	w_rendering_framebuffer_dispatch_deactivate_main_framebuffer, 
	WM_PHASE_POST_RENDER,
{
	// currently no payload, upstream just implements this
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_FRAMEBUFFER_MAIN_DEACTIVATE,
		phase_priority
	);
});

w_ecs_simple_system(
	w_rendering_framebuffer_dispatch_clear_color,
	WM_PHASE_PRE_RENDER,
{
	struct w_rendering_render_config *render_config = w_rendering_get_render_config(world);

	if (render_config->clear_enabled)
	{
		w_rendering_dispatch_render_cmd_value(
			W_RENDERING_CMD_CLEAR_COLOR,
			phase_priority,
			struct w_rendering_cmd_clear_color,
				.clear_color = render_config->window_clear_color
		);
	}
});


w_ecs_simple_system(
	w_rendering_framebuffer_dispatch_set_filter, 
	WM_RENDER_PHASE_ON_FILTER,
{
	struct w_rendering_render_config *render_config = w_rendering_get_render_config(world);

	// currently no payload, upstream just implements this
	w_rendering_dispatch_render_cmd_value(
		W_RENDERING_CMD_FRAMEBUFFER_MAIN_SET_FILTER,
		phase_priority,
		struct w_rendering_cmd_framebuffer_set_filter,
			.filter_type = render_config->texture_filter,
	);
});

#endif /* WHISKER_RENDERING_FRAMEBUFFER_SYSTEMS_H */

