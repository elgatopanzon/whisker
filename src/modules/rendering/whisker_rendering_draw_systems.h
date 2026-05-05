/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_draw_systems
 * @created     : Saturday May 02, 2026 11:02:14 CST
 * @description : 
 */

#include "whisker_rendering.h"
#include "whisker_rendering_commands.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_RENDERING_DRAW_SYSTEMS_H
#define WHISKER_RENDERING_DRAW_SYSTEMS_H

w_ecs_simple_system(
	w_rendering_draw_dispatch_draw_begin, 
	WM_RENDER_PHASE_ON_DRAW,
{
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_DRAW_BEGIN,
		phase_id
	);
});

w_ecs_simple_system(
	w_rendering_draw_dispatch_draw_end, 
	WM_RENDER_PHASE_ON_DRAW,
{
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_DRAW_END,
		phase_id
	);
});

w_ecs_simple_system(
	w_rendering_draw_dispatch_draw_clear_color,
	WM_RENDER_PHASE_ON_DRAW,
{
	struct w_rendering_render_config *render_config = w_rendering_get_render_config(world);

	w_rendering_dispatch_render_cmd_value(
		W_RENDERING_CMD_CLEAR_COLOR,
		phase_id,
		struct w_rendering_cmd_clear_color,
			.clear_color = render_config->render_clear_color
	);
});

w_ecs_simple_system(
	w_rendering_draw_dispatch_draw_main_framebuffer,
	WM_RENDER_PHASE_ON_DRAW,
{
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_DRAW_MAIN_FRAMEBUFFER,
		phase_id
	);
});

#endif /* WHISKER_RENDERING_DRAW_SYSTEMS_H */

